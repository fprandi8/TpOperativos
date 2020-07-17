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
#include "semaphore.h"
#include "delibird/comms/pokeio.h"

t_log* logger;
pthread_t* thread;
sem_t* catch_semaphore = 1;
sem_t* countReady_semaphore = 1;
sem_t* countBlocked_semaphore = 1;
sem_t* countNew_semaphore = 1;
sem_t* countExit_semaphore = 1;
t_pokemonList availablePokemons;
sem_t* availablePokemons_sem = 1;
t_idMessages getList;
t_catchMessages catchList;
t_objetive* missingPkms;
int globalObjetivesDistinctCount;
int globalObjetivesCount;
int missingPokemonsCount;//TODO: Decrementar cada vez que hay un caught
struct SchedulingAlgorithm schedulingAlgorithm;
int alphaForSJF;
int initialEstimatedBurst;


int main(void) {
	t_config* config;
	int teamSocket;
	char* ip;
	char* port;
	int trainersCount;
    pthread_t* subs;

	localized_pokemon* localized;
	appeared_pokemon* appeared;


    globalObjetivesDistinctCount=0;
    globalObjetivesCount=0;
    pthread_t closeScheduler,readyScheduler;

	thread = (pthread_t*)malloc(sizeof(pthread_t));

	//Init de config y logger
	createConfig(&config);
	startLogger(config);

	//Init de broker
	initBroker(&broker);
	readConfigBrokerValues(config,&broker);


	//Init de scheduler
	initScheduler(&schedulingAlgorithm);
	readConfigSchedulerValues(config,&schedulingAlgorithm);//TODO agregar validaciones para que sólo se acepten los algoritmos especificos

	//Init Alpha for SJF and Initial Estimated Burst
	alphaForSJF = readConfigAlphaValue(config);
	initialEstimatedBurst = readConfigInitialEstimatedValue(config);
/*
	teamSocket = iniciar_cliente(ip, port,logger);
	log_debug(logger,"El socket del proceso Team es %i",teamSocket);
	enviar_mensaje(mensaje,strlen(mensaje),teamSocket);
	serve_client(&teamSocket,logger);
*/
	trainersCount=getTrainersCount(config);
	log_debug(logger,"4. Se contaron %i entrenadores",trainersCount);
	log_debug(logger,"5.Se alocó memoria para el array de threads");
	statesLists.newList.trainerList = (t_trainer*)malloc(sizeof(t_trainer)*trainersCount);
	startTrainers(trainersCount,config);
	globalObjetivesCount = getGlobalObjetivesCount(statesLists.newList.trainerList,trainersCount);
	missingPkms=(t_objetive*)malloc(sizeof(t_objetive)*globalObjetivesCount);
	missingPokemons(statesLists.newList.trainerList,trainersCount);
	void* temp = realloc(missingPkms,sizeof(t_objetive)*globalObjetivesDistinctCount);
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	missingPkms=temp;
	missingPokemonsCount = globalObjetivesDistinctCount;

	subs=(pthread_t*)malloc(sizeof(pthread_t)*3);
	subscribeToBroker(subs);//TODO ver si esto no conviene hacerlo con select (Falta ver con Marcos)
	requestNewPokemons(missingPkms,globalObjetivesDistinctCount,broker);
	/*log_debug(logger,"\n\n");
	log_debug(logger,"Test de parametros");
	log_debug(logger,"Entrenador 0 está en la posición (x,y)=(%i,%i), tiene %i pokemons: %s, %s y %s y tiene %i objetivos %s, %s, %s y %s",l_new[0].parameters.position.x,l_new[0].parameters.position.y,l_new[0].parameters.pokemonsCount,l_new[0].parameters.pokemons[0].name,l_new[0].parameters.pokemons[1].name,l_new[0].parameters.pokemons[2].name,l_new[0].parameters.objetivesCount,l_new[0].parameters.objetives[0].name,l_new[0].parameters.objetives[1].name,l_new[0].parameters.objetives[2].name,l_new[0].parameters.objetives[3].name);
	log_debug(logger,"Entrenador 1 está en la posición (x,y)=(%i,%i), tiene %i pokemons: %s y %s y tiene %i objetivos %s, %s y %s",l_new[1].parameters.position.x,l_new[1].parameters.position.y,l_new[1].parameters.pokemonsCount,l_new[1].parameters.pokemons[0].name,l_new[1].parameters.pokemons[1].name,l_new[1].parameters.objetivesCount,l_new[1].parameters.objetives[0].name,l_new[1].parameters.objetives[1].name,l_new[1].parameters.objetives[2].name);
	log_debug(logger,"Entrenador 2 está  en la posición (x,y)=(%i,%i), tiene %i pokemons: %s y tiene %i objetivos %s y %s",l_new[2].parameters.position.x,l_new[2].parameters.position.y,l_new[2].parameters.pokemonsCount,l_new[2].parameters.pokemons[0].name,l_new[2].parameters.objetivesCount,l_new[2].parameters.objetives[0].name,l_new[2].parameters.objetives[1].name);
	log_debug(logger,"globalObjetivesCount: %i",globalObjetivesCount);
	log_debug(logger,"globalObjetivesDistinctCount: %i",globalObjetivesDistinctCount);
	for(int objCount=0;objCount<globalObjetivesDistinctCount;objCount++){
		log_debug(logger,"Missing Pokemon %i: %i %s",objCount,missingPkms[objCount].count,missingPkms[objCount].pokemon.name);
	}*/
	//initStateLists(statesLists);
    //startClosePlanning(new,blocked,ready);
	//startReadyPlaning(ready,exec);

	int teamServer = startServer();
	int client;
	while(1){
		client = waitClient(teamServer);
		log_debug(logger,"Gameboy connected");
		pthread_create(thread,NULL,(void*)attendGameboy,&client);
		pthread_detach(*thread);
	}
	deleteLogger();
	return EXIT_SUCCESS;
}

