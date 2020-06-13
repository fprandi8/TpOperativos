/*
 * MemoriaCache.c
 *
 *  Created on: 30 may. 2020
 *      Author: utnso
 */

#include "CacheMemory.h"

void start_cache(void)
{
	//Receives config from Broker
	config = get_config();
	define_partition_size();
	printf("Partition size %d: ", cache.partition_minimum_size);
	define_cache_maximum_size();
	printf("memory size %d: ", cache.memory_size);
	set_full_memory();
	printf("Memory starting at %d: ", &cache.full_memory);


	t_partition first_partition;
	first_partition.id = 1;
	first_partition.begining = cache.full_memory;
	first_partition.size = cache.memory_size;
	first_partition.queue_type = NULL;
	first_partition.free = 1;
	first_partition.timestap = clock();

	list_add(partitions, first_partition);

	nextPartitionId++; //TODO create a function to wrap the id asignation, so that we only modify it in one place

	uint32_t i; // random variable to use for breakpoints
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
	cache.full_memory = (char) malloc(cache.memory_size * sizeof(char));
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
	list_add(cached_messages, new_message);
}

int save_message_body(void* messageContent, message_type queue){
	t_buffer* messageBuffer = SerializeMessageContent(queue, messageContent);
	t_partition* partition = find_empty_partition_of_size(sizeof(messageBuffer->bufferSize));
	return save_body_in_partition(messageBuffer, partition, queue);
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

	if(config_get_string_value(config, ALGORITMO_MEMORIA) == "DYNAMIC")
	{
		int newPartitionSize = config_get_int_value(config, TAMANO_MINIMO_PARTICION);
		if(messageBuffer->bufferSize > newPartitionSize) newPartitionSize = messageBuffer->bufferSize;

		t_partition newPartition;
		newPartition.id = nextPartitionId;
		newPartition.begining = partition->begining;
		newPartition.size = newPartitionSize;
		newPartition.queue_type = queue;
		newPartition.free = 0;
		newPartition.timestap = clock();

		nextPartitionId++;

		list_add(partitions, newPartition);

		partition->begining += newPartitionSize + 1;
		partition->timestap = clock();

		memcpy(partition->begining, messageBuffer->stream, sizeof(messageBuffer->bufferSize));
		return newPartition.id;
	}
	else
	{
		//TODO Handle buddy creation
		uint32_t var;
		return var;
	}
}


t_partition* find_empty_partition_of_size(uint32_t size){
//	if(size > cache.memory_size); //TODO decide what to do when message is too big
	t_partition* partition = select_partition(size);
	int compaction_frequency =  config_get_int_value(config, FRECUENCIA_COMPACTACION);
	if(partition != NULL) return partition;
	if(compaction_frequency == -1)
	{
		int busyPartitions = 0;
		do
		{
			delete_partition();
			busyPartitions = GetBusyParitionsCount();
			partition = select_partition(size);
		}
		while(partition == NULL && busyPartitions > 0);
		if(partition == NULL)
		{
			compact_memory();
			partition = select_partition(size);
			return partition;
		}
	}
	else if(compaction_frequency > 0)
	{
		//TODO maybe add a validator for config at the start so compaction_frequency == 0 or < -1 cannot be a case
		do
		{
			if(partition == NULL)
			{
				compact_memory();
				partition = select_partition(size);
			}
			if(partition == NULL)
			{
				for(int i = 0; i < compaction_frequency; i++)
				{
					delete_partition(); //TODO we need to know we run out of partitions to delete
					partition = select_partition(size);
				}
			}
		} while(partition == NULL);
		return partition;
	}
}

