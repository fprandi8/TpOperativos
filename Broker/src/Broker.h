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
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "delibird/comms/messages.h"
#include "delibird/comms/serialization.h"
#include "delibird/comms/pokeio.h"


typedef struct {
	message_type tipo;
	t_queue* queue;
	t_list* suscriptores;
	} t_queue_handler;

typedef struct {
	int suscripto;
	} t_suscriptor;


t_log* iniciar_logger(void);
t_config* leer_config(void);
char* obtener_valor_config(t_config* , t_log* , char* );
t_list* inicializar_queues();
t_queue_handler* inicializar_queue_handler(message_type );
int destroy_queue_list(t_list*);
void destroy_queue_handler(t_queue_handler* );
t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* ,int );
void suscribir_proceso(t_queue_handler* , int );
void serve_client(void *);
void recibir_mensaje(t_package*, int, t_list*);
void procesar_mensaje(t_buffer*, t_list*);
void procesar_suscripcion(t_buffer*, int, t_list*);
void recibir_acknowledge(t_buffer*);


#endif /* BROKER_H_ */
