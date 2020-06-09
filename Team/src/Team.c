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

t_pokemon* localized_pokemon;

int main(void) {
	t_config* config;
	int teamSocket;
	char* ip;
	char* port;
	void* mensaje = "Mensaje de prueba";
	t_trainer* new;
	int trainersCount;
	t_log* logger;
	t_trainer* ready,exec;
	int readyCount,execCount;
	t_pokemon* missingPkms;

	//Init de config y logger
	createConfig(&config);
	startLogger(config,&logger);

	//Init de broker
	initBroker(&broker);
	readConfigBrokerValues(config,logger,&broker);


	//Init de scheduler
	initScheduler(&schedulingAlgorithm);
	readConfigSchedulerValues(config,logger,&schedulingAlgorithm);//TODO agregar validaciones para que sólo se acepten los algoritmos especificos

/*
	teamSocket = iniciar_cliente(ip, port,logger);
	log_debug(logger,"El socket del proceso Team es %i",teamSocket);
	enviar_mensaje(mensaje,strlen(mensaje),teamSocket);
	serve_client(&teamSocket,logger);
*/
	trainersCount=getTrainersCount(config,logger);
	log_debug(logger,"4. Se contaron %i entrenadores",trainersCount);
	log_debug(logger,"5.Se alocó memoria para el array de threads");
	new = (t_trainer*)malloc(sizeof(t_trainer)*trainersCount);
	startTrainers(new,trainersCount,config,logger);
	//missingPokemons(new,missingPkms,trainersCount,logger);
	log_debug(logger,"\n\n\n\nTest de parametros");
	log_debug(logger,"Entrenador 0 está en la posición (x,y)=(%i,%i), tiene %i pokemons: %s, %s y %s y tiene %i objetivos %s, %s, %s y %s",new[0].parameters.position.x,new[0].parameters.position.y,new[0].parameters.pokemonsCount,new[0].parameters.pokemons[0].name,new[0].parameters.pokemons[1].name,new[0].parameters.pokemons[2].name,new[0].parameters.objetivesCount,new[0].parameters.objetives[0].name,new[0].parameters.objetives[1].name,new[0].parameters.objetives[2].name,new[0].parameters.objetives[3].name);
	log_debug(logger,"Entrenador 1 está en la posición (x,y)=(%i,%i), tiene %i pokemons: %s y %s y tiene %i objetivos %s, %s y %s",new[1].parameters.position.x,new[1].parameters.position.y,new[1].parameters.pokemonsCount,new[1].parameters.pokemons[0].name,new[1].parameters.pokemons[1].name,new[1].parameters.objetivesCount,new[1].parameters.objetives[0].name,new[1].parameters.objetives[1].name,new[1].parameters.objetives[2].name);
	log_debug(logger,"Entrenador 2 está en la posición (x,y)=(%i,%i), tiene %i pokemons: %s y tiene %i objetivos %s y %s",new[2].parameters.position.x,new[2].parameters.position.y,new[2].parameters.pokemonsCount,new[2].parameters.pokemons[0].name,new[2].parameters.objetivesCount,new[2].parameters.objetives[0].name,new[2].parameters.objetives[1].name);
	//startScheduler(new,logger);

	deleteLogger(&logger);
	return EXIT_SUCCESS;
}


//TODO
/*
void missingPokemons(t_trainer* trainers, t_pokemon* pokemons, int trainersCount,t_log* logger){
	int i = 0;
	int pkmTotalCount =0;
	for(int trainer=0;trainer<trainersCount;trainer++){
		pkmTotalCount = pkmTotalCount+trainers[trainer].parameters.pokemonsCount;
	}

	for(int trainer=0;trainer<trainersCount;trainer++){
		for(int pkm=0;pkm<trainers[trainer].parameters.pokemonsCount;pkm++){
			pokemons[i]=trainers[trainer].parameters.pokemons[pkm];
			log_debug(logger,"MssngPkm: %i: Se agregó el pokemon %s a la lista. En la lista se lee: %s",i,trainers[trainer].parameters.pokemons[pkm].name,pokemons[i]);
			i++;
		}
	}
}*/

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
	log_debug(logger,"4. Finalizó la carga de config de entrenadores");

}