char* select_partition(uint32_t size){
	char *partition;
	if(config_get_string_value(config, ALGORITMO_REEMPLAZO) == "FF"){ //compare dif algoritmos
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


char* select_partition_bf(uint32_t size){

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

	t_partition first_empty_partition = list_find(partitions, (void *)_is_empty_partition);
    t_list* occupied_partitions = list_filter(partitions, (void *) _filter_busy_partition);

	/*t_CacheMemory newCache;
	newCache.partition_minimum_size = cache.partition_minimum_size;
	newCache.memory_size = cache.memory_size;
	newCache.full_memory = (char) malloc(cache.memory_size * sizeof(char));
*/
	char* backUp_memory = (char) malloc(cache.memory_size * sizeof(char));//v2

	//For each occupied partition, copy from the cache to the new memory, and re-asign begining of cache
	/*int offset = 0;
	void _asignPartitionOnNewCache(t_partition* partition)
	{
		memcpy(newCache.full_memory + offset, partition->begining, partition->size);
		partition->begining = cache.full_memory + offset;
		offset += partition->size;
	}*/

	int offsetMem = 0;//v2
    void _asignPartitionOnBackUpMem(t_partition* partition)//v2
	{
		memcpy(backUp_memory + offsetMem, partition->begining, partition->size);
		offsetMem += partition->size;
	}

	int offsetPointerMem = 0;//v2
    void _reasignPartitionPointers(t_partition* partition)//v2
	{
        partition->begining = cache.full_memory + offsetPointerMem;
        offsetPointerMem += partition->size;
	}


    //- create big empty paritition
    uint32_t occupied_size = add_occupied_size_from(occupied_partitions);

    t_partition* emptySpacePartition = CreateNewPartition();
    emptySpacePartition->begining = NULL;
    emptySpacePartition->size = cache.memory_size - occupied_size;
    emptySpacePartition->queue_type = NULL;
    emptySpacePartition->free = true;
    emptySpacePartition->timestap = clock();

    list_add(occupied_partitions, emptySpacePartition);

    /*list_iterate(occupied_partitions, (void*)_asignPartitionOnNewCache);
	free(cache.full_memory); //The moment of truth
*/
    list_iterate(occupied_partitions, (void*)_asignPartitionOnBackUpMem);//v2
    memcpy(cache.full_memory, backUp_memory, sizeof(backUp_memory));//v2
    list_iterate(occupied_partitions, (void*)_reasignPartitionPointers);//v2
    free(backUp_memory);
    //TODO delete all empty partitions
    // Marquitos, te dejo esto aca, se me ocurrio una nueva version sobre la marcha no se que te parece, es lo que
    // tiene el coment 'v2', la idea es copiar en una memoria auxiliar, hacer un memcopy a la memoria principal y reubicar
    // los punteros cosas de que se pise lo viejo. Fijate que te parece; de esta forma la memoria reservada de la cache
    // siempre va a estar apuntando al mismo espacio en memoria

   /* cache = newCache; //TODO consultar si necesitamos que siempre apunte al mismo espacio de memoria la cache original o tirar boludos
*/
    partitions = occupied_partitions;
    //TODO add times compacting here or outside?
}

uint32_t add_occupied_size_from(t_list* list){
    uint32_t sum = 0;
    for (int i = 0; i < sizeof(list); ++i) {
        t_partition partition = list_get(list, i);
        sum += partition.size;
    }
    return sum;
}

void check_compact_restrictions(void){
	//TODO
}

void delete_partition(void){
	int deletedParitionId;
	if(config_get_string_value(config, ALGORITMO_REEMPLAZO) == "FIFO"){//compare dif algoritmos
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

	list_iterate(partitions, (void)_get_least_used);
	leastUsedPartition->free = 1;
	leastUsedPartition->timestap = clock();

	return leastUsedPartition->id;
}

int GetBusyParitionsCount()
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
	t_partition partition;
	partition.id = nextPartitionId;
	nextPartitionId++;
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

t_cachedMessage* GetMessage(int messageId)
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
	t_cachedMessage* message = GetMessage(messageId);
	t_partition* partition = GetPartition(message->partitionId);
	return DeserializeMessageContent(partition->queue_type, partition->begining);
}
