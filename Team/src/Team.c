/*
 ============================================================================
 Name        : Team.c
 Author      : Mauro
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
	char* logFilename;
	void* mensaje = "Mensaje de prueba";
	pthread_t *trainers;
	//sem_t sem;
	char** trainersPosition;
	char** trainersPokemons;
	char** trainersObjetives;
	int trainersCount;
	t_trainerParameters** trainerParameters;


	config = leer_config();
	if (config_has_property(config,"LOG_FILE")){
		logFilename=config_get_string_value(config,"LOG_FILE");
		removeLogger(logFilename);
		logger = startLogger(logFilename);
		log_debug(logger,config_get_string_value(config,"LOG_FILE"));
	}


	if (config_has_property(config,"IP_BROKER")){
		log_debug(logger,config_get_string_value(config,"IP_BROKER"));
		ip=config_get_string_value(config,"IP_BROKER");
	}

	if (config_has_property(config,"PUERTO_BROKER")){
		log_debug(logger,config_get_string_value(config,"PUERTO_BROKER"));
		port=config_get_string_value(config,"PUERTO_BROKER");
	}

	log_debug(logger,"Process Team Started");

/*
	teamSocket = iniciar_cliente(ip, port,logger);
	log_debug(logger,"El socket del proceso Team es %i",teamSocket);
	enviar_mensaje(mensaje,strlen(mensaje),teamSocket);
	serve_client(&teamSocket,logger);
*/
	log_debug(logger,"Comienza la carga de config de entrenadores");
	if (config_has_property(config,"POSICIONES_ENTRENADORES")){
		trainersPosition=config_get_array_value(config,"POSICIONES_ENTRENADORES");
	}
	if (config_has_property(config,"POKEMON_ENTRENADORES")){
		trainersPokemons=config_get_array_value(config,"POKEMON_ENTRENADORES");
	}
	if (config_has_property(config,"OBJETIVOS_ENTRENADORES")){
		trainersObjetives=config_get_array_value(config,"OBJETIVOS_ENTRENADORES");
	}
	log_debug(logger,"Finalizó la carga de config de entrenadores");

	trainersCount=getTrainersCount(trainersPosition);
	log_debug(logger,"Se contaron %i entrenadores",trainersCount);
	trainers = (pthread_t*)malloc(sizeof(pthread_t)*trainersCount);
	log_debug(logger,"Se alocó memoria para el array de threads");
	trainerParameters = malloc(trainersCount*sizeof(t_trainerParameters));
	log_debug(logger,"Se alocó memoria para los el array de parametros de entrenadores");
	log_debug(logger,"Comienza el proceso de carga de atributos");
	getTrainerAttr(trainersPosition,trainersPokemons,trainersObjetives,trainerParameters,trainersCount);
	log_debug(logger,"Finalizó el proceso de carga de atributos");
	deleteLogger(logger);
	return EXIT_SUCCESS;
}

t_log* startLogger(char* logFilename)
{
	t_log* logger;
	logger = log_create(logFilename, "Team", 1, LOG_LEVEL_DEBUG);
	if (logger == NULL){
		printf("No se pudo crear el logger\n");
		exit(1);
	}
	return logger;
}
void deleteLogger(t_log* logger)
{
	if (logger != NULL){
		log_destroy(logger);
	}
}
void removeLogger(char* logFilename)
{
	remove(logFilename);
}
t_config* leer_config(void)
{
	t_config* config;
	config = config_create("Team.config");
	if (config == NULL){
		printf("No se pudo crear el config\n");
		exit(2);
	}
	return config;

}
void startTrainers(char** trainersPosition,char** trainersPokemons,char** trainersObjetives,pthread_t trainers)
{
	int trainerCounts;
	//trainerCount = getTrainersCount(trainersPosition);
	//log_debug(logger,"Se leyeron %i entrenadores",trainerCount);

}

