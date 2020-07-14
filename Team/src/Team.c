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
t_pokemonList availablePokemons;
sem_t* availablePokemons_sem = 1;
t_idMessages getList;
t_catchMessages catchList;
t_objetive* missingPkms;
int globalObjetivesDistinctCount;
int globalObjetivesCount;
int missingPokemonsCount;//TODO: Decrementar cada vez que hay un caught


int main(void) {
	t_config* config;
	int teamSocket;
	char* ip;
	char* port;
	t_trainer* l_new;
	t_ready_trainers* list_ready;
	t_ready_trainers* list_exec;
	t_trainer* l_blocked;
	t_trainer* l_exit;
	t_stateLists stateLists;
	int trainersCount,readyCount,execCount;
    pthread_t* subs;

	localized_pokemon* localized;
	appeared_pokemon* appeared;


    globalObjetivesDistinctCount=0;
    globalObjetivesCount=0;
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
	missingPokemons(l_new,trainersCount,logger);
	void* temp = realloc(missingPkms,sizeof(t_objetive)*globalObjetivesDistinctCount);
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	missingPkms=temp;
	missingPokemonsCount = globalObjetivesDistinctCount;

	subs=(pthread_t*)malloc(sizeof(pthread_t)*3);
	subscribeToBroker(broker,subs);//TODO ver si esto no conviene hacerlo con select (Falta ver con Marcos)
	requestNewPokemons(missingPkms,globalObjetivesDistinctCount,logger,broker);
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
	initStateLists(stateLists,l_new,l_blocked,list_ready,list_exec,l_exit);
    //startClosePlanning(new,blocked,ready);
	//startReadyPlaning(ready,exec);
	//TODO agregar aquí la función "scheduleByDistance"
	int teamServer = startServer(logger);
	int client;
	while(1){
		client = waitClient(teamServer);
		log_debug(logger,"Gameboy connected");
		pthread_create(thread,NULL,(void*)attendGameboy,&client);
		pthread_detach(*thread);
	}
	deleteLogger(&logger);
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
void initStateLists(t_stateLists stateLists,t_trainer* new, t_trainer* blocked,t_ready_trainers* ready, t_ready_trainers* exec, t_trainer* l_exit){
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

int findIdInGetList(uint32_t cid){
	int position=-1;
	for(int i=0;i<getList.count;i++){
		int compare = memcmp(getList.id[i],cid,sizeof(uint32_t));
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
			//TODO: scheduleLargoPlazo;
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
	appearedPokemon->pokemonName;
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
			//TODO: scheduleLargoPlazo;
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
		;//TODO: Borrar de missingPokemon. Decrementar missingPokemonCount.Ver si entrenador pasa a EXIT; sino, cambiar blockstate de WAITING a AVAILABLE.
		}else{
			catchList.catchMessage[resultCatchId].trainer.blockState = AVAILABLE;
		//TODO: ACá habría que chequear si hay pokemons disponibles para atrapar, si Sí, schedule; si no, fin de la función.
		}
	}
}


void requestNewPokemons(t_objetive* pokemons,int globalObjetivesDistinctCount,t_log* logger,struct Broker broker){
	getList.id = 0;
	log_debug(logger,"Se solicitarán %i pokemons",globalObjetivesDistinctCount);
	for(int obj=0;obj<globalObjetivesDistinctCount;obj++){
		requestNewPokemon(pokemons[obj].pokemon,logger,broker);
	}
}

//TODO debería usar la shared cuando este implementado para mandar.
void requestNewPokemon(t_pokemon missingPkm,t_log* logger, struct Broker broker){
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


void missingPokemons(t_trainer* trainers, int trainersCount,t_log* logger){
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

void schedule(t_ready_trainers* trainers,int* readyCount,struct SchedulingAlgorithm schedulingAlgorithm,t_ready_trainers* exec, t_log* logger){//Para el caso de FIFO y RR no hace nada, ya que las listas están ordenadas por FIFO y RR solo cambia como se procesa.
	if (strcmp(schedulingAlgorithm.algorithm,"FIFO")==0){
		scheduleFifo(trainers,readyCount,schedulingAlgorithm, exec, logger);
	}else if(strcmp(schedulingAlgorithm.algorithm,"RR")==0){
		scheduleRR(trainers,readyCount,schedulingAlgorithm, exec, logger);
	}else if(strcmp(schedulingAlgorithm.algorithm,"SJF-SD")==0){
		scheduleSJFSD(trainers,readyCount,schedulingAlgorithm, exec, logger);
	}else if(strcmp(schedulingAlgorithm.algorithm,"SJF-CD")==0){
		scheduleSJFCD(trainers,readyCount,schedulingAlgorithm, exec, logger);
	}
}

void addToReady(t_ready_trainers* trainer,t_ready_trainers* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm,t_log* logger, t_ready_trainers* exec){
	void* temp = realloc(trainers,sizeof(t_trainer)*((*countReady)+1));
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	(trainers)=temp;
	sem_wait(countReady_semaphore);
	(trainers)[(*countReady)]=(*trainer);
	(*countReady)++;
	sem_post(countReady_semaphore);
	schedule(trainers,countReady,schedulingAlgorithm,exec, logger);
}



void addToExec(t_ready_trainers* ready,int* countReady,t_ready_trainers* exec,t_log* logger){
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


//TODO - esta función debe generar el listado de ready ordenado por distancia. Hay que ver como eva
void scheduleBydistance(localized_pokemon* localized, appeared_pokemon* appeared, t_trainer* l_new, int trainersCount){
	appeared_pokemon* pokemonsToScheduleByDistance=(appeared_pokemon*)malloc(sizeof(appeared_pokemon));
	appeared_pokemon* listOfPokemons=(appeared_pokemon*)malloc(sizeof(appeared_pokemon)*(localized->ammount));
	t_ready_trainers* toReady=(t_ready_trainers*)malloc(sizeof(t_ready_trainers));
	int i=0;
	while(localized->ammount){
		localized->ammount--;
		pokemonsToScheduleByDistance->pokemonName = localized->pokemonName;
		pokemonsToScheduleByDistance->horizontalCoordinate = localized->coordinates->x;
		pokemonsToScheduleByDistance->horizontalCoordinate = localized->coordinates->y;
		i++;
		((&listOfPokemons)[i]) = pokemonsToScheduleByDistance;
	}

	//cómo agregar los de la lista de appeared??

	while(i){
		int j;
		int minClockTime = 0;
		for(j=0;j<trainersCount;j++){
			int clockTime;
			clockTime = getDistanceToPokemonTarget(l_new[j].parameters,((&listOfPokemons)[i]));
			if(minClockTime == 0 || minClockTime > clockTime){

				// cómo evaluar las dos listas completas, ya que si empiezo a recorrer por entrenador, debo fijarme que este sea el más cercano a este pokemon, y si lo hago
				// por pokemones debería evaluar esto también.
			}
		}

	}
}


//TODO - No debería hacer nada; siempre se agregan cosas al final de ready y se sacan del HEAD de ready
void scheduleFifo(t_ready_trainers* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm, t_ready_trainers* exec, t_log* logger){

}

//TODO - cuando termina el quantum mandar al final de la lista de ready.
void scheduleRR(t_ready_trainers* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm, t_ready_trainers* exec, t_log* logger){
	while(*countReady){
		int i=0;
		int valueOfExecuteClock = 1;
		t_ready_trainers* trainer;
		trainer = ((&trainers)[i]);
		addToExec(trainer, countReady, exec, logger);
		for(int j=0;j<=(int)(schedulingAlgorithm.quantum) && valueOfExecuteClock == 1;j++){
			valueOfExecuteClock = executeClock(*countReady, exec);
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
void scheduleSJFSD(t_ready_trainers* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm, t_ready_trainers* exec, t_log* logger){
;
}

//TODO
void scheduleSJFCD(t_ready_trainers* trainers,int* countReady,struct SchedulingAlgorithm schedulingAlgorithm, t_ready_trainers* exec, t_log* logger){
;
}



int executeClock(int countReady, t_ready_trainers* exec){

	appeared_pokemon* appeared;
	appeared->pokemonName=exec->pokemon->name;
	appeared->horizontalCoordinate=exec->pokemon->position.x;
	appeared->horizontalCoordinate=exec->pokemon->position.y;

	if(getDistanceToPokemonTarget(exec->trainer->parameters,appeared)!=0){
		moveTrainerToObjective(exec->trainer, exec->pokemon);
		return 1;
	}else if(getDistanceToPokemonTarget(exec->trainer->parameters,appeared)==0){
		catch_pokemon catch;
		catch.pokemonName = exec->pokemon->name;
		catch.horizontalCoordinate = exec->pokemon->position.x;
		catch.verticalCoordinate = exec->pokemon->position.y;
		Send_CATCH(catch, connectBroker(broker.ip, broker.port,logger));
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


int getDistanceToPokemonTarget(t_trainerParameters trainer,  appeared_pokemon* targetPokemon){
	int distanceInX = calculateDifference(trainer.position.x, targetPokemon->horizontalCoordinate);
	int distanceInY = calculateDifference(trainer.position.y, targetPokemon->verticalCoordinate);
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
