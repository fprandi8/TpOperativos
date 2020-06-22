/*
 * MemoriaCache.c
 *
 *  Created on: 30 may. 2020
 *      Author: utnso
 */

#include "CacheMemory.h"


void signal_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        printf("Received SIGUSR1!\n");
    }
}


void start_cache(void)
{
    //Receives config from Broker
    config = get_config();
    define_partition_size();
    define_cache_maximum_size();
    set_full_memory();

	sem_init(mutex_nextPartitionId, 0, 1);
	sem_init(mutex_partitions, 0, 1);
	sem_init(mutex_cached_messages, 0, 1);

	signal(SIGUSR1, signal_handler);

    t_partition* first_partition = CreateNewPartition();
    first_partition->queue_type = 0;
    first_partition->free = 1;
    first_partition->timestap = clock();
	first_partition->begining = cache.full_memory;

	if(strcmp(config_get_string_value(config, ALGORITMO_MEMORIA),"DYNAMIC"))
	{
    	first_partition->size = cache.memory_size;
	}
	else
	{
		//Get the power of two nearest to the memory size we have
		double powerOfTwo = CalculateNearestPowerOfTwo(cache.memory_size);
		first_partition->size = (int)pow(2, powerOfTwo);
	}


	sem_wait(mutex_partitions);
    list_add(partitions, first_partition);
	sem_post(mutex_partitions);

}

t_config* get_config(){
    return config_create("/home/utnso/workspace/tp-2020-1c-MATE-OS/Broker/Broker.config");
}

void define_partition_size(void){
    cache.partition_minimum_size = config_get_int_value(config, TAMANO_MINIMO_PARTICION);
}

void define_cache_maximum_size(void){
    cache.memory_size = config_get_int_value(config, TAMANO_MEMORIA);
}

void set_full_memory(void){
    cache.full_memory = (char*) malloc(cache.memory_size * sizeof(char));
}

void save_message(deli_message message){
    t_cachedMessage new_message = create_cached_from_message(message);
    new_message.partitionId = save_message_body(message.messageContent, message.messageType);
    add_to_cached_messages(new_message);

}

t_cachedMessage create_cached_from_message(deli_message message){
    t_cachedMessage new_message;
    new_message.id = message.id;
    new_message.corelationId = message.correlationId;
    new_message.queue_type = message.messageType;
    new_message.sent_to_subscribers = NULL;
    new_message.ack_by_subscribers = NULL;
    new_message.partitionId = 0;
    return new_message;
}

void add_to_cached_messages(t_cachedMessage new_message){
	sem_wait(mutex_cached_messages);
    list_add(cached_messages, &new_message);
	sem_post(mutex_cached_messages);
}

int save_message_body(void* messageContent, message_type queue){
    t_buffer* messageBuffer = SerializeMessageContent(queue, messageContent);
	sem_wait(mutex_partitions);
	t_partition* partition;
	if(strcmp(config_get_string_value(config, ALGORITMO_MEMORIA),"DYNAMIC"))
	{
    	partition = find_empty_partition_of_size(sizeof(messageBuffer->bufferSize));
	} 
	else 
	{
		int size;
		partition = select_partition_bf(size);
	}
    int savedPartitionId = save_body_in_partition(messageBuffer, partition, queue);
	sem_post(mutex_partitions);
	return savedPartitionId;
}