void startTrainers(t_trainer* trainers,int trainersCount,t_config *config,t_log* logger){

	char** trainersPosition;
	char** trainersPokemons;
	char** trainersObjetives;

	readConfigTrainersValues(config,logger,&trainersPosition,&trainersPokemons,&trainersObjetives);
	log_debug(logger,"4. Se alocó memoria para el array de parametros de entrenadore");
	log_debug(logger,"4. Comienza el proceso de carga de atributos en struc");
	getTrainerAttr(trainersPosition,trainersPokemons,trainersObjetives,trainersCount,logger,trainers);
	log_debug(logger,"5.Comienza el proceso de creación de threads");
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
		sem_init(&(trainers[actualTrainer].semaphore),0,0);
		log_debug(logger,"Creando el entrenador %i",actualTrainer);
		startTrainer(&(trainers[actualTrainer]),logger);
	}
	log_debug(logger,"5. Terminó el proceso de creación de threads");
}

void startTrainer(t_trainer* trainer,t_log *logger){

	trainer->trainer=(pthread_t)malloc(sizeof(pthread_t));
	pthread_create(&(trainer->trainer),NULL,startThread,trainer);
	pthread_join(trainer->trainer,NULL);
	log_debug(logger,"5. Se creó un entrenador");
}

void getTrainerAttr(char** trainersPosition,char** trainersPokemons,char** trainersObjetives, int trainersCount,t_log* logger,t_trainer* trainers){

	getTrainerAttrPos(trainersPosition,trainers,trainersCount,logger);
	getTrainerAttrPkm(trainersPokemons,trainers,trainersCount,logger);
	getTrainerAttrObj(trainersObjetives,trainers,trainersCount,logger);
	log_debug(logger,"4. Finalizó el proceso de carga de atributos");
}