void attendGameboy(void* var){
	uint32_t type;
	void* content = malloc(sizeof(void*));
	int* client = (int*)var;
	int result = RecievePackage(*(client),&type,&content);
	if(!result){
		log_debug(logger,"Gameboy's message processed");
		deli_message* message = (deli_message*)content;
		int result = SendMessageAcknowledge(message->id,*(client));
		if(!result){
			log_debug(logger,"Acknowledge sent to Gameboy");
		}
		processGameBoyMessage(message);
	}else{
		log_debug(logger,"Error receiving Gameboy's message");
	}
	pthread_exit(NULL);
}
void processGameBoyMessage(deli_message* message){
	switch(message->messageType){
		case APPEARED_POKEMON: {
			processMessageAppeared(message);
			break;
		}
	}
	free(message->messageContent);
	free(message);
}

int waitClient(int teamSocket){
	struct sockaddr_in clientDir;
	int tamDirection = sizeof(struct sockaddr_in);
	return accept(teamSocket,(void*)&clientDir,&tamDirection);
}

int startServer(t_log* logger)
{
	int teamSocket;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    //hints.ai_flags = 0;
    //hints.ai_protocol = 0;

    getaddrinfo(teamServerAttr.ip, teamServerAttr.port, &hints, &servinfo);
    log_debug(logger,"IP y puerto configurado");
    for (p = servinfo; p != NULL; p = p->ai_next) {
    	teamSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (teamSocket == -1){
			log_debug(logger,"El socket se configuró incorrectamente");
			continue;
		}
		log_debug(logger,"Socket configurado");
		uint32_t flag=1;
		setsockopt(teamSocket,SOL_SOCKET,SO_REUSEPORT,&(flag),sizeof(flag));
		if (bind(teamSocket, p->ai_addr, p->ai_addrlen)==-1) {
			close(teamSocket);
			log_debug(logger,"La conexión falló");
			continue;
		}else{
			log_debug(logger,"La conexión fue realizada");
			break;
		}

	}
    listen(teamSocket,SOMAXCONN);
    freeaddrinfo(servinfo);
    return teamSocket;
}



//TODO - FIX
/*void initStateLists(t_stateLists statesLists){
	stateLists.new = new;
	stateLists.blocked= blocked;
	stateLists.ready = ready;
	stateLists.exec = exec;
	stateLists.exit = l_exit;
}*/

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

void subscribeToBroker(pthread_t* subs){

	pthread_create(&(subs[0]),NULL,subscribeToBrokerCaught,NULL);
	pthread_create(&(subs[1]),NULL,subscribeToBrokerAppeared,NULL);
	pthread_create(&(subs[2]),NULL,subscribeToBrokerLocalized,NULL);
}

