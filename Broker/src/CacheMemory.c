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
	first_parition.id = 1;
	first_parition.begining = cache.full_memory;
	first_parition.size = cache.memory_size;
	first_parition.queue_type = NULL;
	first_parition.free = 1;
	first_parition.timestap = clock();

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
	new_message.partitionId = save_message_body(message.messageContent, message.queue_type);
	add_to_cached_messages(new_message);

}

t_cachedMessage create_cached_from_message(deli_message message){
	t_cachedMessage new_message;
	new_message.id = m.id;
	new_message.corelationid = m.correlationId;
	new_message.queue_type = m.messageType;
	new_message.sent_to_subscribers = NULL;
	new_message.ack_by_subscribers = NULL;
	new_message.partitionId = 0;
	return new_message;
}

void add_to_cached_messages(t_cachedMessage new_message){
	list_add(cached_messages, new_message);
}
//todo refactor this with the new partition structure
int save_message_body(void* messageContent, queue_type queue){
	t_buffer* messageBuffer = SerializeMessageContent(queue, messageContent);	
	t_partition* partition = find_empty_partition_of_size(sizeof(t_buffer->bufferSize));
	return save_body_in_partition(messageBuffer, partition, queue);
}

void save_body_in_partition(t_buffer* messageBuffer, t_partition* partition, queue_type queue)
{

	if(partition->size == messageBuffer->bufferSize)
	{
		partition.queue_type = queue;
		partition.free = 0;
		partition.timestap = clock();

		nextPartitionId++;

		memcpy(partition->begining, messageBuffer->stream, sizeof(messageBuffer->bufferSize));
		return partitionid;
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

		partition.begining += newPartitionSize + 1;
		partition.timestap = clock();
		
		memcpy(partition->begining, messageBuffer->stream, sizeof(messageBuffer->bufferSize));
		return newPartition.id;
	}
	else
	{
		//TODO Handle buddy creation 
	}
}


t_partition* find_empty_partition_of_size(uint32_t size){
	if(size > cache.memory_size) //TODO decide what to do when message is too big
	t_partition* partition = select_partition(size);
	int compaction_frequency =  config_get_int_value(config, FRECUENCIA_COMPACTACION);
	if(partition != null) return partition;
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
	t_partition* partition = (t_parition*)list_find(partitions, (void*)_does_partition_fit);
	return partition;
}


char* select_partition_bf(uint32_t size){
	//TODO
	return 0;
}


void compact_memory(void){
	check_compact_restrictions();
	//TODO compact logic
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
	//TODO Remove cached message
}

void delete_partition_fifo(void){
	//TODO
}

int delete_partition_lru(void){
	t_partition* leastUsedPartition = NULL;
	void _get_least_used(t_partition* partition)
	{
		if(leastUsedPartition == NULL) leastUsedPartition = partition;
		else
		{
			if(clock() - leastUsedPartition->timestamp < clock() -  partition->timestamp) leastUsedPartition = partition;
		}
	}

	void list_iterate(t_list *, (void)leastUsedPartition);
	leastUsedPartition->free = 1;
	leastUsedPartition->timestamp = clock();

	return leastUsedPartition->id;
}

int GetBusyParitionsCount()
{
	bool _filter_busy_parittion(t_partition* partition)
	{
		if(partition->free == 0) return true; else return false;
	}
	t_list* busyPartitions = list_filter(paritions, (void*)_filter_busy_parittion);
	int busyPartitionsCount = list_size(filteredList);
	list_clean(busyPartitions);
	return busyPartitionsCount;
}