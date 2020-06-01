/*
 * MemoriaCache.h
 *
 *  Created on: 30 may. 2020
 *      Author: utnso
 */

#ifndef CACHEMEMORY_H_
#define CACHEMEMORY_H_

typedef struct {
	char* full_memory;
}t_CacheMemory;

//size configuration
void define_sizes(void);
void define_partition_size(void);
void define_cache_maximum_size(void);
//memory reservation
void set_full_memory(void);
//partition handlers
void save_message_body(t_buffer* messageBuffer); //see if we need to return a value
t_buffer* get_message_body(); //define params
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
