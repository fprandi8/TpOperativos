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

t_log* logger;
pthread_t* thread;
sem_t catch_semaphore = 1;
sem_t countReady_semaphore = 1;

int main(void) {
	t_config* config;
	int teamSocket;
	char* ip;
	char* port;
	t_trainer* l_new;
	t_trainer* l_ready;
	t_trainer* l_blocked;
	t_trainer* l_exec;
	t_trainer* l_exit;
	t_stateLists stateLists;
	t_getMessages* getList;
	int trainersCount,readyCount,execCount;
    pthread_t* subs;
	t_objetive* missingPkms;
	localized_pokemon* localized;
	appeared_pokemon* appeared;
    int globalObjetivesDistinctCount=0,globalObjetivesCount=0;
    pthread_t closeScheduler,readyScheduler;

	thread = (pthread_t*)malloc(sizeof(pthread_t));

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
	l_new = (t_trainer*)malloc(sizeof(t_trainer)*trainersCount);
	startTrainers(l_new,trainersCount,config,logger);
	globalObjetivesCount = getGlobalObjetivesCount(l_new,trainersCount);
	missingPkms=(t_objetive*)malloc(sizeof(t_objetive)*globalObjetivesCount);
	missingPokemons(l_new,missingPkms,trainersCount,&globalObjetivesCount,&globalObjetivesDistinctCount,logger);
	void* temp = realloc(missingPkms,sizeof(t_objetive)*globalObjetivesDistinctCount);
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	missingPkms=temp;

	subs=(pthread_t*)malloc(sizeof(pthread_t)*3);
	subscribeToBroker(broker,subs);//TODO ver si esto no conviene hacerlo con select (Falta ver con Marcos)
	requestNewPokemons(missingPkms,globalObjetivesDistinctCount,logger,broker,getList);
	log_debug(logger,"\n\n");
	log_debug(logger,"Test de parametros");
	log_debug(logger,"Entrenador 0 está en la posición (x,y)=(%i,%i), tiene %i pokemons: %s, %s y %s y tiene %i objetivos %s, %s, %s y %s",l_new[0].parameters.position.x,l_new[0].parameters.position.y,l_new[0].parameters.pokemonsCount,l_new[0].parameters.pokemons[0].name,l_new[0].parameters.pokemons[1].name,l_new[0].parameters.pokemons[2].name,l_new[0].parameters.objetivesCount,l_new[0].parameters.objetives[0].name,l_new[0].parameters.objetives[1].name,l_new[0].parameters.objetives[2].name,l_new[0].parameters.objetives[3].name);
	log_debug(logger,"Entrenador 1 está en la posición (x,y)=(%i,%i), tiene %i pokemons: %s y %s y tiene %i objetivos %s, %s y %s",l_new[1].parameters.position.x,l_new[1].parameters.position.y,l_new[1].parameters.pokemonsCount,l_new[1].parameters.pokemons[0].name,l_new[1].parameters.pokemons[1].name,l_new[1].parameters.objetivesCount,l_new[1].parameters.objetives[0].name,l_new[1].parameters.objetives[1].name,l_new[1].parameters.objetives[2].name);
	log_debug(logger,"Entrenador 2 está  en la posición (x,y)=(%i,%i), tiene %i pokemons: %s y tiene %i objetivos %s y %s",l_new[2].parameters.position.x,l_new[2].parameters.position.y,l_new[2].parameters.pokemonsCount,l_new[2].parameters.pokemons[0].name,l_new[2].parameters.objetivesCount,l_new[2].parameters.objetives[0].name,l_new[2].parameters.objetives[1].name);
	log_debug(logger,"globalObjetivesCount: %i",globalObjetivesCount);
	log_debug(logger,"globalObjetivesDistinctCount: %i",globalObjetivesDistinctCount);
	for(int objCount=0;objCount<globalObjetivesDistinctCount;objCount++){
		log_debug(logger,"Missing Pokemon %i: %i %s",objCount,missingPkms[objCount].count,missingPkms[objCount].pokemon.name);
	}
	initStateLists(stateLists,l_new,l_blocked,l_ready,l_exec,l_exit);
    //startClosePlanning(new,blocked,ready);
	//startReadyPlaning(ready,exec);

	deleteLogger(&logger);
	return EXIT_SUCCESS;
}
//FIX
void initStateLists(t_stateLists stateLists,t_trainer* new, t_trainer* blocked,t_trainer* ready, t_trainer* exec, t_trainer* l_exit){
	stateLists.new = new;
	stateLists.blocked= blocked;
	stateLists.ready = ready;
	stateLists.exec = exec;
	stateLists.exit = l_exit;
}

//TODO
/*void startClosePlanning(closeScheduler,new,blocked,ready){
	t_trainet
	pthread_create(&(closeScheduler),NULL,(void*)startThread,(void*)trainer);
}
*/


