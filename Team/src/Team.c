/*
 ============================================================================
 Name        : Team.c
 Author      : Mauro y Cristian
 Version     :
 Copyright   : Your copyright notice
 Description : Team Process
 ============================================================================
 */

#include "Team.h"



int main(void) {
	t_config* config;
	int teamSocket;
	char* ip;
	char* port;
	void* mensaje = "Mensaje de prueba";
	pthread_t* new;
	//sem_t sem;
	char** trainersPosition;
	char** trainersPokemons;
	char** trainersObjetives;
	int trainersCount;
	t_trainerParameters** trainersParameters;
	t_log* logger;
	int x, y;
	pthread_t* ready,exec;
	int readyCount,execCount;

	//Init de config y logger
	createConfig(&config);
	startLogger(config,&logger);

	//Init de broker
	initBroker(&broker);
	readConfigBrokerValues(config,logger,&broker);


	//Init de scheduler
	initScheduler(&schedulingAlgorithm);
	readConfigSchedulerValues(config,logger,&schedulingAlgorithm);

/*
	teamSocket = iniciar_cliente(ip, port,logger);
	log_debug(logger,"El socket del proceso Team es %i",teamSocket);
	enviar_mensaje(mensaje,strlen(mensaje),teamSocket);
	serve_client(&teamSocket,logger);
*/
	readConfigTrainersValues(config,logger,&trainersPosition,&trainersPokemons,&trainersObjetives);
	trainersCount=getTrainersCount(trainersPosition,logger);
	log_debug(logger,"4. Se contaron %i entrenadores",trainersCount);
	trainersParameters = malloc(trainersCount*sizeof(t_trainerParameters));
	log_debug(logger,"4. Se alocó memoria para el array de parametros de entrenadores");
	log_debug(logger,"4. Comienza el proceso de carga de atributos en struc");
	getTrainerAttr(trainersPosition,trainersPokemons,trainersObjetives,trainersParameters,trainersCount,logger);
	log_debug(logger,"5.Comienza el proceso de creación de threads");
	log_debug(logger,"5.Se alocó memoria para el array de threads");
	new = (pthread_t*)malloc(sizeof(pthread_t)*trainersCount);
	startTrainers(new,trainersParameters,trainersCount);
	log_debug(logger,"5.Finalizó el proceso de creación de threads");
	deleteLogger(&logger);
	printf("\n%i",getClockTimeToNewPosition(2,6));
	return EXIT_SUCCESS;
}










void createLogger(char* logFilename, t_log **logger)
{
	*logger = log_create(logFilename, "Team", 1, LOG_LEVEL_DEBUG);
	if (logger == NULL){
		printf("No se pudo crear el logger\n");
		exit(1);
	}
}

void startLogger(t_config *config,t_log **logger){
	if (config_has_property(config,"LOG_FILE")){
		char* logFilename;
		logFilename=config_get_string_value(config,"LOG_FILE");
		printf("\n %s \n",logFilename);
		removeLogger(logFilename);
		createLogger(logFilename,logger);
	}
	log_debug(*logger,"1. Finaliza creación de config y logger");
}

void deleteLogger(t_log **logger)
{
	if (logger != NULL){
		log_destroy(*logger);
	}
}
void removeLogger(char* logFilename)
{
	remove(logFilename);
}

void createConfig(t_config **config)
{
	*config = config_create("Team.config");
	if (*config == NULL){
		printf("No se pudo crear el config\n");
		exit(2);
	}
}

void readConfigBrokerValues(t_config *config,t_log *logger,struct Broker *broker){
	log_debug(logger,"2. Comienza lectura de config de broker");
	if (config_has_property(config,broker->ipKey)){
		broker->ip=config_get_string_value(config,broker->ipKey);
		log_debug(logger,"2. Se leyó la IP: %s",broker->ip);
	}else{
		exit(-3);
	}

	if (config_has_property(config,broker->portKey)){
		broker->port=config_get_string_value(config,broker->portKey);
		log_debug(logger,"2. Se leyó el puerto: %s",broker->port);
	}else{
		exit(-3);
	}
	log_debug(logger,"2. Finaliza lectura de config de broker");
}