uint32_t save_body_in_partition(t_buffer* messageBuffer, t_partition* partition, message_type queue)
{

    if(partition->size == messageBuffer->bufferSize)
    {
        partition->queue_type = queue;
        partition->free = 0;
        partition->timestap = clock();

        nextPartitionId++;

        memcpy(partition->begining, messageBuffer->stream, sizeof(messageBuffer->bufferSize));
        return partition->id;
    }

    if(strcmp(config_get_string_value(config, ALGORITMO_MEMORIA),"DYNAMIC"))
    {
        int newPartitionSize = config_get_int_value(config, TAMANO_MINIMO_PARTICION);
        if(messageBuffer->bufferSize > newPartitionSize) newPartitionSize = messageBuffer->bufferSize;

        t_partition* newPartition = CreateNewPartition();
        newPartition->begining = partition->begining;
        newPartition->size = newPartitionSize;
        newPartition->queue_type = queue;
        newPartition->free = 0;
        newPartition->timestap = clock();

        list_add(partitions, newPartition);

        partition->begining += newPartitionSize;
        partition->timestap = clock();

        memcpy(partition->begining, messageBuffer->stream, sizeof(messageBuffer->bufferSize));
        return newPartition->id;
    }
    else
    {
		int run = 1;
		while(run == 1)
		{
			if(
				(partition->size / 2 < messageBuffer->bufferSize && partition->size > messageBuffer->bufferSize) || 
				partition->size / 2 >= config_get_int_value(config, TAMANO_MINIMO_PARTICION)
			)
			{
				partition->queue_type = queue;
				partition->free = 0;
				partition->timestap = clock();

				nextPartitionId++;

				memcpy(partition->begining, messageBuffer->stream, sizeof(messageBuffer->bufferSize));
				return partition->id;	
			} 
			else 
			{
				t_partition* newPartition = CreateNewPartition();
				newPartition->begining = partition->begining;
				newPartition->size = partition->size / 2 ;
				newPartition->queue_type = queue;
				newPartition->free = 0;
				newPartition->timestap = clock();

				partition->begining += 0;//newPartitionSize;
				partition->timestap = clock();

				list_add(partitions, newPartition);

				partition = newPartition;
			}
		}
    }
}


t_partition* find_empty_partition_of_size(uint32_t size){
//	if(size > cache.memory_size); //TODO imprimir se pico, no deberia pasar que llegue algo mas grande que la memoria
    t_partition* partition = select_partition(size);
    int compaction_frequency =  config_get_int_value(config, FRECUENCIA_COMPACTACION);
    if(partition != NULL) return partition;
    if(compaction_frequency == -1)
    {
        int busyPartitions = 0;
        do
        {
            delete_partition();
            busyPartitions = GetBusyPartitionsCount();
            partition = select_partition(size);
        }
        while(partition == NULL && busyPartitions > 0);
        if(partition == NULL)
        {
            compact_memory();
            partition = select_partition(size);
            return partition;
        } else
        {
        	return partition;
        }
    }
    else
    {
        //TODO maybe add a validator for config at the start so compaction_frequency == 0 or < -1 cannot be a case
        do
        {
            if(partition == NULL)
            {
                for(int i = 0; i < compaction_frequency; i++)
                {
                    delete_partition(); //TODO we need to know we run out of partitions to delete
                    partition = select_partition(size);
                }
            }
            if(partition == NULL)
            {
                compact_memory();
                partition = select_partition(size);
            }
        } while(partition == NULL);
        return partition;
    }
}

t_partition* select_partition(uint32_t size){
	t_partition *partition;
    if(strcmp(config_get_string_value(config, ALGORITMO_REEMPLAZO),"FF")){ //compare dif algoritmos
        partition = select_partition_ff(size);
    }else{
        partition = select_partition_bf(size);
    }
    return partition;
}

t_partition* select_partition_ff(uint32_t size){
    bool _does_partition_fit(t_partition* partition)
    {
        if(partition->free && partition->size >= size) return true; else return false;
    }
    t_partition* partition = (t_partition*)list_find(partitions, (void*)_does_partition_fit);
    return partition;
}


t_partition* select_partition_bf(uint32_t size){

    t_partition* bestFitPartition = NULL;
    void _get_best_fit(t_partition* partition)
    {
        if(partition->free == 0) return;
        if(bestFitPartition == NULL && partition->size >= size) bestFitPartition = partition;
        else
        {
            if(partition->size >= size && partition->size < bestFitPartition->size) bestFitPartition = partition;
        }
    }
    list_iterate(partitions, (void*)_get_best_fit);
    return bestFitPartition;
}


