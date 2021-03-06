/*
 * MemoriaCache.c
 *
 *  Created on: 30 may. 2020
 *      Author: utnso
 */

#include "CacheMemory.h"


void start_cache(t_log* log)
{
    cache_log = log;
    config = get_config();
    define_partition_size();
    define_cache_maximum_size();
    set_full_memory();

    memorySchemeIsDynamic = strcmp(config_get_string_value(config, ALGORITMO_MEMORIA),"PARTICIONES") == 0;
    partitionSelectionIsFirstFit = strcmp(config_get_string_value(config, ALGORITMO_PARTICION_LIBRE),"FF") == 0;
    victimSelectionIsFifo = strcmp(config_get_string_value(config, ALGORITMO_REEMPLAZO),"FIFO") == 0;

    //Initialize all semaphores
	sem_init(&mutex_nextPartitionId, 0, 1);
	sem_init(&mutex_partitions, 0, 1);
	sem_init(&mutex_cached_messages, 0, 1);
    sem_init(&mutex_parent_partitions, 0, 1);
    sem_init(&mutex_index_finder, 0, 1);
    sem_init(&mutex_index_finder_destroyer, 0, 1);
    sem_init(&mutex_saving, 0, 1);

    nextPartitionId = 0;

    t_partition* first_partition = CreateNewPartition();
    first_partition->queue_type = 0;
    first_partition->free = 1;
    first_partition->timestap = clock();
	first_partition->begining = cache.full_memory;

	if(memorySchemeIsDynamic)
	{
    	first_partition->size = cache.memory_size;
	}
	else
	{
		//Get the power of two nearest to the memory size we have
		double powerOfTwo = CalculateNearestPowerOfTwo(cache.memory_size);
		first_partition->size = (int)pow(2, powerOfTwo);
	}

	//Initialize lists
	//sem_wait(&mutex_cached_messages);
    cached_messages = list_create();
	//sem_post(&mutex_cached_messages);


	//sem_wait(&mutex_partitions);
	partitions = list_create();
    list_add(partitions, first_partition);
	//sem_post(&mutex_partitions);


	//sem_wait(&mutex_parent_partitions);
	parent_partitions = list_create();
	//sem_post(&mutex_parent_partitions);

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

void save_message(deli_message message)
{
	sem_wait(&mutex_saving);
    t_cachedMessage* new_message = create_cached_from_message(message);
    new_message->partitionId = save_message_body(message.messageContent, message.messageType);
    add_to_cached_messages(new_message);
	sem_post(&mutex_saving);
}

t_cachedMessage* create_cached_from_message(deli_message message)
{
    t_cachedMessage* new_message = (t_cachedMessage*)malloc(sizeof(t_cachedMessage));
    new_message->id = message.id;
    new_message->corelationId = message.correlationId;
    new_message->queue_type = message.messageType;
    new_message->sent_to_subscribers = list_create();
    new_message->ack_by_subscribers_left = 0;
    new_message->partitionId = 0;
    sem_init(&new_message->mutex_message, 0, 1);
    return new_message;
}

void add_to_cached_messages(t_cachedMessage* new_message)
{
	//sem_wait(&mutex_cached_messages);
    list_add(cached_messages, new_message);
//	sem_post(&mutex_cached_messages);
}

int save_message_body(void* messageContent, message_type queue){
    t_buffer* messageBuffer = SerializeMessageContent(queue, messageContent);

	t_partition* partition;
	uint32_t minSize = config_get_int_value(config, TAMANO_MINIMO_PARTICION);
	uint32_t requiredSize = messageBuffer->bufferSize > minSize ? messageBuffer->bufferSize : minSize;
    partition = find_empty_partition_of_size(requiredSize);
    log_info(cache_log, "MENSAJE GUARDADO EN PARTICION CON POSICION DE INICIO: 0x%X", partition->begining - cache.full_memory);
    int savedPartitionId = save_body_in_partition(messageBuffer, partition, queue);
	return savedPartitionId;
}

uint32_t save_body_in_partition(t_buffer* messageBuffer, t_partition* partition, message_type queue)
{
    int newPartitionSize = config_get_int_value(config, TAMANO_MINIMO_PARTICION);
    if(messageBuffer->bufferSize > newPartitionSize) newPartitionSize = messageBuffer->bufferSize;

    if(newPartitionSize == partition->size)
    {
        partition->queue_type = queue;
        partition->free = 0;
        partition->timestap = clock();

        memcpy(partition->begining, messageBuffer->stream, messageBuffer->bufferSize);

        //Take out the partition and add it again, this is so we keep consistent with the order of messages entering
        int indexOfPartition = find_index_in_list(partition);
        list_remove(partitions, indexOfPartition);
        list_add(partitions, partition);


        return partition->id;
    }

    if(memorySchemeIsDynamic)
    {
        t_partition* newPartition = CreateNewPartition();
        newPartition->begining = partition->begining;
        newPartition->size = newPartitionSize;
        newPartition->queue_type = queue;
        newPartition->free = 0;
        newPartition->timestap = clock();

        memcpy(newPartition->begining, messageBuffer->stream, messageBuffer->bufferSize);
        list_add(partitions, newPartition);

        partition->begining += newPartitionSize;
        partition->size = partition->size - newPartitionSize;
        partition->timestap = clock();

        return newPartition->id;
    }
    else
    {
		int run = 1;
		int firstOne = 1;
		while(run == 1)
		{
			if(
				(partition->size / 2 < newPartitionSize && partition->size >= newPartitionSize) ||
				partition->size / 2 < config_get_int_value(config, TAMANO_MINIMO_PARTICION)
			)
			{
				partition->queue_type = queue;
				partition->free = 0;
				partition->timestap = clock();

				memcpy(partition->begining, messageBuffer->stream, messageBuffer->bufferSize);

				if(firstOne)
				{
			        //Take out the partition and add it again, this is so we keep consistent with the order of messages entering
			        int indexOfPartition = find_index_in_list(partition);
			        list_remove(partitions, indexOfPartition);
			        list_add(partitions, partition);
				}

				return partition->id;	
			} 
			else 
			{
				firstOne = 0;
				partition = create_childrens_from(partition);
			}
		}
    }
    return -1;
}


t_partition* find_empty_partition_of_size(uint32_t size)
{
	if(size > cache.memory_size) return NULL;
    t_partition* partition = select_partition(size);
    int compaction_frequency =  config_get_int_value(config, FRECUENCIA_COMPACTACION);
    if(partition != NULL) return partition;
    int busyPartitions = GetBusyPartitionsCount();
    //If its BS or is Dynamic a has frequency -1,  we set compaction_frequency be to the same as busy partitions we have
    //0 frequency does not make sense, so we de-activate compaction_frequency also on 0
    if( !memorySchemeIsDynamic || (memorySchemeIsDynamic && compaction_frequency == -1) )
         compaction_frequency = busyPartitions;

    do
    {
        if(partition == NULL)
        {
            for(int i = 0; i < compaction_frequency; i++)
            {
                delete_partition();
                partition = select_partition(size);
                if(partition != NULL) break;
                if(busyPartitions - i <= 0) break;
            }
        }
        if(partition == NULL) //Reached compaction frequency, or run out of busy partitions to delete. Should never reach here on buddy system setting.
        {
            compact_memory();
            partition = select_partition(size);
        }
    } while(partition == NULL);
    return partition;
    
}

t_partition* select_partition(uint32_t size){
	t_partition *partition;
    if(partitionSelectionIsFirstFit && memorySchemeIsDynamic){ //compare dif algoritmos
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


void compact_memory(void)
{
    bool _is_empty_partition(t_partition* partition){ return partition->free; }
    bool _filter_busy_partition(t_partition* partition){ return !partition->free;}

   // sem_wait(&mutex_partitions);
    t_list* occupied_partitions = list_filter(partitions, (void *) _filter_busy_partition);
   // sem_post(&mutex_partitions);

    char* backUp_memory = (char*) malloc(cache.memory_size * sizeof(char));

    int offsetMem = 0;
    void _asignPartitionOnBackUpMem(t_partition* partition)
    {

    	memcpy(backUp_memory + offsetMem, partition->begining, partition->size);
        partition->begining = cache.full_memory + offsetMem;
        offsetMem += partition->size;
    }

    list_iterate(occupied_partitions, (void*)_asignPartitionOnBackUpMem);

    //- create big empty partition

    t_partition* emptySpacePartition = CreateNewPartition();
    emptySpacePartition->size = cache.memory_size - offsetMem;
    emptySpacePartition->begining = cache.full_memory + offsetMem;
    emptySpacePartition->queue_type = 0;
    emptySpacePartition->free = 1;


    list_add(occupied_partitions, emptySpacePartition);

    memcpy(cache.full_memory, backUp_memory, cache.memory_size);

    free(backUp_memory);

   // sem_wait(&mutex_partitions);
	list_remove_and_destroy_by_condition(partitions, (void*)_is_empty_partition, (void*)Free_Partition);
   // sem_post(&mutex_partitions);

	list_clean(partitions);
	free(partitions);

    partitions = occupied_partitions;

	log_info(cache_log, "SE EJECUTO COMPACTACION");
}

uint32_t add_occupied_size_from(t_list* occupied){
    uint32_t sum = 0;
    t_partition* partition;
    for (int i = 0; i < list_size(occupied); i++) {
        partition = (t_partition*) list_get(occupied, i);
        sum += partition->size;
    }
    return sum;
}


void delete_partition(void){
    t_partition* deletedPartition;
    if(victimSelectionIsFifo){ //compare dif algoritmos
    	deletedPartition = delete_partition_fifo();
    }else{
    	deletedPartition = delete_partition_lru();
    }

    log_info(cache_log, "PARTICION ELIMINADA CON POSICION DE INICIO: 0x%X", deletedPartition->begining - cache.full_memory);

    bool _message_to_delete(t_cachedMessage* message)
    {
        if(message->partitionId == deletedPartition->id) return true; else return false;
    }

   // sem_wait(&mutex_cached_messages);
    list_remove_and_destroy_by_condition(cached_messages, (void*)_message_to_delete, (void*)Free_CachedMessage);
    //sem_post(&mutex_cached_messages);

    start_consolidation_for(deletedPartition);
}

t_partition* delete_partition_fifo(void){

    bool _first_busy_partition(t_partition* partition)
    {
        if(partition->free == 0) return true; else return false;
    }

  //  sem_wait(&mutex_partitions);
    t_partition* partition = (t_partition*)list_find(partitions, (void*)_first_busy_partition);
  //  sem_post(&mutex_partitions);

    partition->free = 1;
    return partition;
}

t_partition* delete_partition_lru(void){
    t_partition* leastUsedPartition = NULL;
    void _get_least_used(t_partition* partition)
    {
    	if(partition->free) return;
        if(leastUsedPartition == NULL) leastUsedPartition = partition;
        else
        {
            if(clock() - leastUsedPartition->timestap < clock() -  partition->timestap) leastUsedPartition = partition;
        }
    }

    list_iterate(partitions, (void*)_get_least_used);
    leastUsedPartition->free = 1;

    return leastUsedPartition;
}

int GetBusyPartitionsCount()
{
    bool _filter_busy_parittion(t_partition* partition)
    {
        if(partition->free == 0) return true; else return false;
    }
   // sem_wait(&mutex_partitions);
    t_list* busyPartitions = list_filter(partitions, (void*)_filter_busy_parittion);
  //  sem_post(&mutex_partitions);
    int busyPartitionsCount = list_size(busyPartitions);
    list_clean(busyPartitions);
    return busyPartitionsCount;
}

void Free_CachedMessage(t_cachedMessage* message)
{
    void _free_sendOrAck(int* sendOrAck)
    {
        free(sendOrAck);
    }
    list_clean_and_destroy_elements(message->sent_to_subscribers, (void*)_free_sendOrAck);
    free(message->sent_to_subscribers);
    free(message);
}

void Free_Partition(t_partition* partition)
{
    free(partition);
}

int GetNewId()
{
    int result;
//	sem_wait(&mutex_nextPartitionId);
	result = nextPartitionId;
    nextPartitionId++;
    if(nextPartitionId == INT_MAX) nextPartitionId = 0;
//	sem_post(&mutex_nextPartitionId);
    return result;    
}

t_partition* CreateNewPartition()
{
    t_partition* partition = (t_partition*)malloc(sizeof(t_partition));
	partition->id = GetNewId();
	partition->timestap = clock();
	partition->parentId = 0;
	partition->free = 1;
	partition->begining = NULL;
    return partition;
}


t_list* GetMessagesFromQueue(message_type type)
{
    bool _message_by_queue(t_cachedMessage* message)
    {
        if(message->queue_type == type) return true; else return false;
    }
    return UpdateClockOn(list_filter(cached_messages, (void*)_message_by_queue));
}


t_list* UpdateClockOn(t_list* c_messages)
{
    t_cachedMessage* message;
    for(int index = 0; index < list_size(c_messages); index++)
    {
        message = (t_cachedMessage*)list_get(c_messages, index);
        UpdateClockOnMessage(message);
    }
    return c_messages;
}

t_cachedMessage* UpdateClockOnMessage(t_cachedMessage* message){
    if(message != NULL)
        UpdateTimestamp(message->partitionId);
    return message;
}

void UpdateTimestamp(uint32_t partitionId)
{
    t_partition* part;
    part = GetPartition(partitionId);
    part->timestap = clock();
}


t_cachedMessage* GetCachedMessage(int messageId)
{
    bool _message_by_id(t_cachedMessage* message)
    {
        if(message->id == messageId) return true; else return false;
    }
    //TODO ver que pasa si no encuentra el mensaje id en el casteo
    return UpdateClockOnMessage((t_cachedMessage*)list_find(cached_messages, (void*)_message_by_id));
}

t_cachedMessage* GetCachedMessageInPartition(int partitionId)
{
    bool _message_by_partition_id(t_cachedMessage* message)
    {
        if(message->partitionId == partitionId) return true; else return false;
    }
    return (t_cachedMessage*)list_find(cached_messages, (void*)_message_by_partition_id);
}

t_partition* GetPartition(int partitionId)
{
    bool _partition_by_id(t_partition* partition)
    {
        if(partition->id == partitionId) return true; else return false;
    }
    return (t_partition*)list_find(partitions, (void*)_partition_by_id);
}

t_list* GetAllMessagesForSuscriptor(int client, message_type queueType)
{
	sem_wait(&mutex_saving);
	bool _contains_client(int* id)
	{
		if(*id == client) return true; else return false;
	}

    bool _message_by_queue(t_cachedMessage* cachedMessage)
    {
        if(cachedMessage->queue_type == queueType && (list_find(cachedMessage->sent_to_subscribers, (void*)_contains_client) == NULL)) return true; else return false;
    }

    void* _get_deli_message(t_cachedMessage* cachedMessage)
    {
		deli_message* message = (deli_message*)malloc(sizeof(deli_message));
		message->id = cachedMessage->id;
		message->correlationId = cachedMessage->corelationId;
		message->messageType = cachedMessage->queue_type;
		message->messageContent =  DeserializeMessageContent(queueType, (GetPartition(cachedMessage->partitionId))->begining);
    	return message;
    }

    t_list* filteredMessages = list_filter(cached_messages, (void*)_message_by_queue);

    filteredMessages = UpdateClockOn(filteredMessages);

    sem_post(&mutex_saving);
    return list_map(filteredMessages, (void*)_get_deli_message);
}

void AddASentSubscriberToMessage(int messageId, int client)
{
	sem_wait(&mutex_saving);
	t_cachedMessage* cachedMessage = GetCachedMessage(messageId);
	if(cachedMessage == NULL)
	{
		sem_post(&mutex_saving);
		return;
	}
	//sem_wait(&cachedMessage->mutex_message);
	int* cachedClient = (int*)malloc(sizeof(int));
	*cachedClient = client;
	list_add(cachedMessage->sent_to_subscribers, cachedClient);
	cachedMessage->ack_by_subscribers_left++;
	//sem_post(&cachedMessage->mutex_message);
	sem_post(&mutex_saving);
}

void AddAcknowledgeToMessage(int messageId)
{
	sem_wait(&mutex_saving);
	t_cachedMessage* cachedMessage = GetCachedMessage(messageId);
	if(cachedMessage == NULL)
	{
		sem_post(&mutex_saving);
		return;
	}
	//sem_wait(&cachedMessage->mutex_message);
	cachedMessage->ack_by_subscribers_left--;
	//sem_post(&cachedMessage->mutex_message);
	sem_post(&mutex_saving);
}

deli_message* GetMessage(int messageId)
{
    //TODO despues de resolver lo de abajo ver si aca va sem_wait(&mutex_saving);
    sem_wait(&mutex_saving);
    t_cachedMessage* cachedMessage = GetCachedMessage(messageId);
    if(cachedMessage == NULL){
        sem_post(&mutex_saving);
        return NULL;
    }
    //TODO agregar analisis de cached message == NULL
    //TODO el GetCachedMessages ya updatea le timestamp, sacarlo de abajo no?
    t_partition* partition = GetPartition(cachedMessage->partitionId);
    if(partition->free)
    {
    	sem_post(&mutex_saving);
    	return NULL;
    }

	deli_message* message = (deli_message*)malloc(sizeof(deli_message));
	message->id = cachedMessage->id;
	message->correlationId = cachedMessage->corelationId;
	message->messageType = cachedMessage->queue_type;

	message->messageContent =  DeserializeMessageContent(partition->queue_type, partition->begining);
	sem_post(&mutex_saving);
    return message;
}



void PrintDumpOfCache()
{
   log_info(cache_log, "SE SOLICITO DUMP DE CACHE.");
   time_t rawtime;
   struct tm *info;
   time( &rawtime );
   info = localtime( &rawtime );

   FILE *fp;
   int myInt = 5;
   fp = fopen("CacheDump.txt", "a");
   fseek(fp, 0, SEEK_END);

   t_list* listToPrint = list_duplicate(partitions);

   bool _sort_by_position(t_partition* one, t_partition* two)
   {
	   return one->begining < two->begining;
   }

   list_sort(listToPrint, (void*)_sort_by_position);

    fprintf(fp,"\n------------------------------------------------------------------------\n");
    fprintf(fp,"Dump: %d/%d/%d %s\n", info->tm_mday, info->tm_mon, info->tm_year, temporal_get_string_time());

    void _print_partition_info(t_partition* partition)
    {
        char* busyStatus;
        char* memoryLocation[20];
        sprintf(memoryLocation, "0x%X - 0x%X",  partition->begining - cache.full_memory, partition->begining + partition->size - 1 - cache.full_memory);
        if(partition->free == 1)
        {
            busyStatus = "L";
            fprintf(fp,"Partición %d: %s. [%s] Size:%ub\n",
            	(int)partition->id,
				memoryLocation,
                busyStatus, 
                (uint32_t)partition->size,
				partition->parentId
            );

        } 
        else 
        {
            busyStatus = "X";
            t_cachedMessage* message = GetCachedMessageInPartition(partition->id);
            char* queue = GetStringFromMessageType(message->queue_type);
            fprintf(fp,"Partición %d: %s. [%s] Size:%ub LRU:%ld Cola:%s ID:%d\n",
                partition->id,
				memoryLocation,
                busyStatus,
                (uint32_t)partition->size,
                clock() - partition->timestap,
                queue,
                message->id
            );
        }
    }

    list_iterate(listToPrint, (void*)_print_partition_info);

    fprintf(fp,"------------------------------------------------------------------------\n");
    fclose(fp); //Don't forget to close the file when finished
    list_clean(listToPrint);
    free(listToPrint);
}

void start_consolidation_for(t_partition* freed_partition)
{
    if(!memorySchemeIsDynamic)
    {
        uint32_t partitionId = freed_partition->id;
        int result = 0;
        while(result != -1)
        {
            result = check_validations_and_consolidate_BS(&partitionId);
        }
     }
    else
    {
        if(freed_partition->size == cache.memory_size)
            return;
        check_validations_and_consolidate_PD(freed_partition);
     }
}

/**
 * This function gets the pointer to the freed partition an checks whether it's neighbours are free; if positive increments the size of the
 * first one and deletes the other from the partitions list.
 * @param freed_partition pointer
 * @return void
 */
void check_validations_and_consolidate_PD(t_partition* freed_partition){

    bool _is_left_neighbor(t_partition* partition){
        return (partition->begining + partition->size) == freed_partition->begining;
    }

    bool _is_right_neighbor(t_partition* partition){
        return (freed_partition->begining + freed_partition->size) == partition->begining;
    }
    
    int new_size = freed_partition->size;

    //Check if there is a free partition to our left
    t_partition* left_partition = freed_partition;
    if(freed_partition->begining != cache.full_memory)
    {
        sem_wait(&mutex_partitions);
        t_partition* tmp = (t_partition*) list_find(partitions, (void*)_is_left_neighbor);
        sem_post(&mutex_partitions);
        if(tmp->free)
        {
        	left_partition = tmp;
        	new_size += left_partition->size;
        }
    }

    //Check if there is a free partition to our right
    t_partition* right_partition = freed_partition;
    if((freed_partition->begining + freed_partition->size) < cache.full_memory + cache.memory_size)
    {
        sem_wait(&mutex_partitions);
        t_partition* tmp = (t_partition*) list_find(partitions, (void*)_is_right_neighbor);
        sem_post(&mutex_partitions);

        if(tmp->free)
        {
        	right_partition = tmp;
			new_size += right_partition->size;

        	//Delete the partition to the right
			sem_wait(&mutex_index_finder_destroyer);
			find_index_in_list_and_destroy(right_partition);
			sem_post(&mutex_index_finder_destroyer);
        }
    }

    //If there is a parition to the left, delete the one we deleted originaly and se this one as the new free partition
    if(left_partition != freed_partition)
    {
    	sem_wait(&mutex_index_finder_destroyer);
        find_index_in_list_and_destroy(freed_partition);
        sem_post(&mutex_index_finder_destroyer);
    }

    //If false, it means we did not create any new paritions
    if(left_partition->size < new_size) left_partition->size = new_size;
}

/**
 *
 * @param freed_partition_id
 * @return id of the parent that replaced both children to continue the recursion, if >= 0 then call again with the partition
 * id, if < 0 then break the cycle.
 */
int check_validations_and_consolidate_BS(uint32_t* freed_partition_id)
{

    bool _is_related_partition(t_partition* partition)
    {
        return ( partition->free
        		&& partition->id != bs_freed_partition->id
                && partition->size == bs_freed_partition->size
                && partition->parentId == bs_freed_partition->parentId);
    }

    bool _has_wanted_id(t_partition* partition){ return partition->id == *freed_partition_id;}

    sem_wait(&mutex_partitions);
    bs_freed_partition = (t_partition*) list_find(partitions, (void*) _has_wanted_id);
    sem_post(&mutex_partitions);
        
    if(bs_freed_partition->size >= cache.memory_size)
        return -1;


    sem_wait(&mutex_partitions);
    t_partition* related_partition = (t_partition*) list_find(partitions, (void*) _is_related_partition);
    sem_post(&mutex_partitions);
        
    if(related_partition == NULL)
        return -1;

    *freed_partition_id = consolidate(related_partition);

    return 0;
}

void find_index_in_list_and_destroy(t_partition* partition){
    sem_wait(&mutex_partitions);
    int index = find_index_in_list(partition);
        
    free((t_partition*) list_remove(partitions, index));
    sem_post(&mutex_partitions);
 }

int find_index_in_list(t_partition* partition)
{
    int index = 0;
    int wanted_id = partition->id;
    t_partition* aux_partition;
    while(index < list_size(partitions))
    {
        aux_partition = (t_partition*)list_get(partitions, index);
        if(aux_partition->id == wanted_id)
            break;
        index++;
    }
    return index;
}


int CalculateNearestPowerOfTwo(int x)
{
	double evenSize = x - (x % 2);
	return (int)floor(log10(evenSize) / log10(2));
}

uint32_t consolidate(t_partition* related_partition)
{

	uint32_t parentId = (*related_partition).parentId;

    bool _is_wanted_parent(t_partition* partition){ return(partition->id == parentId); }

    bool _is_child_partition(t_partition* partition)
    {
        return (partition->parentId == parentId);
    }

    log_info(cache_log, "SE ASOCIARON LOS BLOQUES CON COMIENZO EN: 0x%X y 0x%X",  bs_freed_partition->begining, related_partition->begining);

    sem_wait(&mutex_parent_partitions);
    t_partition* parent = (t_partition*) list_remove_by_condition(parent_partitions, (void*) _is_wanted_parent);
    sem_post(&mutex_parent_partitions);

    sem_wait(&mutex_partitions);
    list_remove_and_destroy_by_condition(partitions, (void*)_is_child_partition, (void*)Free_Partition);
    list_remove_and_destroy_by_condition(partitions, (void*)_is_child_partition, (void*)Free_Partition);
    list_add(partitions, parent);
    sem_post(&mutex_partitions);

    return parent->id;
}

t_partition* create_childrens_from(t_partition* parent){
    
	int newPartitionsSize = parent->size / 2;

    t_partition* one_children = CreateNewPartition();
    one_children->parentId = parent->id;
    one_children->begining = parent->begining;
    one_children->free = 1;
    one_children->size = newPartitionsSize;

    t_partition* another_children = CreateNewPartition();
    another_children->parentId = parent->id;
    another_children->begining = parent->begining + newPartitionsSize;
    another_children->free = 1;
    another_children->size = newPartitionsSize;

    sem_wait(&mutex_partitions);
    list_add(partitions, one_children);
    list_add(partitions, another_children);
    list_remove(partitions ,find_index_in_list(parent));
    sem_post(&mutex_partitions);

    sem_wait(&mutex_parent_partitions);
    list_add(parent_partitions, parent);
    sem_post(&mutex_parent_partitions);


    return one_children;
}

void destroy_cache()
{
	free(cache.full_memory);
	list_clean_and_destroy_elements(partitions,(void*)Free_Partition);
	list_clean_and_destroy_elements(parent_partitions,(void*)Free_Partition);
	list_clean_and_destroy_elements(cached_messages,(void*)Free_CachedMessage);
}






























