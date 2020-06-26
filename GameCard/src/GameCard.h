/*
 * GameCard.h
 *
 *  Created on: 14 jun. 2020
 *      Author: utnso
 */

#ifndef GAMECARD_H_
#define GAMECARD_H_
#define TIEMPO_DE_REINTENTO_CONEXION "TIEMPO_DE_REINTENTO_CONEXION"
#define TIEMPO_DE_REINTENTO_OPERACION "TIEMPO_DE_REINTENTO_OPERACION"
#define TIEMPO_RETARDO_OPERACION "TIEMPO_RETARDO_OPERACION"
#define PUNTO_MONTAJE_TALLGRASS "PUNTO_MONTAJE_TALLGRASS"
#define IP_BROKER "IP_BROKER"
#define PUERTO_BROKER "PUERTO_BROKER"
#define BLOCK_SIZE "BLOCK_SIZE"
#define	BLOCKS "BLOCKS"
#define	MAGIC_NUMBER "MAGIC_NUMBER"
#define DIRECTORY "DIRECTORY"
#define	SIZE "SIZE"
#define	OPEN "OPEN"

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
#include<string.h>
#include "utils.h"
#include<commons/log.h>
#include<commons/bitarray.h>
#include<commons/string.h>
#include<commons/collections/list.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "delibird/comms/messages.h"
#include "delibird/comms/serialization.h"
#include "delibird/comms/pokeio.h"
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
	t_log* logger;
	char* ptoMnt;
	char* filePath;
	char* metadataPath;
	int blocks;
	int block_size;
	char* blocksPath;
	t_bitarray* bitArray;
	char* fileMapped;
} t_GameCard;

struct Broker
{
	char* ipKey;
	char* ip;
	char* portKey;
	char* port;
} broker;

typedef enum{
	METADATA,
	METADATA_DIRECTORY,
	BIN_FILE,
	BITMAP,
	POKE_METADATA
} fileType;

typedef struct{
	char directory;
	char* size;
	t_list* block;
	char open;

}t_file_metadata;

typedef struct {
	t_list* values;
}t_values;


t_GameCard* GameCard_initialize(t_log*, char*);
int GameCard_mountFS(t_config*);
void GameCard_Process_Message(deli_message*);
void GameCard_Process_Message_New(deli_message*);
void GameCard_Initialize_bitarray();

int create_directory(char*);
int create_file(fileType, t_values*);
int create_file_bitmap();
int create_file_bin(t_values*);
int create_file_metadata(t_values* );
int create_file_metadata_directory(t_values* );
int create_file_metadata_poke(t_values*);

int create_poke_file(t_values*);
int modify_poke_file(t_values*, char*);

void read_metadata_file(t_file_metadata*, t_config*);
char* get_file_content(t_file_metadata*);

void turn_a_set_of_bits_on(int,int);
int get_first_free_block();
void write_blocks(t_file_metadata*, int, new_pokemon*);

void readConfigBrokerValues(t_config*,t_log* ,struct Broker*);
void subscribeToBroker(struct Broker broker,pthread_t* subs);
void* subscribeToBrokerNew(void *brokerAdress);
void* subscribeToBrokerCatch(void *brokerAdress);
void* subscribeToBrokerGet(void *brokerAdress);

void initBroker(struct Broker *broker){
	broker->ipKey=IP_BROKER;
	broker->portKey=PUERTO_BROKER;

}

int connectBroker(char* , char* ,t_log* );

#endif /* GAMECARD_H_ */
