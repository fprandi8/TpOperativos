/*
 * Broker.h
 *
 *  Created on: 19 abr. 2020
 *      Author: utnso
 */

#ifndef BROKER_H_
#define BROKER_H_

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<commons/log.h>
#include<commons/collections/queue.h>
#include<commons/collections/list.h>
#include "utils.h"




typedef struct {
	char* nombre;
	t_queue* queue;
	t_list* suscriptores;
	} t_queue_handler;

typedef struct {
	int tipo_suscriptor;
	int suscripto;
	} t_suscriptor;


t_log* iniciar_logger(void);
t_config* leer_config(void);

t_queue_handler* inicializar_queue_handler(char* nombre);
void destroy_queue_handler(t_queue_handler* self);
t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* self,int pos);

#endif /* BROKER_H_ */
