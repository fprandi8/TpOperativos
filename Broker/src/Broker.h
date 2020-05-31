/*
 * Broker.h
 *
 *  Created on: 19 abr. 2020
 *      Author: utnso
 */

#ifndef BROKER_H_
#define BROKER_H_


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
#include "CacheMemory.h"
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "delibird/comms/messages.h"
#include "delibird/comms/serialization.h"
#include "delibird/comms/pokeio.h"


typedef struct {
	message_type type;
	t_queue* queue;
	t_list* suscriptors;
	} t_queue_handler;

typedef struct {
	int suscripted;
	} t_suscriptor;

typedef struct {
	t_list * queues;
	t_CacheMemory* cacheMemory;
	} t_Broker;

typedef struct {
	int* cliente;
	t_Broker * broker;
	} t_args;



t_log* iniciar_logger(void);
t_config* leer_config(void);
char* obtener_valor_config(t_config* , t_log* , char* );

void Broker_initialize(t_Broker*);
void Broker_destroy(t_Broker*);
void Broker_Suscribe_Process(t_buffer* buffer, int cliente, t_Broker* broker);
t_queue_handler* Broker_Get_Specific_Queue(t_Broker , message_type );
void Broker_Get_Acknowledge(t_buffer* , int , t_Broker* );
void Broker_Process_Message(t_buffer* , int , t_Broker* );


t_queue_handler* queue_handler_initialize(message_type );
void queue_handler_process_message(t_queue_handler* , deli_message*);

int destroy_queue_list(t_list*);
void destroy_queue_handler(t_queue_handler* );

t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* ,int );

void serve_client(void *);
void recive_message(t_package* , int, t_Broker*);


#endif /* BROKER_H_ */
