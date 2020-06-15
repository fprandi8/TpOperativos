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
#include<commons/collections/queue.h>
#include<commons/collections/list.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
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
} t_GameCard;

typedef enum{
	METADATA,
	POKE_METADATA,
	POKE_FILE,
	BITMAP
} fileType;

typedef struct {
	t_list* values;
}t_values;


t_GameCard* GameCard_initialize(t_log*, char*);
int create_directory(char*);
int create_file(char*, fileType, t_values*);

#endif /* GAMECARD_H_ */
