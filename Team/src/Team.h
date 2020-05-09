/*
 * Team.h
 *
 *  Created on: 19 abr. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/log.h>
#include "utils.h"
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore.h>
#include<commons/collections/list.h>




/* typedef enum
{
	FIFO=1,//FIFO
	RR=2, //RoundRobin
	SJFCD=3, //SJF con desalojo
	SJFSD=4	//SJF sin desalojo
} schedulingAlgorithm;
*/

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
void scheduleFifo(pthread_t**,int*);
void scheduleRR(pthread_t**,int*,struct SchedulingAlgorithm*);
void scheduleSJFSD(pthread_t**,int*,struct SchedulingAlgorithm*);
void scheduleSJFCD(pthread_t**,int*,struct SchedulingAlgorithm*);
void scheduleDistance(pthread_t**);

void initBroker(struct Broker *broker){
	broker->ipKey="IP_BROKER";
	broker->portKey="PUERTO_BROKER";

}

void initScheduler(struct SchedulingAlgorithm *schedulingAlgorithm){
	schedulingAlgorithm->algorithmKey="ALGORITMO_PLANIFICACION";
	schedulingAlgorithm->quantumKey="QUANTUM";
	schedulingAlgorithm->initEstimationKey="ESTIMACION_INICIAL";
}

void moveTrainerToTarget(t_trainerParameters* trainer, int distanceToMoveInX, int distanceToMoveInY);

t_trainerParameters* moveTrainerToObjective(t_trainerParameters** trainer,  t_pokemon* pokemonTargeted);
int calculateDifference(int, int);
int getDistanceToPokemonTarget(t_trainerParameters*, t_pokemon*);

#endif /* TEAM_H_ */