void readConfigSchedulerValues(t_config *config,t_log *logger,struct SchedulingAlgorithm *schedulingAlgorithm){
	log_debug(logger,"3. Comienza lectura de config de scheduler");
	if (config_has_property(config,schedulingAlgorithm->algorithmKey)){
		schedulingAlgorithm->algorithm=config_get_string_value(config,schedulingAlgorithm->algorithmKey);
		log_debug(logger,"3. Se leyó el algoritmo: %s",schedulingAlgorithm->algorithm);
	}else{
		exit(-3);
	}
	if (config_has_property(config, schedulingAlgorithm->quantumKey)){
		schedulingAlgorithm->quantum=config_get_string_value(config,schedulingAlgorithm->quantumKey);
		log_debug(logger,"3. Se leyó el quantum: %s",schedulingAlgorithm->quantum);
	}else{
		exit(-3);
	}
	if (config_has_property(config,schedulingAlgorithm->initEstimationKey)){
		schedulingAlgorithm->initEstimation=config_get_string_value(config,schedulingAlgorithm->initEstimationKey);
		log_debug(logger,"3. Se leyó la estimación inicial: %s",schedulingAlgorithm->initEstimation);
	}else{
		exit(-3);
	}
	log_debug(logger,"3. Finaliza lectura de config de scheduler");
}


void readConfigTrainersValues(t_config *config,t_log *logger,char*** trainersPosition,char*** trainersPokemons,char*** trainersObjetives){
	log_debug(logger,"4. Comienza lectura de parámetros de entrenador");
	if (config_has_property(config,"POSICIONES_ENTRENADORES")){
		*trainersPosition=config_get_array_value(config,"POSICIONES_ENTRENADORES");
	}
	if (config_has_property(config,"POKEMON_ENTRENADORES")){
		*trainersPokemons=config_get_array_value(config,"POKEMON_ENTRENADORES");
	}
	if (config_has_property(config,"OBJETIVOS_ENTRENADORES")){
		*trainersObjetives=config_get_array_value(config,"OBJETIVOS_ENTRENADORES");
	}
	log_debug(logger,"Finalizó la carga de config de entrenadores");

}

void startTrainers(pthread_t* trainers,t_trainerParameters** trainersParameters,int trainersCount)
{
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
		startTrainer(trainers[actualTrainer],trainersParameters[actualTrainer]);
	}
	//log_debug(logger,"Se leyeron %i entrenadores",trainerCount);
}
void startTrainer(pthread_t trainer,t_trainerParameters* trainerParameters)
{
	pthread_create(&trainer,NULL,startThread,trainerParameters);
	pthread_join(trainer,NULL);
}

void getTrainerAttr(char** trainersPosition,char** trainersPokemons,char** trainersObjetives,t_trainerParameters** trainersParameters, int trainersCount,t_log* logger)
{
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
			trainersParameters[actualTrainer]=malloc(sizeof(t_trainerParameters*));
	}
	getTrainerAttrPos(trainersPosition,trainersParameters,trainersCount,logger);
	getTrainerAttrPkm(trainersPokemons,trainersParameters,trainersCount,logger);
	getTrainerAttrObj(trainersObjetives,trainersParameters,trainersCount,logger);
	log_debug(logger,"4. Finalizó el proceso de carga de atributos");
	}



