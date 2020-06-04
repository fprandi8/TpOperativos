/*
 * MemoriaCache.c
 *
 *  Created on: 30 may. 2020
 *      Author: utnso
 */

#include "CacheMemory.h"

int main(void)
{
	//Receives config from Broker
	t_config* config = get_config();
	cache cache;
	define_partition_size();
	define_cache_maximum_size();
	set_full_memory();

	return 0;
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
	cache.full_memory = (char) malloc(sizeof(cache.memory_size));
}

void save_message(t_message message){
	//see what to do with the rest of the message's information
	save_message_body(message.messageBuffer);

}

void save_message_body(t_buffer* messageBuffer){
	char *partition = find_empty_partition_of_size(sizeof messageBuffer);
	save_body_in_partition(messageBuffer, partition);
}

char* find_empty_partition_of_size(int size){
	char *partition = select_partition(size);
	if(false)//TODO partition found logic
		compact_memory(void);
	if(false)//TODO check compact logic
		delete_partition(void);
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
	check_compact_restrictions(void);
	//TODO compact logic
}

void check_compact_restrictions(void){
	//TODO
}

void delete_partition(void){
	if(ALGORITMO_REEMPLAZO){//compare dif algoritmos
		delete_partition_fifo(void);
	}else{
		delete_partition_lru(void);
	}
}

void delete_partition_fifo(void){
	//TODO
}

void delete_partition_lru(void){
	//TODO
}