void* subscribeToBrokerLocalized(){
	log_debug(logger,"Creando thread Localized Subscriptions Handler");
	int socketLocalized = connectBroker(broker.ip,broker.port);
	if (-1==SendSubscriptionRequest(LOCALIZED_POKEMON,socketLocalized)){
		log_debug(logger,"Error en subscripcion de Localized");
	}else{
		log_debug(logger,"Se subscribió a Localized");
	}

	t_args* args= (t_args*) malloc (sizeof (t_args));

	args->suscription = socketLocalized;
	args->queueType = LOCALIZED_POKEMON;

	pthread_create(thread,NULL,(void*)waitForMessage,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}




void* subscribeToBrokerAppeared(){
	log_debug(logger,"Creando thread Appeared Subscriptions Handler");
	int socketAppeared = connectBroker(broker.ip,broker.port);
	if(-1==SendSubscriptionRequest(APPEARED_POKEMON,socketAppeared)){
		log_debug(logger,"Error en subscripcion de Appeared");
	}else{
		log_debug(logger,"Se subscribió a Appeared");
	}
	t_args* args= (t_args*) malloc (sizeof (t_args));

	args->suscription = socketAppeared;
	args->queueType = APPEARED_POKEMON;

	pthread_create(thread,NULL,(void*)waitForMessage,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}

void* subscribeToBrokerCaught(){
	log_debug(logger,"Creando thread Caught Subscriptions Handler");
	int socketCaught = connectBroker(broker.ip,broker.port);
	if(-1==SendSubscriptionRequest(CAUGHT_POKEMON,socketCaught)){
		log_debug(logger,"Error en subscripcion de Caught");
	}else{
		log_debug(logger,"Se subscribió a Caught");
	}
	t_args* args= (t_args*) malloc (sizeof (t_args));

	args->suscription = socketCaught;
	args->queueType = CAUGHT_POKEMON;
	pthread_create(thread,NULL,(void*)waitForMessage,args);
	pthread_detach(*thread);

	pthread_exit(NULL);
}

void waitForMessage(void* variables){

	t_args* args= (t_args*)variables;
	int suscription = ((t_args*)variables)->suscription;
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

int findIdInGetList(uint32_t cid){
	int position=-1;
	for(int i=0;i<getList.count;i++){
		int size = sizeof(uint32_t);
		int compare = memcmp(getList.id[i],cid,size);
		if(compare==0){
			position=i;
			break;
		}
	}
	return position;
}

int findNameInAvailableList(char* pokeName){
	int boolean=0;
	int size;
	for(int i=0;i<availablePokemons.count;i++){
		size = sizeof(pokeName);
		int compare = memcmp(availablePokemons.pokemons[i].name,pokeName,sizeof(size));
		if(compare==0){
			boolean=1;
			break;
		}
	}
	return boolean;
}

void processMessageLocalized(deli_message* message){
	localized_pokemon* localizedPokemon = (localized_pokemon*)message->messageContent;
	uint32_t cid = (uint32_t)message->correlationId;
	int resultGetId = findIdInGetList(cid);
	int resultReceivedPokemon = findNameInAvailableList(localizedPokemon->pokemonName);
	if(resultGetId>=0 && resultReceivedPokemon==0){
		sem_wait(availablePokemons_sem);
		void* temp = realloc(availablePokemons.pokemons,sizeof(t_pokemon)*(availablePokemons.count+localizedPokemon->ammount));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(9);
		}
		availablePokemons.pokemons=temp;
		for(int i=0;i<localizedPokemon->ammount;i++){
			availablePokemons.pokemons[availablePokemons.count+1].name=localizedPokemon->pokemonName;
			availablePokemons.pokemons[availablePokemons.count+1].position.x=localizedPokemon->coordinates->x;
			availablePokemons.pokemons[availablePokemons.count+1].position.y=localizedPokemon->coordinates->y;
			availablePokemons.count++;
			sem_post(availablePokemons_sem);
		}
	}
}

int findNameInMissingPokemons(char* pokeName){
	int boolean=0;
	int size;
	for(int i=0;i<missingPokemonsCount;i++){
		size = sizeof(pokeName);
		int compare = memcmp(missingPkms[i].pokemon.name,pokeName,size);
		if(compare==0){
			boolean=1;
			break;
		}
	}
	return boolean;
}


void processMessageAppeared(deli_message* message){
	appeared_pokemon* appearedPokemon = (appeared_pokemon*)message->messageContent;
	int resultMissingPokemon = findNameInMissingPokemons(appearedPokemon->pokemonName);
		if(resultMissingPokemon==1){//TODO: Agregar en el planificador que valide si ya hay un entrenador planificado para ese pokemon que llego por otro appeared o localized
			sem_wait(availablePokemons_sem);
			void* temp = realloc(availablePokemons.pokemons,sizeof(t_pokemon)*(availablePokemons.count+1));
			if (!temp){
				log_debug(logger,"error en realloc");
				exit(9);
			}
			availablePokemons.pokemons=temp;
			availablePokemons.pokemons[availablePokemons.count+1].name=appearedPokemon->pokemonName;
			availablePokemons.pokemons[availablePokemons.count+1].position.x=appearedPokemon->horizontalCoordinate;
			availablePokemons.pokemons[availablePokemons.count+1].position.y=appearedPokemon->verticalCoordinate;
			availablePokemons.count++;
			sem_post(availablePokemons_sem);
				}
}

int findIdInCatchList(uint32_t cid){
	int position=-1;
	for(int i=0;i<catchList.count;i++){
		int compare = memcmp(catchList.catchMessage[i].id,cid,sizeof(uint32_t));
		if(compare==0){
			position=i;
			break;
		}
	}
	return position;
}

void processMessageCaught(deli_message* message){
	caught_pokemon* caughtPokemon = (caught_pokemon*)message->messageContent;
	uint32_t cid = (uint32_t)message->correlationId;
	int resultCatchId = findIdInCatchList(cid);
	if(resultCatchId>=0){
		if(caughtPokemon->caught==1){
			missingPokemonsCount--;
		;//TODO: Borrar de missingPokemon. Ver si entrenador pasa a EXIT; sino, cambiar blockstate de WAITING a AVAILABLE.SACAR PKM de scheduledPokemon
		}else{
			//catchList.catchMessage[resultCatchId].trainer.blockState = AVAILABLE;
		//TODO: ACá habría que chequear si hay pokemons disponibles para atrapar, si Sí, ; si no, fin de la función.SACAR PKM de scheduledPokemon
		}
	}
}


void requestNewPokemons(t_objetive* pokemons,int globalObjetivesDistinctCount,struct Broker broker){
	getList.id = 0;
	getList.count=0;
	log_debug(logger,"Se solicitarán %i pokemons",globalObjetivesDistinctCount);
	for(int obj=0;obj<globalObjetivesDistinctCount;obj++){
		requestNewPokemon(pokemons[obj].pokemon,broker);
	}

}

//TODO debería usar la shared cuando este implementado para mandar.
void requestNewPokemon(t_pokemon missingPkm, struct Broker broker){
	log_debug(logger,"Se solicitarán el pokemon %s", missingPkm.name);
	int clientSocket = connectBroker(broker.ip, broker.port);
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
		void* temp = realloc(getList.id,sizeof(uint32_t)*((getList.count)+1));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(9);
		}
		getList.id=temp;
		getList.id[getList.count]=id;
		getList.count++;
	}
	else{
		log_debug(logger,"Error al recibir el acknowledge");
	}
}


