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
	t_trainerPosition position;
	t_pokemon* pokemons;
	int pokemonsCount;
	t_pokemon* objetives;
	uint32_t objetivesCount;
	uint32_t previousBurst;
	float previousEstimate;
	t_pokemon scheduledPokemon;//TODO: inicializar en "NULL"
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
	uint32_t id;

} t_trainer;

typedef struct
{
	uint32_t id;
	int blockPos;
} t_catchMessage;

typedef struct
{
	t_catchMessage* catchMessage;
	uint32_t count;
} t_catchMessages;

struct Broker
{
	char* ipKey;
	char* ip;
	char* portKey;
	char* port;
} broker,teamServerAttr;

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

typedef struct t_trainerList
{
	t_trainer* trainerList;
	uint32_t count;
} t_trainerList;

typedef struct t_trainerExec{
	t_trainer trainer;
	uint32_t boolean;
} t_execTrainer;

struct t_stateLists
{
	t_trainerList newList;
	t_trainerList blockedList;
	t_trainerList readyList;
	t_execTrainer execTrainer;
	t_trainerList exitList;

} statesLists;

typedef struct t_list_of_objectives{
	t_pokemon* pokemon;
	uint32_t* bit;
} t_list_of_objectives_validation;


void createConfig(t_config**);
void startLogger(t_config*);
void createLogger(char*);
t_config* readConfig(void);
t_config* startConfig(void);
void deleteLogger();
void removeLogger(char*);
void initBroker(struct Broker*);
//void initTeam_stateListsServer(struct Broker*);
void readConfigBrokerValues(t_config*,struct Broker*);
void readConfigSchedulerValues(t_config*, struct SchedulingAlgorithm*);
void readConfigTrainersValues(t_config*,char***,char***,char***);
void initScheduler(struct SchedulingAlgorithm*);
void addToReady(t_trainer);
void addToExec(t_trainer);
void scheduleFifo();
void scheduleRR();
void scheduleSJFSD();
void scheduleSJFCD();
void scheduleDistance(pthread_t**);
void startInitMatrix(char *);
void assignMatrixValues(void *);
void addToList(char *);
void initlist(char *);
int getListSize(t_list *);
void getMatrix(char**,char**);
int getTrainersCount(t_config*);
void getTrainerAttr(char**,char**,char**,int);
void getTrainerAttrPos(char**,int);
void getTrainerAttrPkm(char**,int);
void getTrainerAttrObj(char**,int);
void startTrainers(int,t_config*);
void startTrainer(t_trainer*);
void startThread(t_trainer*);
void missingPokemons(t_trainer*, int);
void subscribeToBroker(pthread_t*);
void* subscribeToBrokerLocalized();
void* subscribeToBrokerAppeared();
void* subscribeToBrokerCaught();
int connectBroker(char*, char*);
void requestNewPokemons(t_objetive*,int,struct Broker);
void requestNewPokemon(t_pokemon,struct Broker);
int getGlobalObjetivesCount(t_trainer*, int);
//void initStateLists();
void waitForMessage(void*);
void processMessage(void*);
void processMessageLocalized(deli_message*);
void processMessageCaught(deli_message*);
void processMessageAppeared(deli_message*);
int findIdInGetList(uint32_t);
int findNameInAvailableList(char*);
void attendGameboy(void*);
void processGameBoyMessage(deli_message*);
void addToBlocked(t_trainer);
int startServer();
int waitClient(int);
void scheduleByDistance();
void initPreviousBurst(t_trainer);
void initScheduledPokemon(t_trainer);
void initBurstScheduledPokemon();
void removeFromExit();
void removeFromExec();
void removeFromBlocked(int);
void removeFromReady(int);
int getCountBlockedWaiting();
int getCountBlockedAvailable();
int getFirstBlockedAvailable();
void getClosestTrainer(t_pokemon*);
int getClosestTrainerNew(t_pokemon*);
int getClosestTrainerBlocked(t_pokemon*);
void initTrainerName();
void readConfigReconnectWaiting(t_config*);
void catchPokemon();
void readConfigTeamValues(t_config*);
void* startScheduling();
void initializeLists();


void initBroker(struct Broker *broker){
	broker->ipKey="IP_BROKER";
	broker->portKey="PUERTO_BROKER";

}

void initTeamServer(){
	teamServerAttr.ipKey="IP_TEAM";
	teamServerAttr.portKey="PUERTO_TEAM";

}

void initScheduler(struct SchedulingAlgorithm *schedulingAlgorithm){
	schedulingAlgorithm->algorithmKey="ALGORITMO_PLANIFICACION";
	schedulingAlgorithm->quantumKey="QUANTUM";
	schedulingAlgorithm->initEstimationKey="ESTIMACION_INICIAL";
}

void moveTrainerToTarget(t_trainer*, int distanceToMoveInX, int distanceToMoveInY);
void moveTrainerToObjective(t_trainer*);
int calculateDifference(int, int);
int getDistanceToPokemonTarget(t_trainerParameters, t_pokemon);
void moveTrainetToObjective(t_trainer*, t_pokemon*);
int executeClock();
int readConfigAlphaValue(t_config*);
int readConfigInitialEstimatedValue(t_config*);
float estimatedTimeForNextBurstCalculation(int);
void initializeTrainersWithBurts();
int getTrainerWithBestEstimatedBurst();
float differenceBetweenEstimatedBurtsAndExecutedClocks(float, uint32_t);
int blockedInDeadlock();
int evaluateDeadlockCondition();
#endif /* TEAM_H_ */