int getGlobalObjetivesCount(t_trainer* trainers, int trainersCount){
	int objcount = 0;
	for(int trainer=0;trainer<trainersCount;trainer++){
		objcount = objcount+trainers[trainer].parameters.objetivesCount;
	}
	return objcount;
}


//TODO Consultar con marcos como importar las comons de delibird, meter en threads
void subscribeToBroker(struct Broker broker,pthread_t* subs){

	pthread_create(&(subs[0]),NULL,subscribeToBrokerCaught,(void*)&broker);
	pthread_create(&(subs[1]),NULL,subscribeToBrokerAppeared,(void*)&broker);
	pthread_create(&(subs[2]),NULL,subscribeToBrokerLocalized,(void*)&broker);
}

void* subscribeToBrokerLocalized(void *brokerAdress){
	log_debug(logger,"Creando thread Localized Subscriptions Handler");
	struct Broker broker = *((struct Broker*) brokerAdress);
	int socketLocalized = connectBroker(broker.ip,broker.port,logger);
	if (-1==SendSubscriptionRequest(LOCALIZED_POKEMON,socketLocalized)){
		log_debug(logger,"Error en subscripcion de Localized");
	}else{
		log_debug(logger,"Se subscribió a Localized");
	}

	t_args* args= (t_args*) malloc (sizeof (t_args));

	args->suscription = socketLocalized;
	args->queueType = LOCALIZED_POKEMON;
	args->brokerAddress= brokerAdress;

	pthread_create(thread,NULL,(void*)waitForMessage,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}




void* subscribeToBrokerAppeared(void *brokerAdress){
	log_debug(logger,"Creando thread Appeared Subscriptions Handler");
	struct Broker broker = *((struct Broker*) brokerAdress);
	int socketAppeared = connectBroker(broker.ip,broker.port,logger);
	if(-1==SendSubscriptionRequest(APPEARED_POKEMON,socketAppeared)){
		log_debug(logger,"Error en subscripcion de Appeared");
	}else{
		log_debug(logger,"Se subscribió a Appeared");
	}
	t_args* args= (t_args*) malloc (sizeof (t_args));

	args->suscription = socketAppeared;
	args->queueType = APPEARED_POKEMON;
	args->brokerAddress= brokerAdress;

	pthread_create(thread,NULL,(void*)waitForMessage,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}

void* subscribeToBrokerCaught(void *brokerAdress){
	log_debug(logger,"Creando thread Caught Subscriptions Handler");
	struct Broker broker = *((struct Broker*) brokerAdress);
	int socketCaught = connectBroker(broker.ip,broker.port,logger);
	if(-1==SendSubscriptionRequest(CAUGHT_POKEMON,socketCaught)){
		log_debug(logger,"Error en subscripcion de Caught");
	}else{
		log_debug(logger,"Se subscribió a Caught");
	}
	t_args* args= (t_args*) malloc (sizeof (t_args));

	args->suscription = socketCaught;
	args->queueType = CAUGHT_POKEMON;
	args->brokerAddress= brokerAdress;

	pthread_create(thread,NULL,(void*)waitForMessage,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}

void waitForMessage(void* variables){

	t_args* args= (t_args*)variables;
	int suscription = ((t_args*)variables)->suscription;
	void* brokerAddress = args->brokerAddress;
	uint32_t queueType = ((t_args*)variables)->queueType;

	char* queue;

	switch (queueType) {

		case NEW_POKEMON: {
			queue =(char*)malloc(strlen("QUEUE LOCALIZED POKEMON") + 1);
			strcpy(queue,"QUEUE LOCALIZED POKEMON");
			break;
		}

		case GET_POKEMON:{
			queue =(char*)malloc(strlen("QUEUE APPEARED POKEMON") + 1);
			strcpy(queue,"QUEUE APPEARED POKEMON");
			break;
		}

		case CAUGHT_POKEMON:{
			queue =(char*)malloc(strlen("QUEUE CAUGHT POKEMON") + 1);
			strcpy(queue,"QUEUE CAUGHT POKEMON");
			break;
		}

	}

	log_debug(logger, "Esperando mensajes de la %s ", queue);
	free(queue);

	op_code type;
	void* content = malloc(sizeof(void*));

	int resultado= RecievePackage(suscription,&type,&content);

	if (!resultado)
	{
		deli_message* message = (deli_message*)content;

		t_args_process_message* argsProcessMessage= (t_args_process_message*) malloc (sizeof (t_args_process_message));
		argsProcessMessage->message = message;
		argsProcessMessage->brokerAddress= brokerAddress;

		pthread_create(thread,NULL,(void*)processMessage,argsProcessMessage);
	}
	else
		log_debug(logger,"Resultado de envio del mensaje: %d", resultado);

	thread = (pthread_t*)malloc(sizeof(pthread_t));

	pthread_create(thread,NULL,(void*)waitForMessage,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}

void processMessage(void* variables){

	t_args_process_message* args = (t_args_process_message*)variables;

	deli_message* message = args->message;
	struct Broker broker = *((struct Broker*) ((t_args*)variables)->brokerAddress);

	switch (message->messageType){
		case LOCALIZED_POKEMON: {
			processMessageLocalized(message);
			break;
		}

		case APPEARED_POKEMON:{
			processMessageAppeared(message);
			break;
		}

		case CAUGHT_POKEMON:{
			processMessageCaught(message);
			break;
		}
	}

	free(args);
	log_debug(logger, "Se procesó el mensaje");
}

void processMessageLocalized(deli_message* message){
	;//TODO
}

void processMessageAppeared(deli_message* message){
	;//TODO
}

void processMessageCaught(deli_message* message){
	;//TODO
}


void requestNewPokemons(t_objetive* pokemons,int globalObjetivesDistinctCount,t_log* logger,struct Broker broker,t_getMessages* getList){
	getList->id = 0;
	log_debug(logger,"Se solicitarán %i pokemons",globalObjetivesDistinctCount);
	for(int obj=0;obj<globalObjetivesDistinctCount;obj++){
		requestNewPokemon(pokemons[obj].pokemon,logger,broker,getList);
	}
}

//TODO debería usar la shared cuando este implementado para mandar.
void requestNewPokemon(t_pokemon missingPkm,t_log* logger, struct Broker broker,t_getMessages* getList){
	log_debug(logger,"Se solicitarán el pokemon %s", missingPkm.name);
	int clientSocket = connectBroker(broker.ip, broker.port,logger);
	get_pokemon get;
	strcpy(get.pokemonName,missingPkm.name);
	log_debug(logger,"Se enviará el send para el pokemon %s", missingPkm.name);
	Send_GET(get, clientSocket);
	log_debug(logger,"Pokemon requested: %s",missingPkm.name);

	op_code type;
	void* content = malloc(sizeof(void*));
	int cut=0;
	int result;
	uint32_t* id;
	while(cut != 1){
		result = RecievePackage(clientSocket,&type,&content);
		if(type == ACKNOWLEDGE){
			cut=1;
		}
	}
	if(!result){
		id = (uint32_t*)content;
		(*getList).id[(*getList).count]=id;
		(*getList).count++;
	}
	else{
		log_debug(logger,"Error al recibir el acknowledge");
	}
}


void missingPokemons(t_trainer* trainers, t_objetive* objetives, int trainersCount,int* globalObjetivesCount,int* globalObjetivesDistinctCount,t_log* logger){
	for(int obj=0;obj<(*globalObjetivesCount);obj++){
		objetives[obj].count = 0;
	}

	for(int trainer=0;trainer<trainersCount;trainer++){
		for(int obj=0;obj<trainers[trainer].parameters.objetivesCount;obj++){
			if((*globalObjetivesDistinctCount)==0){
				objetives[(*globalObjetivesDistinctCount)].pokemon=trainers[trainer].parameters.objetives[obj];
				objetives[(*globalObjetivesDistinctCount)].count++;
				log_debug(logger,"add Entrenador %i, objetivo: %i, Agregar objetivo: %s en registro %i, cantidad total %i",trainer,obj,objetives[(*globalObjetivesDistinctCount)].pokemon.name,(*globalObjetivesDistinctCount),objetives[(*globalObjetivesDistinctCount)].count);
				(*globalObjetivesDistinctCount)++;
			}else{
				int added=0;
				for(int objCmp=0;objCmp<(*globalObjetivesDistinctCount);objCmp++){
					if(0==strcmp(objetives[objCmp].pokemon.name,trainers[trainer].parameters.objetives[obj].name)){
						added=1;
						objetives[objCmp].count++;
						log_debug(logger,"add Entrenador %i, objetivo: %i, Agregar objetivo: %s en registro %i, cantidad total %i",trainer,obj,objetives[objCmp].pokemon.name,objCmp,objetives[objCmp].count);
					}

				}
				if(added==0){
					objetives[(*globalObjetivesDistinctCount)].pokemon=trainers[trainer].parameters.objetives[obj];
					objetives[(*globalObjetivesDistinctCount)].count++;
					log_debug(logger,"add Entrenador %i, objetivo: %i, Agregar objetivo: %s en registro %i, cantidad total %i",trainer,obj,objetives[(*globalObjetivesDistinctCount)].pokemon.name,(*globalObjetivesDistinctCount),objetives[(*globalObjetivesDistinctCount)].count);
					(*globalObjetivesDistinctCount)++;
				}
			}
		}
	}
	for(int trainer=0;trainer<trainersCount;trainer++){
			for(int pkm=0;pkm<trainers[trainer].parameters.pokemonsCount;pkm++){
				for(int total=0;total<(*globalObjetivesCount);total++){
					if(0==strcmp(objetives[total].pokemon.name,trainers[trainer].parameters.pokemons[pkm].name)){
						if(objetives[total].count==1){
							log_debug(logger,"diff Entrenador %i, objetivo entrenador: %i vs objetivo lista: %i, Nombre: %s",trainer,pkm,total,objetives[total].pokemon.name);
							for(int new=total;new<(*globalObjetivesDistinctCount);new++){
								objetives[new]=objetives[new+1];
							}
							(*globalObjetivesCount)--;
							(*globalObjetivesDistinctCount)--;
						}else{
							log_debug(logger,"diff Entrenador %i, objetivo entrenador: %i vs objetivo lista: %i, Nombre: %s",trainer,pkm,total,objetives[total].pokemon.name);
							objetives[total].count--;
							(*globalObjetivesCount)--;
						}
						total=(*globalObjetivesCount);
					}
				}
			}
		}
	log_debug(logger,"El objetivo global consta de %i pokemons",(*globalObjetivesCount));
	log_debug(logger,"El objetivo global consta de %i registros",(*globalObjetivesDistinctCount));

	for(int count=0;count<(*globalObjetivesDistinctCount);count++){
		log_debug(logger,"Registro %i: %i %s",count,objetives[count].count,objetives[count].pokemon.name);
	}
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
	trainer->blockState=AVAILABLE;
	pthread_create(&(trainer->trainer),NULL,(void*)startThread,(void*)trainer);
	//pthread_join(trainer->trainer,NULL);
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

//TODO Agregar el cast de trainer (en realidad viene como void*)
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
	sem_wait(catch_semaphore);
	(*countReady)++;
	sem_post(catch_semaphore);
	schedule(trainers,countReady,schedulingAlgorithm,exec, logger);
}

//TODO - No debería hacer nada; siempre se agregan cosas al final de ready y se sacan del HEAD de ready
void scheduleFifo(t_trainer* trainers,int* count, t_trainer* exec, t_log* logger){

}

void addToExec(t_trainer* ready,int* countReady,t_trainer* exec,t_log* logger){
	exec[0]=ready[0];
	sem_wait(catch_semaphore);
	(*countReady)--;
	sem_post(catch_semaphore);
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
void scheduleRR(t_trainer* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm, t_trainer* exec, t_log* logger, t_objetive* localized_pokemon, int teamSocket){
	while(*countReady){
		int i=0;
		int valueOfExecuteClock = 1;
		t_trainer* trainer;
		trainer = ((&trainers)[i]);
		addToExec(trainer, countReady, exec, logger);
		for(int j=0;j<=(int)(schedulingAlgorithm.quantum) && valueOfExecuteClock == 1;j++){
			valueOfExecuteClock = executeClock(*countReady, exec, localized_pokemon, teamSocket);
		}
		if(valueOfExecuteClock == -1){
			for(i=0;i<(*countReady); i++){
				((&trainers)[i]) = ((&trainers)[i+1]);
			}
			addToReady(trainer, trainers, countReady, schedulingAlgorithm, logger, exec);
		}
		sem_wait(countReady_semaphore);
		(*countReady)--;
		sem_post(countReady_semaphore);
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
int executeClock(int countReady, t_trainer* trainer, t_pokemon* pokemonTargeted, int teamSocket){

	if(getDistanceToPokemonTarget(trainer,pokemonTargeted)!=0){
		moveTrainerToObjective(trainer, pokemonTargeted);
		return 1;
	}else if(getDistanceToPokemonTarget(trainer,pokemonTargeted)==0){
		sem_wait(catch_semaphore);
		Send_CATCH(pokemonTargeted, teamSocket);
		sem_post(catch_semaphore);
		return 0;
	}
	return -1;
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

int connectBroker(char* ip, char* puerto,t_log* logger)
{
	int teamSocket;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    getaddrinfo(ip, puerto, &hints, &servinfo);
    log_debug(logger,"IP y puerto configurado");
    for (p = servinfo; p != NULL; p = p->ai_next) {
    	teamSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		log_debug(logger,"Socket configurado");
		if (teamSocket == -1){
			log_debug(logger,"El socket se configuró incorrectamente");
			return -2;
		}
		log_debug(logger,"Se intentará conectar con Broker");
		if (connect(teamSocket, p->ai_addr, p->ai_addrlen)==0) {
			log_debug(logger,"La conexión fue realizada");
			freeaddrinfo(servinfo);
			return teamSocket;
		}else{
			log_debug(logger,"La conexión falló");
			close(teamSocket);
			return -1;
		}
	}
    return -1;
}
