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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <commons/log.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>
#include <commons/memory.h>
#include "commons/temporal.h"
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
    void* full_memory;
    uint32_t partition_minimum_size;
    uint32_t memory_size;
}t_CacheMemory;

typedef struct{
    //partition number would be generated when iterating the list
    uint32_t id; //its message id
    void* begining;
    uint32_t size;
    message_type queue_type;
    bool free;
    int timestap; //use clock() get a stamp
    uint32_t parentId; // indicates parent Id for buddy system
}t_partition;

typedef struct{
    uint32_t id;
    uint32_t corelationId;
    message_type queue_type;
    t_list* sent_to_subscribers;
    int ack_by_subscribers_left;
    uint32_t partitionId;
    sem_t mutex_message;
}t_cachedMessage;


/*
	*******************GLOBAL VARIABLES*****************
*/
t_log* cache_log;
t_CacheMemory cache;
t_config* config;

t_list* cached_messages;
t_list* partitions;
t_list* parent_partitions;
t_partition* bs_freed_partition;

int nextPartitionId;

int memorySchemeIsDynamic;
int victimSelectionIsFifo;
int partitionSelectionIsFirstFit;

sem_t mutex_cached_messages;
sem_t mutex_nextPartitionId;
sem_t mutex_partitions;
sem_t mutex_parent_partitions;
sem_t mutex_index_finder;
sem_t mutex_index_finder_destroyer;
sem_t mutex_occupied_partitions;
sem_t mutex_saving;
/*
	********************CONTRACTS***********************
*/

void start_cache();
void destroy_cache();

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
int find_index_in_list(t_partition* partition);
uint32_t consolidate(t_partition* related_partition);
void find_index_in_list_and_destroy(t_partition* partition);
void check_compact_restrictions(void);
t_cachedMessage* create_cached_from_message(deli_message message);
void add_to_cached_messages(t_cachedMessage* new_message);
t_partition* create_childrens_from(t_partition* parent);

//delete a partition
void delete_partition(void);
t_partition* select_partition(uint32_t size);
t_partition* select_partition_ff(uint32_t size); //select by first fit
t_partition* select_partition_bf(uint32_t size); //select by best fit
t_partition* delete_partition_fifo(void); //delete by fifo
t_partition* delete_partition_lru(void); //delete by lru
void find_index_in_list_and_destroy(t_partition* partition);

//Access methods
t_list* GetMessagesFromQueue(message_type queue_type);
t_cachedMessage* GetCachedMessage(int);
deli_message* GetMessage(int);
t_partition* CreateNewPartition();
int GetBusyPartitionsCount();
void PrintDumpOfCache();
void AddASentSubscriberToMessage(int messageId, int client);
void AddAcknowledgeToMessage(int messageId);
t_partition* GetPartition(int partitionId);
t_list* GetAllMessagesForSuscriptor(int client, message_type queueType);

//Free's
void Free_Partition(t_partition* partition);
void Free_CachedMessage(t_cachedMessage*);


//consolidation and compaction
uint32_t add_occupied_size_from(t_list* occupied);
void start_consolidation_for(t_partition* freed_partition);
void check_validations_and_consolidate_PD(t_partition* freed_partition);
int check_validations_and_consolidate_BS(uint32_t* freed_partition_id);

//misc
t_list* UpdateClockOn(t_list* c_messages);
t_cachedMessage* UpdateClockOnMessage(t_cachedMessage* message);
void UpdateTimestamp(uint32_t partitionId);
int CalculateNearestPowerOfTwo(int x);

#endif /* CACHEMEMORY_H_ */