void missingPokemons(t_trainer* trainers, int trainersCount){
	for(int obj=0;obj<globalObjetivesCount;obj++){
		missingPkms[obj].count = 0;
	}

	for(int trainer=0;trainer<trainersCount;trainer++){
		for(int obj=0;obj<trainers[trainer].parameters.objetivesCount;obj++){
			if(globalObjetivesDistinctCount==0){
				missingPkms[globalObjetivesDistinctCount].pokemon=trainers[trainer].parameters.objetives[obj];
				missingPkms[globalObjetivesDistinctCount].count++;
				log_debug(logger,"add Entrenador %i, objetivo: %i, Agregar objetivo: %s en registro %i, cantidad total %i",trainer,obj,missingPkms[globalObjetivesDistinctCount].pokemon.name,globalObjetivesDistinctCount,missingPkms[globalObjetivesDistinctCount].count);
				globalObjetivesDistinctCount++;
			}else{
				int added=0;
				for(int objCmp=0;objCmp<globalObjetivesDistinctCount;objCmp++){
					if(0==strcmp(missingPkms[objCmp].pokemon.name,trainers[trainer].parameters.objetives[obj].name)){
						added=1;
						missingPkms[objCmp].count++;
						log_debug(logger,"add Entrenador %i, objetivo: %i, Agregar objetivo: %s en registro %i, cantidad total %i",trainer,obj,missingPkms[objCmp].pokemon.name,objCmp,missingPkms[objCmp].count);
					}

				}
				if(added==0){
					missingPkms[globalObjetivesDistinctCount].pokemon=trainers[trainer].parameters.objetives[obj];
					missingPkms[globalObjetivesDistinctCount].count++;
					log_debug(logger,"add Entrenador %i, objetivo: %i, Agregar objetivo: %s en registro %i, cantidad total %i",trainer,obj,missingPkms[globalObjetivesDistinctCount].pokemon.name,globalObjetivesDistinctCount,missingPkms[globalObjetivesDistinctCount].count);
					globalObjetivesDistinctCount++;
				}
			}
		}
	}
	for(int trainer=0;trainer<trainersCount;trainer++){
			for(int pkm=0;pkm<trainers[trainer].parameters.pokemonsCount;pkm++){
				for(int total=0;total<globalObjetivesCount;total++){
					if(0==strcmp(missingPkms[total].pokemon.name,trainers[trainer].parameters.pokemons[pkm].name)){
						if(missingPkms[total].count==1){
							log_debug(logger,"diff Entrenador %i, objetivo entrenador: %i vs objetivo lista: %i, Nombre: %s",trainer,pkm,total,missingPkms[total].pokemon.name);
							for(int new=total;new<globalObjetivesDistinctCount;new++){
								missingPkms[new]=missingPkms[new+1];
							}
							globalObjetivesCount--;
							globalObjetivesDistinctCount--;
						}else{
							log_debug(logger,"diff Entrenador %i, objetivo entrenador: %i vs objetivo lista: %i, Nombre: %s",trainer,pkm,total,missingPkms[total].pokemon.name);
							missingPkms[total].count--;
							globalObjetivesCount--;
						}
						total=globalObjetivesCount;
					}
				}
			}
		}
	log_debug(logger,"El objetivo global consta de %i pokemons",globalObjetivesCount);
	log_debug(logger,"El objetivo global consta de %i registros",globalObjetivesDistinctCount);

	for(int count=0;count<globalObjetivesDistinctCount;count++){
		log_debug(logger,"Registro %i: %i %s",count,missingPkms[count].count,missingPkms[count].pokemon.name);
	}
}

void createLogger(char* logFilename)
{
	logger = log_create(logFilename, "Team", 1, LOG_LEVEL_DEBUG);
	if (logger == NULL){
		printf("No se pudo crear el logger\n");
		exit(1);
	}
}

void startLogger(t_config *config){
	if (config_has_property(config,"LOG_FILE")){
		char* logFilename;
		logFilename=config_get_string_value(config,"LOG_FILE");
		printf("\n %s \n",logFilename);
		removeLogger(logFilename);
		createLogger(logFilename);
	}
	log_debug(logger,"1. Finaliza creación de config y logger");
}