void getTrainerAttr(char** trainersPosition,char** trainersPokemons,char** trainersObjetives,t_trainerParameters** trainerParameters, int trainersCount)
{
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
			trainerParameters[actualTrainer]=malloc(sizeof(t_trainerParameters*));
	}
	getTrainerAttrPos(trainersPosition,trainerParameters,trainersCount);
	getTrainerAttrPkm(trainersPokemons,trainerParameters,trainersCount);
	getTrainerAttrObj(trainersObjetives,trainerParameters,trainersCount);
	}



void getTrainerAttrPos(char** trainersPosition,t_trainerParameters** trainerParameters, int trainersCount)
{

	log_debug(logger,"Comienza el proceso de carga de posición");
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
		log_debug(logger,"Comienza el proceso de carga de posición del entrenador %i",actualTrainer);
		trainerParameters[actualTrainer]->position=malloc(sizeof(t_trainerPosition*));
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
				  log_debug(logger,"Comienza el proceso de carga de posición x");
				  trainerParameters[actualTrainer]->position->x=malloc(sizeof(int));
				  trainerParameters[actualTrainer]->position->x=atoi((char*)element);
				  log_debug(logger,"El valor es %i",trainerParameters[actualTrainer]->position->x);
				  pos='y';
			  }else if(pos=='y'){
				  log_debug(logger,"Comienza el proceso de carga de posición y");
				  trainerParameters[actualTrainer]->position->y=malloc(sizeof(int));
				  trainerParameters[actualTrainer]->position->y=atoi((char*)element);
				  log_debug(logger,"El valor es %i",trainerParameters[actualTrainer]->position->y);
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
}
void getTrainerAttrPkm(char** trainersPokemons,t_trainerParameters** trainerParameters, int trainersCount)
{

	log_debug(logger,"Comienza el proceso de carga de pokemons");
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
		log_debug(logger,"Comienza el proceso de carga de pokemons del entrenador %i",actualTrainer);

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
				  log_debug(logger,"Comienza el proceso de carga de pokemon");
				  trainerParameters[actualTrainer]->pokemons[counter]=malloc(sizeof(t_pokemon*));
				  trainerParameters[actualTrainer]->pokemons[counter]->name=malloc(sizeof(char*));
				  trainerParameters[actualTrainer]->pokemons[counter]->name=(char*)element;
				  log_debug(logger,"El valor es %s",trainerParameters[actualTrainer]->pokemons[counter]->name);
				  counter++;
		  }

		  void _getRow(char *string) {
			  if(string != NULL) {
				  if(rowCount==actualTrainer){
				  char **row = string_split(string, "|");
				  string_iterate_lines(row, _toList);
				  list_iterate(list, countPokemons);
				  trainerParameters[actualTrainer]->pokemons=malloc(pokemonCount*sizeof(t_pokemon*));
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
void getTrainerAttrObj(char** trainersObjetives,t_trainerParameters** trainerParameters, int trainersCount)
{

	log_debug(logger,"Comienza el proceso de carga de objetivos");
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
		log_debug(logger,"Comienza el proceso de carga de objetivos del entrenador %i",actualTrainer);

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
				  log_debug(logger,"Comienza el proceso de carga de objetivos");
				  trainerParameters[actualTrainer]->objetives[counter]=malloc(sizeof(t_pokemon*));
				  trainerParameters[actualTrainer]->objetives[counter]->name=malloc(sizeof(char*));
				  trainerParameters[actualTrainer]->objetives[counter]->name=(char*)element;
				  log_debug(logger,"El valor es %s",trainerParameters[actualTrainer]->objetives[counter]->name);
				  counter++;
		  }

		  void _getRow(char *string) {
			  if(string != NULL) {
				  if(rowCount==actualTrainer){
				  char **row = string_split(string, "|");
				  string_iterate_lines(row, _toList);
				  list_iterate(list, countPokemons);
				  trainerParameters[actualTrainer]->objetives=malloc(pokemonCount*sizeof(t_pokemon*));
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
}
