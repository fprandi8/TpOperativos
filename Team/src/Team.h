/*
 * Team.h
 *
 *  Created on: 19 abr. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include "utils.h"
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include <commons/collections/list.h>
#include <delibird/comms/pokeio.h>
#include <delibird/comms/messages.h>


typedef struct
{
	uint32_t x;
	uint32_t y;
} t_trainerPosition, t_pokemonPosition;



typedef struct
{
	char* name;
	t_pokemonPosition position;
} t_pokemon;

typedef struct
{
	t_pokemon* pokemons;
	uint32_t count;
} t_pokemonList;

typedef struct
{
	t_pokemon pokemon;
	uint32_t count;
} t_objetive;

typedef struct
{
	uint32_t* id;
	uint32_t count;
} t_idMessages;

typedef struct
{
	uint32_t id;
	t_trainer trainer;
} t_catchMessage;

typedef struct
{
	t_catchMessage* catchMessage;
	uint32_t count;
} t_catchMessages;

typedef struct
{
	t_trainerPosition position;
	t_pokemon* pokemons;
	uint32_t pokemonsCount;
	t_pokemon* objetives;
	uint32_t objetivesCount;
} t_trainerParameters;

typedef enum
{
	AVAILABLE = 1,
	DEADLOCK = 2,
	WAITING = 3
} t_blockState;

 typedef struct
{
	pthread_t trainer;
	t_trainerParameters parameters;
	sem_t semaphore;
	t_blockState blockState;

} t_trainer;



struct Broker
{
	char* ipKey;
	char* ip;
	char* portKey;
	char* port;
} broker;

struct SchedulingAlgorithm
{
	char* algorithmKey;
	char* algorithm;
	char* quantumKey;
	char* quantum;
	char* initEstimationKey;
	char* initEstimation;
} schedulingAlgorithm;

typedef struct {
	int suscription;
	uint32_t queueType;
	void* brokerAddress;
} t_args;

typedef struct {
	deli_message* message;
	void* brokerAddress;
} t_args_process_message;

typedef struct
{
	t_trainer* trainer;
	t_pokemon* pokemon;
} t_ready_trainers;

typedef struct
{
	t_trainer* new;
	t_trainer* blocked;
	t_ready_trainers* ready;
	t_ready_trainers* exec;
	t_trainer* exit;

} t_stateLists;


void createConfig(t_config**);
void startLogger(t_config*,t_log**);
void createLogger(char*,t_log**);
t_config* readConfig(void);
t_config* startConfig(void);
void deleteLogger(t_log**);
void removeLogger(char*);
void initBroker(struct Broker*);
void readConfigBrokerValues(t_config*,t_log*,struct Broker*);
void readConfigSchedulerValues(t_config*, t_log*, struct SchedulingAlgorithm*);
void readConfigTrainersValues(t_config*,t_log*,char***,char***,char***);
void initScheduler(struct SchedulingAlgorithm*);
void addToReady(t_ready_trainers* ,t_ready_trainers* ,int* ,struct SchedulingAlgorithm ,t_log* , t_ready_trainers* );
void addToExec(t_ready_trainers* ,int* ,t_ready_trainers* ,t_log* );
void scheduleFifo(t_ready_trainers*,int*,struct SchedulingAlgorithm,t_ready_trainers*,t_log*);
void scheduleRR(t_ready_trainers*,int*,struct SchedulingAlgorithm,t_ready_trainers*,t_log*);
void scheduleSJFSD(t_ready_trainers*,int*,struct SchedulingAlgorithm,t_ready_trainers*,t_log*);
void scheduleSJFCD(t_ready_trainers*,int*,struct SchedulingAlgorithm,t_ready_trainers*,t_log*);
void scheduleDistance(pthread_t**);
void startInitMatrix(char *);
void assignMatrixValues(void *);
void addToList(char *);
void initlist(char *);
int getListSize(t_list *);
void getMatrix(char**,char**);
int getTrainersCount(t_config*,t_log*);
void getTrainerAttr(char**,char**,char**,int,t_log*,t_trainer*);
void getTrainerAttrPos(char**,t_trainer*,int,t_log*);
void getTrainerAttrPkm(char**,t_trainer*,int,t_log*);
void getTrainerAttrObj(char**,t_trainer*,int,t_log*);
void startTrainers(t_trainer*,int,t_config*,t_log*);
void startTrainer(t_trainer*,t_log*);
void startThread(t_trainer*);
void missingPokemons(t_trainer*, int,t_log*);
void subscribeToBroker(struct Broker,pthread_t*);
void* subscribeToBrokerLocalized(void* Broker);
void* subscribeToBrokerAppeared(void* Broker);
void* subscribeToBrokerCaught(void* Broker);
int connectBroker(char*, char*,t_log*);
void requestNewPokemons(t_objetive*,int,t_log*,struct Broker);
void requestNewPokemon(t_pokemon,t_log*,struct Broker);
int getGlobalObjetivesCount(t_trainer*, int);
void initStateLists(t_stateLists,t_trainer*,t_trainer*,t_ready_trainers*,t_ready_trainers*,t_trainer*);
void waitForMessage(void*);
void processMessage(void*);
void processMessageLocalized(deli_message*);
void processMessageCaught(deli_message*);
void processMessageAppeared(deli_message*);
int findIdInGetList(uint32_t);
int findNameInAvailableList(char*);

void initBroker(struct Broker *broker){
	broker->ipKey="IP_BROKER";
	broker->portKey="PUERTO_BROKER";

}

void initScheduler(struct SchedulingAlgorithm *schedulingAlgorithm){
	schedulingAlgorithm->algorithmKey="ALGORITMO_PLANIFICACION";
	schedulingAlgorithm->quantumKey="QUANTUM";
	schedulingAlgorithm->initEstimationKey="ESTIMACION_INICIAL";
}

void moveTrainerToTarget(t_trainer* trainer, int distanceToMoveInX, int distanceToMoveInY);
void moveTrainerToObjective(t_trainer* trainer,  t_pokemon* pokemonTargeted);
int calculateDifference(int, int);
int getDistanceToPokemonTarget(t_trainerParameters, appeared_pokemon*);
void moveTrainetToObjective(t_trainer*, t_pokemon*);
int executeClock(int, t_ready_trainers*);

#endif /* TEAM_H_ */
