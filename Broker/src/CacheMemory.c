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


	int i; // random variable to use for breakpoints
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
	cache.full_memory = (char) malloc(cache.memory_size);
}

void save_message(t_message message){
	//see what to do with the rest of the message's information
	save_message_body(message.messageBuffer);

}

void save_message_body(t_buffer* messageBuffer){
	char *partition = find_empty_partition_of_size(sizeof messageBuffer);
	save_body_in_partition(messageBuffer, partition);
}

void save_body_in_partition(t_buffer* messageBuffer,char *partition){

}


char* find_empty_partition_of_size(int size){
	char *partition = select_partition(size);
	if(0)//TODO partition found logic
		compact_memory();
	if(0)//TODO check compact logic
		delete_partition();
}

char* select_partition(int size){
	char *partition;
	if(ALGORITMO_PARTICION_LIBRE){ //compare dif algoritmos
		partition = select_partition_ff(size);
	}else{
		partition = select_partition_bf(size);
	}
	return partition;
}

char* select_partition_ff(int size){
	//TODO
	return 0;
}

char* select_partition_bf(int size){
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
	if(ALGORITMO_REEMPLAZO){//compare dif algoritmos
		delete_partition_fifo();
	}else{
		delete_partition_lru();
	}
}

void delete_partition_fifo(void){
	//TODO
}

void delete_partition_lru(void){
	//TODO
}