void compact_memory(void){
    bool _is_empty_partition(t_partition* partition){ return partition->free; }
    bool _filter_busy_partition(t_partition* partition){ return !partition->free;}

    t_list* occupied_partitions = list_filter(partitions, (void *) _filter_busy_partition);

    char* backUp_memory = (char*) malloc(cache.memory_size * sizeof(char));

    int offsetMem = 0;
    void _asignPartitionOnBackUpMem(t_partition* partition)
    {
        memcpy(backUp_memory + offsetMem, partition->begining, partition->size);
        offsetMem += partition->size;
    }

    int offsetPointerMem = 0;
    void _reasignPartitionPointers(t_partition* partition)
    {
        partition->begining = cache.full_memory + offsetPointerMem;
        offsetPointerMem += partition->size;
    }


    //- create big empty paritition
    uint32_t occupied_size = add_occupied_size_from(occupied_partitions);

    t_partition* emptySpacePartition = CreateNewPartition();
    emptySpacePartition->begining = NULL;
    emptySpacePartition->size = cache.memory_size - occupied_size;
    emptySpacePartition->queue_type = 0;
    emptySpacePartition->free = true;
    emptySpacePartition->timestap = clock();

    list_add(occupied_partitions, emptySpacePartition);

    list_iterate(occupied_partitions, (void*)_asignPartitionOnBackUpMem);
    memcpy(cache.full_memory, backUp_memory, sizeof(cache.memory_size));
    list_iterate(occupied_partitions, (void*)_reasignPartitionPointers);
    free(backUp_memory);

	list_remove_and_destroy_by_condition(partitions, (void*)_is_empty_partition, (void*)Free_CachedMessage);
	list_clean(partitions);
	free(partitions);

    partitions = occupied_partitions;
    //TODO add times compacting here or outside?
}

uint32_t add_occupied_size_from(t_list* occupied){
    uint32_t sum = 0;
    t_partition* partition = (t_partition*) malloc(sizeof(t_partition*));
    for (int i = 0; i < sizeof(occupied); ++i) {
        partition = list_get(occupied, i);
        sum += partition->size;
    }
    free(partition);
    return sum;
}

void check_compact_restrictions(void){
    //TODO
}

void delete_partition(void){
    int deletedParitionId;
    if(strcmp(config_get_string_value(config, ALGORITMO_REEMPLAZO),"FIFO")){//compare dif algoritmos
        deletedParitionId = delete_partition_fifo();
    }else{
        deletedParitionId = delete_partition_lru();
    }


    bool _message_to_delete(t_cachedMessage* message)
    {
        if(message->partitionId == deletedParitionId) return true; else return false;
    }

    list_remove_and_destroy_by_condition(cached_messages, (void*)_message_to_delete, (void*)Free_CachedMessage);
}

int delete_partition_fifo(void){
    bool _first_busy_partition(t_partition* partition)
    {
        if(partition->free == 0) return true; else return false;
    }
    t_partition* partition = (t_partition*)list_find(partitions, (void*)_first_busy_partition);
    partition->free = 1;
    return partition->id;
}

int delete_partition_lru(void){
    t_partition* leastUsedPartition = NULL;
    void _get_least_used(t_partition* partition)
    {
        if(leastUsedPartition == NULL) leastUsedPartition = partition;
        else
        {
            if(clock() - leastUsedPartition->timestap < clock() -  partition->timestap) leastUsedPartition = partition;
        }
    }

    list_iterate(partitions, (void*)_get_least_used);
    leastUsedPartition->free = 1;
    leastUsedPartition->timestap = clock();

    return leastUsedPartition->id;
}

int GetBusyPartitionsCount()
{
    bool _filter_busy_parittion(t_partition* partition)
    {
        if(partition->free == 0) return true; else return false;
    }
    t_list* busyPartitions = list_filter(partitions, (void*)_filter_busy_parittion);
    int busyPartitionsCount = list_size(busyPartitions);
    list_clean(busyPartitions);
    return busyPartitionsCount;
}