void getTrainerAttrPos(char** trainersPosition,t_trainerParameters** trainersParameters, int trainersCount,t_log *logger)
{

	log_debug(logger,"4.1. Comienza el proceso de carga de posición de entrenadores");
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
		trainersParameters[actualTrainer]->position=malloc(sizeof(t_trainerPosition*));
		int rowCount=0;
			char pos='x';

		  t_list *list = list_create();
		  void _toList(char *row) {
			if (row != NULL) {
			  list_add(list, row);
			}
		  }
		  void getElement(void *element) {
			  if(pos=='x'){
				  trainersParameters[actualTrainer]->position->x=malloc(sizeof(int));
				  trainersParameters[actualTrainer]->position->x=atoi((char*)element);
				  pos='y';
			  }else if(pos=='y'){
				  trainersParameters[actualTrainer]->position->y=malloc(sizeof(int));
				  trainersParameters[actualTrainer]->position->y=atoi((char*)element);
			  }
		  }

		  void _getRow(char *string) {
			  if(string != NULL) {
				  if(rowCount==actualTrainer){
				  char **row = string_split(string, "|");
				  string_iterate_lines(row, _toList);
				  list_iterate(list, getElement);
				  }
				  rowCount++;
			} else {
			  printf("Got NULL\n");
			}
		  }
		  string_iterate_lines(trainersPosition, _getRow);
		  list_destroy_and_destroy_elements(list,free);
	  }
	log_debug(logger,"4.1. Finaliza el proceso de carga de posición de entrenadores");
}
void getTrainerAttrPkm(char** trainersPokemons,t_trainerParameters** trainersParameters, int trainersCount,t_log *logger)
{

	log_debug(logger,"4.2. Comienza el proceso de carga de pokemons de entrenadores");
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){

		int rowCount=0;

		  t_list *list = list_create();
		  void _toList(char *row) {
			if (row != NULL) {
			  list_add(list, row);
			}
		  }
		  int pokemonCount=0;
		  void countPokemons(void *element){
			  pokemonCount++;
		  }
		  void getElement(void *element) {
				  int counter=0;
				  trainersParameters[actualTrainer]->pokemons[counter]=malloc(sizeof(t_pokemon*));
				  trainersParameters[actualTrainer]->pokemons[counter]->name=malloc(sizeof(char*));
				  trainersParameters[actualTrainer]->pokemons[counter]->name=(char*)element;
				  counter++;
		  }

		  void _getRow(char *string) {
			  if(string != NULL) {
				  if(rowCount==actualTrainer){
				  char **row = string_split(string, "|");
				  string_iterate_lines(row, _toList);
				  list_iterate(list, countPokemons);
				  trainersParameters[actualTrainer]->pokemons=malloc(pokemonCount*sizeof(t_pokemon*));
				  list_iterate(list, getElement);
				  }
				  rowCount++;
			} else {
			  printf("Got NULL\n");
			}
		  }
		  string_iterate_lines(trainersPokemons, _getRow);
		  list_destroy_and_destroy_elements(list,free);
	  }
	log_debug(logger,"4.2. Finaliza el proceso de carga de pokemons de entrenadores");
}
void getTrainerAttrObj(char** trainersObjetives,t_trainerParameters** trainersParameters, int trainersCount,t_log *logger)
{

	log_debug(logger,"4.3. Comienza el proceso de carga de objetivos de entrenadores");
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
		int rowCount=0;
			char pos='x';

		  t_list *list = list_create();
		  void _toList(char *row) {
			if (row != NULL) {
			  list_add(list, row);
			}
		  }
		  int pokemonCount=0;
		  void countPokemons(void *element){
			  pokemonCount++;
		  }
		  void getElement(void *element) {
				  int counter=0;
				  trainersParameters[actualTrainer]->objetives[counter]=malloc(sizeof(t_pokemon*));
				  trainersParameters[actualTrainer]->objetives[counter]->name=malloc(sizeof(char*));
				  trainersParameters[actualTrainer]->objetives[counter]->name=(char*)element;
				  counter++;
		  }

		  void _getRow(char *string) {
			  if(string != NULL) {
				  if(rowCount==actualTrainer){
				  char **row = string_split(string, "|");
				  string_iterate_lines(row, _toList);
				  list_iterate(list, countPokemons);
				  trainersParameters[actualTrainer]->objetives=malloc(pokemonCount*sizeof(t_pokemon*));
				  list_iterate(list, getElement);
				  }
				  rowCount++;
			} else {
			  printf("Got NULL\n");
			}
		  }
		  string_iterate_lines(trainersObjetives, _getRow);
		  list_destroy_and_destroy_elements(list,free);
	  }
	log_debug(logger,"4.3. Finaliza el proceso de carga de objetivos de entrenadores");
}
void startThread(t_trainerParameters* trainerParameters){
//do something
	int prueba;
}

int getTrainersCount(char** array,t_log* logger) {
	log_debug(logger,"Comienza el conteo de entrenadores");
	int count=0;
	t_list *list = list_create();
	void _toLista(char *row) {
		if (row != NULL) {
			list_add(list, row);
		}
	}
	void _getRow(char *string) {
		if(string != NULL) {
			char **row = string_split(string, "|");
			string_iterate_lines(row, _toLista);
			count++;
		} else {
			printf("Got NULL\n");
		}
	}
	string_iterate_lines(array, _getRow);
	list_destroy_and_destroy_elements(list,free);
	log_debug(logger,"Los entrenadores son: %i",count);
	return count;
}

