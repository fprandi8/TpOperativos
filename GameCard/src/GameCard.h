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

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<unistd.h>
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

typedef struct {
	t_log* logger;
} t_GameCard;


t_GameCard* GameCard_initialize(t_log*);

#endif /* GAMECARD_H_ */