void getTrainerAttrPos(char** trainersPosition,t_trainer* trainers, int trainersCount,t_log *logger){

	log_debug(logger,"4.1. Comienza el proceso de carga de posición de entrenadores");
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
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
				  trainers[actualTrainer].parameters.position.x=atoi((char*)element);
				  pos='y';
			  }else if(pos=='y'){
				  trainers[actualTrainer].parameters.position.y=atoi((char*)element);
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
void getTrainerAttrPkm(char** trainersPokemons,t_trainer* trainers, int trainersCount,t_log *logger)
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
		  int counter=0;
		  void getElement(void *element) {
			  trainers[actualTrainer].parameters.pokemons[counter].name=malloc(sizeof((char*)element));
			  strcpy(trainers[actualTrainer].parameters.pokemons[counter].name,(char*)element);
			  counter++;
		  }

		  void _getRow(char *string) {
			  if(string != NULL) {
				  if(rowCount==actualTrainer){
				  char **row = string_split(string, "|");
				  string_iterate_lines(row, _toList);
				  list_iterate(list, countPokemons);
				  trainers[actualTrainer].parameters.pokemonsCount=pokemonCount;
				  trainers[actualTrainer].parameters.pokemons=malloc(pokemonCount*sizeof(t_pokemon));
				  int counter=0;
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

}

void getTrainerAttrObj(char** trainersObjetives,t_trainer* trainers, int trainersCount,t_log *logger)
{

	log_debug(logger,"4.3. Comienza el proceso de carga de objetivos de entrenadores");
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
		  int counter=0;
		  void getElement(void *element) {
			  trainers[actualTrainer].parameters.objetives[counter].name=malloc(sizeof((char*)element));
				  strcpy(trainers[actualTrainer].parameters.objetives[counter].name,(char*)element);
				  counter++;
		  }

		  void _getRow(char *string) {
			  if(string != NULL) {
				  if(rowCount==actualTrainer){
				  char **row = string_split(string, "|");
				  string_iterate_lines(row, _toList);
				  list_iterate(list, countPokemons);
				  trainers[actualTrainer].parameters.objetivesCount=pokemonCount;
				  trainers[actualTrainer].parameters.objetives=malloc(pokemonCount*sizeof(t_pokemon));
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

//TODO SACAR SEM_WAIT
void startThread(t_trainer* trainer){
	//sem_wait(&(trainer->semaphore));
	printf("\nTrainer Thread created\n");

}

int getTrainersCount(t_config *config,t_log* logger) {
	char** array;
	if (config_has_property(config,"POSICIONES_ENTRENADORES")){
			array=config_get_array_value(config,"POSICIONES_ENTRENADORES");
		}

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
	return count;
}

void schedule(t_trainer* trainers,int* readyCount,struct SchedulingAlgorithm schedulingAlgorithm,t_trainer* exec, t_log* logger){//Para el caso de FIFO y RR no hace nada, ya que las listas están ordenadas por FIFO y RR solo cambia como se procesa.
	if (strcmp(schedulingAlgorithm.algorithm,"FIFO")==0){
		scheduleFifo(trainers,readyCount, exec, logger);
	}else if(strcmp(schedulingAlgorithm.algorithm,"RR")==0){
		scheduleRR(trainers,readyCount,schedulingAlgorithm, exec, logger);
	}else if(strcmp(schedulingAlgorithm.algorithm,"SJF-SD")==0){
		scheduleSJFSD(trainers,readyCount,schedulingAlgorithm, exec, logger);
	}else if(strcmp(schedulingAlgorithm.algorithm,"SJF-CD")==0){
		scheduleSJFCD(trainers,readyCount,schedulingAlgorithm, exec, logger);
	}
}

void addToReady(t_trainer* trainer,t_trainer* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm,t_log* logger, t_trainer* exec){
	void* temp = realloc(trainers,sizeof(t_trainer)*((*countReady)+1));
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	(trainers)=temp;
	(trainers)[(*countReady)]=(*trainer);
	(*countReady)++;
	schedule(trainers,countReady,schedulingAlgorithm,exec, logger);
}

//TODO - No debería hacer nada; siempre se agregan cosas al final de ready y se sacan del HEAD de ready
void scheduleFifo(t_trainer* trainers,int* count, t_trainer* exec, t_log* logger){

}

void addToExec(t_trainer* ready,int* countReady,t_trainer* exec,t_log* logger){
	exec[0]=ready[0];
	(*countReady)--;
	for(int i=0;i<(*countReady);i++){
		ready[i]=ready[i+1];
	}
	void* temp = realloc(ready,sizeof(t_trainer)*(*countReady));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(9);
		}
		ready=temp;
}

//TODO - cuando termina el quantum mandar al final de la lista de ready.
void scheduleRR(t_trainer* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm, t_trainer* exec, t_log* logger){
	while((&countReady)){
		int i=0;
		t_trainer* trainer;
		trainer = ((&trainers)[i]);
		addToExec(trainer, countReady, exec, logger);
		for(int j=0;j<=(int)(schedulingAlgorithm.quantum);j++){
			executeClock(&countReady, exec, localized_pokemon);
		}
		(*countReady)--;
	}
}

//TODO
void scheduleSJFSD(t_trainer* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm, t_trainer* exec, t_log* logger){
;
}

//TODO
void scheduleSJFCD(t_trainer* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm, t_trainer* exec, t_log* logger){
;
}


//TODO - Cómo hacemos para pasarle el targetedPokemon
int executeClock(int countReady, t_trainer* trainer, t_pokemon* pokemonTargeted){

	if(getDistanceToPokemonTarget(trainer,pokemonTargeted)!=0){
		moveTrainerToObjective(trainer, pokemonTargeted);
		return 1;
	}else if(getDistanceToPokemonTarget(trainer,pokemonTargeted)==0){
		//CATCH_POKEMON(localized_pokemon)
		return 0;
	}
}


//TODO - Función que mueve al entrenador - Falta ver como implementaremos los semáforos
void moveTrainerToObjective(t_trainer* trainer,  t_pokemon* pokemonTargeted){

	//t_trainerParameters* trainerToMove;
	//trainerToMove = *trainer;
	int difference_x;
	difference_x = calculateDifference(trainer->parameters.position.x, pokemonTargeted->position.x);
	int difference_y;
	difference_y = calculateDifference(trainer->parameters.position.y, pokemonTargeted->position.y);
	//meter semáforos acá
	moveTrainerToTarget(trainer, difference_x, difference_y);
	//fin semáforos;
}

//TODO - funcion que mueve una posición al entrenador - Falta Definir como haremos el CATCH
void moveTrainerToTarget(t_trainer* trainer, int distanceToMoveInX, int distanceToMoveInY){
	if(distanceToMoveInX > 0){
		trainer->parameters.position.x++;
	}
	else if(distanceToMoveInX < 0){
		trainer->parameters.position.x--;
	}
	else if(distanceToMoveInY > 0){
		trainer->parameters.position.y++;
	}
	else if(distanceToMoveInY < 0){
		trainer->parameters.position.y--;
	}

}


//Funcion que calcula la diferencia de posiciones tanto en x como y
int calculateDifference(int oldPostion, int newPosition){
	int difference = oldPostion - newPosition;
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
int getDistanceToPokemonTarget(t_trainer* trainer,  t_pokemon* targetPokemon){
	int distanceInX = calculateDifference(trainer->parameters.position.x, targetPokemon->position.x);
	int distanceInY = calculateDifference(trainer->parameters.position.y, targetPokemon->position.y);
	int distance = getClockTimeToNewPosition(distanceInX, distanceInY);
	return distance;
}

