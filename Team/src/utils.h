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
	MENSAJE=1,//Handshake
	GET=2, //hacia el broker
	LOCALIZED=3,//desde el broker
	APPEARED=4, //desde el gameboy y broker
	CATCH=5, //hacia el broker
	CAUGHT=6//desde el broker
}op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	int x;
	int y;
} t_trainerPosition, t_pokemonPosition;



typedef struct
{
	char** name;
	t_pokemonPosition* position;
} t_pokemon;


typedef struct
{
	t_trainerPosition* position;
	t_pokemon** pokemons;
	t_pokemon** objetives;
} t_trainerParameters;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;

//TODO
typedef struct
{

} t_map;


int iniciar_cliente(char*, char*,t_log*);
void* serializar_paquete(t_paquete*, int);
void enviar_mensaje(void*, int, int);
void* recibir_mensaje(int, int*,t_log*);
void serve_client(int *socket,t_log*);
void process_request(int cod_op, int cliente_fd,t_log*);
void startInitMatrix(char *);
void assignMatrixValues(void *);
void addToList(char *);
void initlist(char *);
int getListSize(t_list *);
void getMatrix(char**,char**);
int getTrainersCount(char**,t_log*);
void getTrainerAttr(char**,char**,char**,t_trainerParameters**,int,t_log*);
void getTrainerAttrPos(char**,t_trainerParameters**,int,t_log*);
void getTrainerAttrPkm(char**,t_trainerParameters**,int,t_log*);
void getTrainerAttrObj(char**,t_trainerParameters**,int,t_log*);
void startTrainers(pthread_t*,t_trainerParameters**,int);
void startTrainer(pthread_t,t_trainerParameters*);
void startThread(t_trainerParameters*);
#endif /* UTILS_H_ */