void Free_CachedMessage(t_cachedMessage* message)
{
    void _free_sendOrAck(void* sendOrAck)
    {
        free(sendOrAck);
    }
    list_clean_and_destroy_elements(message->sent_to_subscribers, (void*)_free_sendOrAck);
    list_clean_and_destroy_elements(message->ack_by_subscribers, (void*)_free_sendOrAck);
    free(message->sent_to_subscribers);
    free(message->ack_by_subscribers);
    free(message);
}

t_partition* CreateNewPartition()
{
    t_partition* partition = (t_partition*)malloc(sizeof(t_partition));
	sem_wait(mutex_nextPartitionId);
	partition->id = nextPartitionId;
    nextPartitionId++;
	sem_post(mutex_nextPartitionId);
    if(nextPartitionId== INT_MAX) nextPartitionId = 0;
    return partition;
}

t_list* GetMessagesFromQueue(message_type type)
{
    bool _message_by_queue(t_cachedMessage* message)
    {
        if(message->queue_type == type) return true; else return false;
    }
    return list_filter(cached_messages, (void*)_message_by_queue);
}

t_cachedMessage* GetCachedMessage(int messageId)
{
    bool _message_by_id(t_cachedMessage* message)
    {
        if(message->id == messageId) return true; else return false;
    }
    return (t_cachedMessage*)list_find(cached_messages, (void*)_message_by_id);
}

t_partition* GetPartition(int partitionId)
{
    bool _partition_by_id(t_partition* partition)
    {
        if(partition->id == partitionId) return true; else return false;
    }
    return (t_partition*)list_find(partitions, (void*)_partition_by_id);
}

void* GetMessageContent(int messageId)
{
    t_cachedMessage* message = GetCachedMessage(messageId);
    t_partition* partition = GetPartition(message->partitionId);
    return DeserializeMessageContent(partition->queue_type, partition->begining);
}



void PrintDumpOfCache()
{

}

/**
 *
 * @param freed_partition
 * @return index of the partition after consolidation, if >= 0 then call again with the partition
 * at index, if < 0 then break the cycle.
 */
int check_validations_and_consolidate(t_partition freed_partition){

    int index = find_index_in_list(freed_partition);
    t_partition* possible_partition = (t_partition*)malloc(sizeof(t_partition));
    int first_partition = 1;
    int possible_index;

    if(is_pair(index)){
        possible_index = index + 1;
    }else{
        possible_index = index - 1;
        first_partition = 2;
    }

    possible_partition = list_get(partitions, possible_index);

    if(!possible_partition->free){
        free(possible_partition);
        return -1;
    }

    if(possible_partition->size != freed_partition.size){
        free(possible_partition);
        return -1;
    }

    int result_index = consolidate(possible_partition, possible_index, freed_partition, index, first_partition);

    free(possible_partition);
    return result_index;
}

int is_pair(int index){
    return (index % 2) == 0;
}

int find_index_in_list(t_partition partition){
    int index = 0;
    int wanted_id = partition.id;
    t_partition* aux_partition = (t_partition*)malloc(sizeof(t_partition));
    while(index < sizeof(partitions)){
        aux_partition = list_get(partitions, index);
        if(aux_partition->id == wanted_id)
            break;
        index++;
    }
    free(aux_partition);
    return index;
}

double CalculateNearestPowerOfTwo(int x)
{
	double evenSize = x - (x % 2);
	return 1;//powerOfTwo = (int)floor(log10(evenSize) / log10(2));
}

double CalculateNearestPowerOfTwoRelativeToCache(int memoryLocation)
{
		int relativeMemory = memoryLocation; //- cache.full_memory;
		double evenSize = relativeMemory - (relativeMemory % 2);
		return 1;//powerOfTwo = (int)floor(log10(evenSize) / log10(2));
}

int consolidate(t_partition one_partition,int one_index, t_partition another_partition,int another_index, int first_partition){
    int consolidation_ok = -1;

    if(first_partition == 2){
        one_partition.size *= 2;
        free(list_remove(partitions, another_index));
        consolidation_ok = one_index;
    }else{
        another_partition.size *= 2;
        free(list_remove(partitions, one_index));
        consolidation_ok = another_index;
    }
    return consolidation_ok;
}
