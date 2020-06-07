/*
 * MemoriaCache.h
 *
 *  Created on: 30 may. 2020
 *      Author: utnso
 */

#ifndef CACHEMEMORY_H_
#define CACHEMEMORY_H_
#define TAMANO_MEMORIA "TAMANO_MEMORIA"
#define TAMANO_MINIMO_PARTICION "TAMANO_MINIMO_PARTICION"
#define ALGORITMO_MEMORIA "ALGORITMO_MEMORIA"
#define ALGORITMO_REEMPLAZO "ALGORITMO_REEMPLAZO"
#define ALGORITMO_PARTICION_LIBRE "ALGORITMO_PARTICION_LIBRE"
#define IP_BROKER "IP_BROKER"
#define PUERTO_BROKER "PUERTO_BROKER"
#define FRECUENCIA_COMPACTACION "FRECUENCIA_COMPACTACION"

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<commons/log.h>
#include<commons/collections/queue.h>
#include<commons/collections/list.h>
#include "utils.h"

typedef struct t_CacheMemory{
	char *full_memory;
	int partition_minimum_size;
	int memory_size;
}t_CacheMemory;

//size configuration
t_config* get_config(t_config* config);
void define_partition_size(void);
void define_cache_maximum_size(void);
//memory reservation
void set_full_memory(void);
//message handler
void save_message(t_message message);
t_buffer* get_message_body(); //implement if needed
void save_message_body(t_buffer* messageBuffer); //see if we need to return a value
//partition handlers
char* find_empty_partition_of_size(int size);
void save_body_in_partition(t_buffer* messageBuffer, char* position); //check if this is needed
void compact_memory(void);
void check_compact_restrictions(void);
void delete_partition(void);
char* select_partition(int size);
char* select_partition_ff(int size); //select by first fit
char* select_partition_bf(int size); //select by best fit
void delete_partition_fifo(void); //delete by fifo
void delete_partition_lru(void); //delete by lru



#endif /* CACHEMEMORY_H_ */