void deleteLogger()
{
	if (logger != NULL){
		log_destroy(logger);
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

void readConfigBrokerValues(t_config *config,struct Broker *broker){
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

void readConfigSchedulerValues(t_config *config,struct SchedulingAlgorithm *schedulingAlgorithm){
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


void readConfigTrainersValues(t_config *config,char*** trainersPosition,char*** trainersPokemons,char*** trainersObjetives){
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

int readConfigAlphaValue(t_config *config){//TODO: DONDE LO CARGAS?
	if(config_has_property(config,"ALPHA")){
		int alpha;
		alpha=config_get_int_value(config,"ALPHA");
		return alpha;
	}else{
		return 0;
	}
}

int readConfigInitialEstimatedValue(t_config* config){//TODO: DONDE LO CARGAS?
	if(config_has_property(config,"ESTIMACION_INICIAL")){
		int initialEstimatedBurst = config_get_int_value(config,"ESTIMACION_INICIAL");
		return initialEstimatedBurst;
	}else{
		return 0;
	}
}
void startTrainers(int trainersCount,t_config *config){

	char** trainersPosition;
	char** trainersPokemons;
	char** trainersObjetives;

	readConfigTrainersValues(config,&trainersPosition,&trainersPokemons,&trainersObjetives);
	log_debug(logger,"4. Se alocó memoria para el array de parametros de entrenadore");
	log_debug(logger,"4. Comienza el proceso de carga de atributos en struc");
	getTrainerAttr(trainersPosition,trainersPokemons,trainersObjetives,trainersCount);
	initBurstScheduledPokemon();
	log_debug(logger,"5.Comienza el proceso de creación de threads");
	for(int actualTrainer = 0; actualTrainer < trainersCount; actualTrainer++){
		sem_init(&(statesLists.newList.trainerList[actualTrainer].semaphore),0,0);
		log_debug(logger,"Creando el entrenador %i",actualTrainer);
		startTrainer(&(statesLists.newList.trainerList[actualTrainer]));
	}
	log_debug(logger,"5. Terminó el proceso de creación de threads");
}

void initBurstScheduledPokemon(){
	for(int trainer = 0; trainer < statesLists.newList.count; trainer++){
		initPreviousBurst(statesLists.newList.trainerList[trainer]);
		initScheduledPokemon(statesLists.newList.trainerList[trainer]);
	}
}

void initPreviousBurst(t_trainer trainer){
	trainer.parameters.previousBurst=-1;
}

void initScheduledPokemon(t_trainer trainer){
	trainer.parameters.scheduledPokemon.name="NULL";
}

void startTrainer(t_trainer* trainer){

	trainer->trainer=(pthread_t)malloc(sizeof(pthread_t));
	trainer->blockState=AVAILABLE;
	pthread_create(&(trainer->trainer),NULL,(void*)startThread,(void*)trainer);
	pthread_detach(trainer->trainer);
	log_debug(logger,"5. Se creó un entrenador");
}

void getTrainerAttr(char** trainersPosition,char** trainersPokemons,char** trainersObjetives, int trainersCount){

	getTrainerAttrPos(trainersPosition,trainersCount);
	getTrainerAttrPkm(trainersPokemons,trainersCount);
	getTrainerAttrObj(trainersObjetives,trainersCount);
	log_debug(logger,"4. Finalizó el proceso de carga de atributos");
}



void getTrainerAttrPos(char** trainersPosition, int trainersCount){

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
				  statesLists.newList.trainerList[actualTrainer].parameters.position.x=atoi((char*)element);
				  pos='y';
			  }else if(pos=='y'){
				  statesLists.newList.trainerList[actualTrainer].parameters.position.y=atoi((char*)element);
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
void getTrainerAttrPkm(char** trainersPokemons, int trainersCount)
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
			  statesLists.newList.trainerList[actualTrainer].parameters.pokemons[counter].name=malloc(sizeof((char*)element));
			  strcpy(statesLists.newList.trainerList[actualTrainer].parameters.pokemons[counter].name,(char*)element);
			  counter++;
		  }

		  void _getRow(char *string) {
			  if(string != NULL) {
				  if(rowCount==actualTrainer){
				  char **row = string_split(string, "|");
				  string_iterate_lines(row, _toList);
				  list_iterate(list, countPokemons);
				  statesLists.newList.trainerList[actualTrainer].parameters.pokemonsCount=pokemonCount;
				  statesLists.newList.trainerList[actualTrainer].parameters.pokemons=malloc(pokemonCount*sizeof(t_pokemon));
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

void getTrainerAttrObj(char** trainersObjetives, int trainersCount)
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
			  statesLists.newList.trainerList[actualTrainer].parameters.objetives[counter].name=malloc(sizeof((char*)element));
				  strcpy(statesLists.newList.trainerList[actualTrainer].parameters.objetives[counter].name,(char*)element);
				  counter++;
		  }

		  void _getRow(char *string) {
			  if(string != NULL) {
				  if(rowCount==actualTrainer){
				  char **row = string_split(string, "|");
				  string_iterate_lines(row, _toList);
				  list_iterate(list, countPokemons);
				  statesLists.newList.trainerList[actualTrainer].parameters.objetivesCount=pokemonCount;
				  statesLists.newList.trainerList[actualTrainer].parameters.objetives=malloc(pokemonCount*sizeof(t_pokemon));
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
	sem_wait(&(trainer->semaphore));
	printf("\nTrainer Thread created\n");

}

int getTrainersCount(t_config *config) {
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

void schedule(){//Para el caso de FIFO y RR no hace nada, ya que las listas están ordenadas por FIFO y RR solo cambia como se procesa.
	if (strcmp(schedulingAlgorithm.algorithm,"FIFO")==0){
		scheduleFifo();
	}else if(strcmp(schedulingAlgorithm.algorithm,"RR")==0){
		scheduleRR();
	}else if(strcmp(schedulingAlgorithm.algorithm,"SJF-SD")==0){
		scheduleSJFSD();
	}else if(strcmp(schedulingAlgorithm.algorithm,"SJF-CD")==0){
		scheduleSJFCD();
	}
}

void addToReady(t_trainer trainer){
	void* temp = realloc(statesLists.readyList.trainerList,sizeof(t_trainer)*((statesLists.readyList.count)+1));
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	(statesLists.readyList.trainerList)=temp;
	sem_wait(countReady_semaphore);
	statesLists.readyList.trainerList[statesLists.readyList.count]=trainer;
	(statesLists.readyList.count)++;
	sem_post(countReady_semaphore);
	schedule();
}

void removeFromReady(int trainerPositionInList){

	for(int i=trainerPositionInList;i<(statesLists.readyList.count)-1; i++){
		statesLists.readyList.trainerList[i] = statesLists.readyList.trainerList[i+1];

	}
	sem_wait(countReady_semaphore);
	(statesLists.readyList.count)--;
	sem_post(countReady_semaphore);
	void* temp = realloc(statesLists.readyList.trainerList,sizeof(t_trainer)*((statesLists.readyList.count)));
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	(statesLists.readyList.trainerList)=temp;
}

void removeFromNew(int trainerPositionInList){

	for(int i=trainerPositionInList;i<(statesLists.newList.count)-1; i++){
		statesLists.newList.trainerList[i] = statesLists.newList.trainerList[i+1];

	}
	sem_wait(countNew_semaphore);
	(statesLists.newList.count)--;
	sem_post(countNew_semaphore);
	void* temp = realloc(statesLists.newList.trainerList,sizeof(t_trainer)*((statesLists.newList.count)));
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	(statesLists.newList.trainerList)=temp;
}


void removeFromBlocked(int trainerPositionInList){

	for(int i=trainerPositionInList;i<(statesLists.blockedList.count)-1; i++){
		statesLists.blockedList.trainerList[i] = statesLists.blockedList.trainerList[i+1];

	}
	sem_wait(countBlocked_semaphore);
	(statesLists.blockedList.count)--;
	sem_post(countBlocked_semaphore);
	void* temp = realloc(statesLists.blockedList.trainerList,sizeof(t_trainer)*((statesLists.blockedList.count)));
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	(statesLists.blockedList.trainerList)=temp;
}

void addToBlocked(t_trainer trainer){
	void* temp = realloc(statesLists.blockedList.trainerList,sizeof(t_trainer)*((statesLists.blockedList.count)+1));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(10);
		}
		(statesLists.blockedList.trainerList)=temp;
		sem_wait(countBlocked_semaphore);
		statesLists.blockedList.trainerList[statesLists.blockedList.count]=trainer;
		(statesLists.blockedList.count)++;
		sem_post(countBlocked_semaphore);
}


void addToExec(t_trainer trainer){
	statesLists.execTrainer.trainer=trainer;
	statesLists.execTrainer.boolean = 1;
}

void removeFromExec(){
	statesLists.execTrainer.boolean = 0;
}

void addToExit(t_trainer trainer){
	void* temp = realloc(statesLists.exitList.trainerList,sizeof(t_trainer)*((statesLists.exitList.count)+1));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(10);
		}
		(statesLists.exitList.trainerList)=temp;
		sem_wait(countExit_semaphore);
		statesLists.exitList.trainerList[statesLists.exitList.count]=trainer;
		(statesLists.exitList.count)++;
		sem_post(countExit_semaphore);
}

int getCountBlockedWaiting(){
	int contador = 0;
	for(int i=0;i<statesLists.blockedList.count;i++){
		if(statesLists.blockedList.trainerList[i].blockState==WAITING){
			contador++;
		}
	}
	return contador;
}

int getCountBlockedAvailable(){
	int contador = 0;
	for(int i=0;i<statesLists.blockedList.count;i++){
		if(statesLists.blockedList.trainerList[i].blockState==AVAILABLE){
			contador++;
		}
	}
	return contador;
}

int getFirstBlockedAvailable(){
	for(int i=0;i<statesLists.blockedList.count;i++){
		if(statesLists.blockedList.trainerList[i].blockState==AVAILABLE){
			return i;
		}
	}
	return -1;
}

void getClosestTrainer(t_pokemon* pkmAvailable){
	int closestNew;
	int closestBlocked;
	if(getCountBlockedAvailable()==0){
		closestNew = getClosestTrainerNew(pkmAvailable);
		statesLists.newList.trainerList[closestNew].parameters.scheduledPokemon=*pkmAvailable;
		addToReady(statesLists.newList.trainerList[closestNew]);
		removeFromNew(closestNew);
	}else if(statesLists.newList.count==0){
		closestBlocked = getClosestTrainerBlocked(pkmAvailable);
		statesLists.blockedList.trainerList[closestBlocked].parameters.scheduledPokemon=*pkmAvailable;
		addToReady(statesLists.blockedList.trainerList[closestBlocked]);
		removeFromBlocked(closestBlocked);
	}else{

		closestNew = getClosestTrainerNew(pkmAvailable);
		closestBlocked = getClosestTrainerBlocked(pkmAvailable);

		int distanceNew = getDistanceToPokemonTarget(statesLists.newList.trainerList[closestNew].parameters,*pkmAvailable);
		int distanceBlocked = getDistanceToPokemonTarget(statesLists.blockedList.trainerList[closestBlocked].parameters,*pkmAvailable);
			if(distanceNew<=distanceBlocked ){
				statesLists.newList.trainerList[closestNew].parameters.scheduledPokemon=*pkmAvailable;
				addToReady(statesLists.newList.trainerList[closestNew]);
				removeFromNew(closestNew);
			}else{
				statesLists.blockedList.trainerList[closestBlocked].parameters.scheduledPokemon=*pkmAvailable;
				addToReady(statesLists.blockedList.trainerList[closestBlocked]);
				removeFromBlocked(closestBlocked);
			}
	}
}

int getClosestTrainerNew(t_pokemon* pkmAvailable){
	int clockTimeToPokemon;
	int pos=-1;
	int clockTimeToPokemonAux;
	if(statesLists.newList.count>0){
		clockTimeToPokemon = getDistanceToPokemonTarget(statesLists.newList.trainerList[0].parameters,*pkmAvailable);
		pos = 0;
		for(int i=1;i<statesLists.newList.count;i++){
			clockTimeToPokemonAux = getDistanceToPokemonTarget(statesLists.newList.trainerList[i].parameters,*pkmAvailable);
			if(clockTimeToPokemon > clockTimeToPokemonAux){
				clockTimeToPokemon = clockTimeToPokemonAux;
				pos=i;
			}
		}
	}
	return pos;
}

int getClosestTrainerBlocked(t_pokemon* pkmAvailable){
	int clockTimeToPokemon;
	int pos = -1;
	if(getCountBlockedAvailable()>0){
		int i=getFirstBlockedAvailable();
		pos=i;
		clockTimeToPokemon = getDistanceToPokemonTarget(statesLists.blockedList.trainerList[i].parameters,*pkmAvailable);
		i++;
		int clockTimeToPokemonAux;
		for(;i<statesLists.blockedList.count;i++){
			if(statesLists.blockedList.trainerList[i].blockState==AVAILABLE){
				clockTimeToPokemonAux = getDistanceToPokemonTarget(statesLists.blockedList.trainerList[i].parameters,*pkmAvailable);
				if(clockTimeToPokemon > clockTimeToPokemonAux){
					clockTimeToPokemon = clockTimeToPokemonAux;
					pos=i;
				}
			}
		}
	}
	return pos;
}

void scheduleBydistance(int trainersCount){
	if(getCountBlockedAvailable()+statesLists.newList.count){
		int i,selectedMissingPokemonCount;
		for(int pkm=0;pkm<availablePokemons.count;pkm++){//TODO:agregar corte. encontre missing
			for(i=0; (i<missingPokemonsCount); i++){
				selectedMissingPokemonCount = 0;
				if(availablePokemons.pokemons[pkm].name == missingPkms[i].pokemon.name){
					selectedMissingPokemonCount = missingPkms[i].count;
					if(statesLists.readyList.count+getCountBlockedWaiting()+statesLists.execTrainer.boolean < missingPokemonsCount){//TODO: 4 ready+1exec+ X blocked (Catch)       5 missingpokemon

						if(statesLists.execTrainer.trainer.parameters.scheduledPokemon.name==availablePokemons.pokemons[pkm].name){
							selectedMissingPokemonCount--;
						}
						for(int j=0;j<statesLists.readyList.count;j++){
							if(statesLists.readyList.trainerList[j].parameters.scheduledPokemon.name ==availablePokemons.pokemons[pkm].name){
								selectedMissingPokemonCount--;
							}
						}
						for(int j=0;statesLists.blockedList.count;j++){
							if(statesLists.blockedList.trainerList[j].blockState==WAITING){
								if(statesLists.blockedList.trainerList[j].parameters.scheduledPokemon.name ==availablePokemons.pokemons[pkm].name){
									selectedMissingPokemonCount--;
								}
							}
						}

						if(selectedMissingPokemonCount>0){
							getClosestTrainer(&(availablePokemons.pokemons[pkm]));
						}

					}

				}break;
			}
		}
	}
}



void scheduleFifo(){
	if(statesLists.readyList.count  && statesLists.execTrainer.boolean==0){
		addToExec(statesLists.readyList.trainerList[0]);
		removeFromReady(0);
		int valueOfExecuteClock = 1;
		while(valueOfExecuteClock){
			valueOfExecuteClock = executeClock();
		}
		statesLists.execTrainer.trainer.blockState = WAITING;
		removeFromExec();
		addToBlocked(statesLists.execTrainer.trainer);
	}
}


void scheduleRR(){
	if(statesLists.readyList.count  && statesLists.execTrainer.boolean==0){
		int valueOfExecuteClock = 1;
		addToExec(statesLists.readyList.trainerList[0]);
		removeFromReady(0);
		for(int i=0;i<(int)(schedulingAlgorithm.quantum) && valueOfExecuteClock == 1;i++){
			valueOfExecuteClock = executeClock();
		}
		if(valueOfExecuteClock == 1){
			removeFromExec();
			addToReady(statesLists.execTrainer.trainer);
		}else{
			statesLists.execTrainer.trainer.blockState = WAITING;
			removeFromExec();
			addToBlocked(statesLists.execTrainer.trainer);
		}
	}
}

//TODO - DONE Si mando a EXEC un entrenador que no está en la primer posición, la lista de ready se va a actualizar mal. VER como hacer esto.
void scheduleSJFSD(){
		if(statesLists.readyList.count  && statesLists.execTrainer.boolean==0){
			int pos=getTrainerWithBestEstimatedBurst();
			float estimatedBurstTimeForCPU = estimatedTimeForNextBurstCalculation(pos);
			addToExec(statesLists.readyList.trainerList[pos]);
			removeFromReady(pos);
			int cutWhile = 1;
			if(statesLists.execTrainer.trainer.parameters.previousBurst == -1){
				statesLists.execTrainer.trainer.parameters.previousBurst = 0;
			}
			statesLists.execTrainer.trainer.parameters.previousEstimate = estimatedBurstTimeForCPU;
			while(cutWhile){
				cutWhile = executeClock();
				if(cutWhile==1){
					statesLists.execTrainer.trainer.parameters.previousBurst++;
				}
			}
			if(cutWhile == 0){
				statesLists.execTrainer.trainer.blockState = WAITING;
				removeFromExec();
				addToBlocked(statesLists.execTrainer.trainer);
			}

		}
}

//TODO
void scheduleSJFCD(){
	if(statesLists.readyList.count  && statesLists.execTrainer.boolean==0){
		int pos = getTrainerWithBestEstimatedBurst();
		float estimatedBurstTimeForCPU = estimatedTimeForNextBurstCalculation(pos);
		addToExec(statesLists.readyList.trainerList[pos]);
		removeFromReady(pos);
		int cutWhile = 1;
		if(statesLists.execTrainer.trainer.parameters.previousBurst == -1){
			statesLists.execTrainer.trainer.parameters.previousBurst = 0;
		}
		statesLists.execTrainer.trainer.parameters.previousEstimate = estimatedBurstTimeForCPU;
		while(cutWhile && (differenceBetweenEstimatedBurtsAndExecutedClocks(estimatedBurstTimeForCPU, statesLists.execTrainer.trainer.parameters.previousBurst)) <= estimatedTimeForNextBurstCalculation(getTrainerWithBestEstimatedBurst())){
			estimatedBurstTimeForCPU--;
			cutWhile = executeClock();
			if(cutWhile==1){
				statesLists.execTrainer.trainer.parameters.previousBurst++;
			}
		}
		if(cutWhile == 0){
			statesLists.execTrainer.trainer.blockState = WAITING;
			removeFromExec();
			addToBlocked(statesLists.execTrainer.trainer);
		}else{
			addToReady(statesLists.execTrainer.trainer);
			removeFromExec();
		}

	}
}


float differenceBetweenEstimatedBurtsAndExecutedClocks(float estimatedTrainerBurstTime, uint32_t executedBursts){
	float difference = estimatedTrainerBurstTime - executedBursts;
	return difference;
}

float estimatedTimeForNextBurstCalculation(int pos){
	float estimatedBurstTime;
	if(statesLists.readyList.trainerList[pos].parameters.previousBurst==-1){
		estimatedBurstTime=initialEstimatedBurst;
	}else{
		estimatedBurstTime = (alphaForSJF * statesLists.readyList.trainerList[pos].parameters.previousEstimate) + ((1 - alphaForSJF) * statesLists.readyList.trainerList[pos].parameters.previousBurst);
	}
	return estimatedBurstTime;
}

int getTrainerWithBestEstimatedBurst(){
	int pos = 0;
	int estimatedBurstTimeAux;
	int estimatedBurstTime = estimatedTimeForNextBurstCalculation(0);
	for(int i=1;i<statesLists.readyList.count;i++){
		estimatedBurstTimeAux = estimatedTimeForNextBurstCalculation(i);
		if(estimatedBurstTimeAux<estimatedBurstTime){
			estimatedBurstTime = estimatedBurstTimeAux;
			pos = i;
		}
	}
	return pos;
}


int executeClock(){

	if(getDistanceToPokemonTarget(statesLists.execTrainer.trainer.parameters,statesLists.execTrainer.trainer.parameters.scheduledPokemon)!=0){
		moveTrainerToObjective(&(statesLists.execTrainer.trainer));
		return 1;
	}else if(getDistanceToPokemonTarget(statesLists.execTrainer.trainer.parameters,statesLists.execTrainer.trainer.parameters.scheduledPokemon)==0){
		catch_pokemon catch;
		catch.pokemonName = statesLists.execTrainer.trainer.parameters.scheduledPokemon.name;
		catch.horizontalCoordinate = statesLists.execTrainer.trainer.parameters.scheduledPokemon.position.x;
		catch.verticalCoordinate = statesLists.execTrainer.trainer.parameters.scheduledPokemon.position.y;
		Send_CATCH(catch, connectBroker(broker.ip, broker.port));
		return 0;
	}
	return -1;
}


//TODO - Función que mueve al entrenador - Falta ver como implementaremos los semáforos
void moveTrainerToObjective(t_trainer* trainer){

	//t_trainerParameters* trainerToMove;
	//trainerToMove = *trainer;
	int difference_x = calculateDifference(trainer->parameters.position.x, trainer->parameters.scheduledPokemon.position.x);
	int difference_y = calculateDifference(trainer->parameters.position.y, trainer->parameters.scheduledPokemon.position.y);
	//TODO:meter semáforos acá
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


int getDistanceToPokemonTarget(t_trainerParameters trainer,  t_pokemon targetPokemon){
	int distanceInX = calculateDifference(trainer.position.x, targetPokemon.position.x);
	int distanceInY = calculateDifference(trainer.position.y, targetPokemon.position.y);
	int distance = getClockTimeToNewPosition(distanceInX, distanceInY);
	return distance;
}

int connectBroker(char* ip, char* port)
{
	int teamSocket;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    getaddrinfo(ip, port, &hints, &servinfo);
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
