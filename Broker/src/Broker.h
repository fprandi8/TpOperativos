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
#include <signal.h>
#include <poll.h>
#include <stdbool.h>


typedef struct {
	message_type type;
	t_queue* queue;
	t_list* suscriptors;
	//t_list* messagesAdministrator;
	} t_queue_handler;
/*
typedef struct{
	uint32_t messageId;
	uint32_t amountPendingAcknowledge;
	sem_t semaphoreACK;
	}t_message_administrator;
*/
typedef struct {
	int suscripted;
	} t_suscriptor;

typedef struct {
	t_list * queues;
	sem_t semaphoreID;
	int servidor;
	t_log * logger;
	} t_Broker;

typedef struct {
	int cliente;
	t_Broker * broker;
	} t_args;

typedef struct {
	t_queue_handler* queue;
	deli_message* message;
	t_Broker* broker;
	int cliente;
	//t_message_administrator* messageAdministrator;
	} t_args_queue;


t_log* iniciar_logger(void);
t_config* leer_config(void);
char* get_config_value(t_config* , t_log* , char* );

void broker_initialize(t_Broker*, int, t_log*);
void broker_destroy(t_Broker*);
void broker_suscribe_process(void* buffer, int cliente, t_Broker* broker);
t_queue_handler* broker_get_specific_Queue(t_Broker , message_type );
void broker_get_acknowledge(void* , int , t_Broker* );
void broker_process_message(void* , int , t_Broker* );
void broker_assign_id(t_Broker* , deli_message* );


t_queue_handler* queue_handler_initialize(message_type );
void queue_handler_process_message(t_queue_handler* , deli_message*, t_Broker*);
void queue_handler_send_message(void* );

int destroy_queue_list(t_list*);
void destroy_queue_handler(t_queue_handler* );

t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* ,int );

void serve_client(void *);
void recive_message(uint32_t , int, t_Broker*, void*);
/*
t_message_administrator* message_administrator_initialize(uint32_t);
t_message_administrator* messege_administrator_get_administrator(t_list* , uint32_t);
void message_administrator_pending_acknowledge(t_message_administrator* );
void message_administrator_receive_acknowledge(t_message_administrator* );
uint32_t message_admnistrator(t_message_administrator*);*/

void RemoveClient(int client);



void signaltHandler(int);
void cacheSigHandler(int);
#endif /* BROKER_H_ */
