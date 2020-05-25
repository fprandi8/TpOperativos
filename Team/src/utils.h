/*
 * utils.h
 *
 *  Created on: 19 abr. 2020
 *      Author: utnso
 */

#ifndef UTILS_H_
#define UTILS_H_

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

typedef enum
{
	MENSAJE=1
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

pthread_t thread;
int iniciar_cliente(char*, char*,t_log*);
void* serializar_paquete(t_paquete*, int);
void enviar_mensaje(void*, int, int);
void* recibir_mensaje(int, int*,t_log*);
void serve_client(int *socket,t_log*);
void process_request(int cod_op, int cliente_fd,t_log*);

#endif /* UTILS_H_ */
