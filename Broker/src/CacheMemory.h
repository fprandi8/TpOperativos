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
#include <stdbool.h>
#include<unistd.h>
#include <time.h>
#include<commons/log.h>
#include<commons/collections/queue.h>
#include<commons/collections/list.h>
#include "utils.h"
#include "delibird/comms/messages.h"
#include "delibird/comms/serialization.h"
#include <limits.h>
#include <semaphore.h>
#include <signal.h>
#include <string.h>
#include <math.h>

/*
	******************STRUCTS***********************
*/
typedef struct{
    char* full_memory;
    uint32_t partition_minimum_size;
    uint32_t memory_size;
}t_CacheMemory;

typedef struct{
    //partition number would be generated when iterating the list
    uint32_t id; //its message id
    char* begining;
    uint32_t size;
    message_type queue_type;
    bool free;
    clock_t timestap; //use clock() get a stamp
    uint32_t parentId; // indicates parent Id for buddy system
}t_partition;

typedef struct{
    uint32_t id;
    uint32_t corelationId;
    message_type queue_type;
    t_list* sent_to_subscribers;
    t_list* ack_by_subscribers;
    uint32_t partitionId;
}t_cachedMessage;

/*
	*******************GLOBAL VARIABLES*****************
*/
t_CacheMemory cache;
t_config* config;
sem_t* mutex_cached_messages;
t_list* cached_messages;
sem_t* mutex_partitions;
t_list* partitions;
sem_t* mutex_nextPartitionId;
int nextPartitionId;
t_list* parent_partitions;
t_partition* bs_freed_partition;

/*
	********************CONTRACTS***********************
*/

void start_cache(void);

//size configuration
t_config* get_config(void);
void define_partition_size(void);
void define_cache_maximum_size(void);
//memory reservation
void set_full_memory(void);
//message handler
void save_message(deli_message message);
t_buffer* get_message_body(); //implement if needed
int save_message_body(void*, message_type);
//partition handlers
t_partition* find_empty_partition_of_size(uint32_t size);
uint32_t save_body_in_partition(t_buffer*, t_partition*, message_type); //check if this is needed
void compact_memory(void);
void check_compact_restrictions(void);
void delete_partition(void);
t_partition* select_partition(uint32_t size);
t_partition* select_partition_ff(uint32_t size); //select by first fit
t_partition* select_partition_bf(uint32_t size); //select by best fit
int delete_partition_fifo(void); //delete by fifo
int delete_partition_lru(void); //delete by lru
t_list* GetMessagesFromQueue(message_type queue_type);
t_cachedMessage* GetCachedMessage(int);
void* GetMessageContent(int);
t_partition* CreateNewPartition();
t_cachedMessage create_cached_from_message(deli_message);
void Free_CachedMessage(t_cachedMessage*);
void add_to_cached_messages(t_cachedMessage);
uint32_t add_occupied_size_from(t_list* occupied);
int GetBusyPartitionsCount();
double CalculateNearestPowerOfTwo(int x);
double CalculateNearestPowerOfTwoRelativeToCache(int memoryLocation);

#endif /* CACHEMEMORY_H_ */
