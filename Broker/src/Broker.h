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

typedef enum{
	NEW_POKEMON,
	APPEARED_POKEMON,
	CATCH_POKEMON,
	CAUGHT_POKEMON,
	GET_POKEMON,
	LOCALIZED_POKEMON,
	} t_queue_type;

typedef struct {
	t_queue_type tipo;
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
t_queue_handler* inicializar_queue_handler(t_queue_type );
int destroy_queue_list(t_list*);
void destroy_queue_handler(t_queue_handler* );
t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* ,int );
void suscribir_proceso(t_queue_handler* , int );

#endif /* BROKER_H_ */