void schedule(pthread_t** threads,int* readyCount,struct SchedulingAlgorithm* schedulingAlgorithm,t_log* logger){
	if (strcmp(schedulingAlgorithm->algorithm,"FIFO")==0){
		scheduleFifo(threads,readyCount);
	}else if(strcmp(schedulingAlgorithm->algorithm,"RR")==0){
		scheduleRR(threads,readyCount,schedulingAlgorithm);
	}else if(strcmp(schedulingAlgorithm->algorithm,"SJF-SD")==0){
		scheduleSJFSD(threads,readyCount,schedulingAlgorithm);
	}else if(strcmp(schedulingAlgorithm->algorithm,"SJF-CD")==0){
		scheduleSJFCD(threads,readyCount,schedulingAlgorithm);
	}else{
		log_debug(logger,"Valor incorrecto de scheduler en config");
		exit(8);
	}
}

void addToReady(pthread_t* thread,pthread_t** threads,int* countReady,struct SchedulingAlgorithm* schedulingAlgorithm,t_log* logger){
	void* temp = realloc(*threads,sizeof(pthread_t)*((*countReady)+1));
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	(*threads)=temp;
	(*threads)[(*countReady)]=(*thread);
	(*countReady)++;
	schedule(threads,countReady,schedulingAlgorithm,logger);
}

//TODO - No debería hacer nada; siempre se agregan cosas al final de ready y se sacan del HEAD de ready
void scheduleFifo(pthread_t** threads,int* count){
	int i;
	i++;

}

void addToExec(pthread_t** ready,int* countReady,pthread_t** exec,t_log* logger){
	(*exec)[0]=(*ready)[0];
	(*countReady)--;
	for(int i=0;i<(*countReady);i++){
		(*ready)[i]=(*ready)[i+1];
	}
	void* temp = realloc(*ready,sizeof(pthread_t)*(*countReady));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(9);
		}
		(*ready)=temp;
}

//TODO
void scheduleRR(pthread_t** threads,int* countReady,struct SchedulingAlgorithm* schedulingAlgorithm){
	int i=1;
	i++;
}

//TODO
void scheduleSJFSD(pthread_t** threads,int* countReady,struct SchedulingAlgorithm* schedulingAlgorithm){
	int i=1;
	i++;
}

//TODO
void scheduleSJFCD(pthread_t** threads,int* countReady,struct SchedulingAlgorithm* schedulingAlgorithm){
	int i=1;
	i++;
}





//TODO - Función que mueve al entrenador - Falta ver como implementaremos los semáforos
t_trainerParameters* moveTrainerToObjective(t_trainerParameters** trainer,  t_pokemon* pokemonTargeted){

	//t_trainerParameters* trainerToMove;
	//trainerToMove = *trainer;
	int difference_x;
	difference_x = calculateDifference((*trainer)->position->x, pokemonTargeted->position->x);
	int difference_y;
	difference_y = calculateDifference((*trainer)->position->y, pokemonTargeted->position->y);
	//meter semáforos acá
	moveTrainerToTarget(*trainer, difference_x, difference_y);
	//fin semáforos;
	return *trainer;
}

//TODO - funcion que mueve una posición al entrenador - Falta Definir como haremos el CATCH
void moveTrainerToTarget(t_trainerParameters* trainer, int distanceToMoveInX, int distanceToMoveInY){
	if(distanceToMoveInX > 0){
		trainer->position->x++;
	}
	else if(distanceToMoveInX < 0){
		trainer->position->x--;
	}
	else if(distanceToMoveInY > 0){
		trainer->position->y++;
	}
	else if(distanceToMoveInY < 0){
		trainer->position->y++;
	}
	else{
		//CATCH_POKEMON
	}
}


//Funcion que calcula la diferencia de posiciones tanto en x como y
int calculateDifference(int position_old, int position_new){
	int difference = position_old - position_new;
	return difference;
}


//Función que calcula los ciclos de Reloj que demora en mover al entrenador
int getClockTimeToNewPosition(int difference_x, int difference_y){
	int clockTime = 0;
	clockTime = (difference_x >= 0) ? clockTime + difference_x : clockTime - difference_x;
	clockTime = (difference_y >= 0) ? clockTime + difference_y : clockTime - difference_y;
	return clockTime;
}

//Función que devuelve la distancia hacia el pokemon.  TODO hay que hacer una funcion target generica,porque el target puede ser un trainer tambien (deadlock)
int getDistanceToPokemonTarget(t_trainerParameters* trainer,  t_pokemon* targetPokemon){
	int distanceInX = calculateDifference(trainer->position->x, targetPokemon->position->x);
	int distanceInY = calculateDifference(trainer->position->y, targetPokemon->position->y);
	int distance = getClockTimeToNewPosition(distanceInX, distanceInY);
	return distance;
}

