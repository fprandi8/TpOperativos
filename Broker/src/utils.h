/*
 * conexiones.h
 *
 *  Created on: 3 mar. 2019
 *      Author: utnso
 */

#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<pthread.h>
#include<commons/config.h>
#include <semaphore.h>
#include "delibird/comms/messages.h"

typedef struct {
	int* cliente;
	sem_t* mutex;
	t_list * queues;
	} t_args;


void* recibir_buffer(int*, int);

int iniciar_servidor(char*, char*);
int esperar_cliente(int);
int recibir_operacion(int);
void process_request(int cod_op, int cliente_fd);


#endif /* CONEXIONES_H_ */
