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
sem_t catch_semaphore;
sem_t countReady_semaphore;
sem_t countBlocked_semaphore;
sem_t countNew_semaphore;
sem_t countExit_semaphore;
t_pokemonList availablePokemons;
sem_t availableTrainersCount_sem;
sem_t availablePokemonsCount_sem;
sem_t availablePokemons_mutex;
sem_t availablePokemons_sem;
sem_t readyTrainer_sem;
sem_t execTrainer_sem;
sem_t deadlockCount_sem;
sem_t exitCount_sem;
t_idMessages getList;
t_catchMessages catchList;
t_objetive* missingPkms;
int globalObjetivesDistinctCount;
int globalObjetivesCount;
int missingPokemonsCount;//TODO: Decrementar cada vez que hay un caught
struct SchedulingAlgorithm schedulingAlgorithm;
double alphaForSJF;
int initialEstimatedBurst;
char* RR = "RR";
char* SJFCD = "SJF-CD";
char* SJFSD = "SJF-SD";
char* FIFO = "FIFO";
int retryConnectionTime=0;
int connectedToBroker=0;
int trainersCount=0;
pthread_t* subs;
int clockSimulationTime;
int deadlockCount = 0;
int solvedDeadlocks = 0;
int cpuClocksCount = 0;
int switchContextCount = 0;


int main(void) {
	t_config* config;
	int teamSocket;
	char* ip;
	char* port;
	int trainersCount;
	catchList.count=0;
	catchList.catchMessage = (t_catchMessage*)malloc(sizeof(t_catchMessage));
	getList.count=0;
	getList.id = (uint32_t*)malloc(sizeof(uint32_t));


    availablePokemons.pokemons=(t_pokemon*)malloc(sizeof(t_pokemon));
    sem_init(&(catch_semaphore),0,1);
    sem_init(&(countReady_semaphore),0,1);
    sem_init(&(countBlocked_semaphore),0,1);
    sem_init(&(countNew_semaphore),0,1);
    sem_init(&(countExit_semaphore),0,1);
    sem_init(&(availablePokemons_sem),0,1);
    sem_init(&(availablePokemonsCount_sem),0,0);
    sem_init(&(readyTrainer_sem),0,0);
    sem_init(&(execTrainer_sem),0,1);
    sem_init(&(availablePokemons_mutex),0,1);
    initializeLists();



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
	initTeamServer();
	readConfigBrokerValues(config,&broker);
	readConfigTeamValues(config);
	readConfigReconnectWaiting(config);
	readConfigClockSimulationTime(config);


	//Init de scheduler
	initScheduler(&schedulingAlgorithm);
	readConfigSchedulerValues(config,&schedulingAlgorithm);//TODO agregar validaciones para que sólo se acepten los algoritmos especificos - DONE en líne de código 657 a 661.

	//Init Alpha for SJF and Initial Estimated Burst
	alphaForSJF = readConfigAlphaValue(config);
	log_debug(logger, "alpha valor %f", alphaForSJF);
	initialEstimatedBurst = readConfigInitialEstimatedValue(config);
/*
	teamSocket = iniciar_cliente(ip, port,logger);
	log_debug(logger,"El socket del proceso Team es %i",teamSocket);
	enviar_mensaje(mensaje,strlen(mensaje),teamSocket);
	serve_client(&teamSocket,logger);
*/
	trainersCount = getTrainersCount(config);
	sem_init(&(availableTrainersCount_sem),0,trainersCount);
	sem_init(&(deadlockCount_sem),0,1-trainersCount);
	sem_init(&(exitCount_sem),0,1-trainersCount);
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
	subscribeToBroker();//TODO ver si esto no conviene hacerlo con select (Falta ver con Marcos)
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

	pthread_create(thread,NULL,(void*)startCloseScheduling,NULL);
	pthread_detach(*thread);
	pthread_create(thread,NULL,(void*)startAlgorithmScheduling,NULL);
	pthread_detach(*thread);
	pthread_create(thread,NULL,(void*)resolveDeadlock,NULL);
	pthread_detach(*thread);
	pthread_create(thread,NULL,(void*)finishTeam,NULL);
	pthread_detach(*thread);
	log_debug(logger,"se iniciará el aceptamiento de mensajes de la gameboy, fiera");
	int teamServer = startServer();
	log_debug(logger,"se inició el server");
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


void* resolveDeadlock(){
	sem_wait(&(deadlockCount_sem));
	for(int trainerPos=0;trainerPos<statesLists.blockedList.count;trainerPos++){
		if(statesLists.blockedList.trainerList[trainerPos].blockState==DEADLOCK){
			for(int trainerPos2=trainerPos+1;trainerPos2<statesLists.blockedList.count;trainerPos2++){
				if(statesLists.blockedList.trainerList[trainerPos2].blockState==DEADLOCK){


					int distinctPkmCount;
					int distinctPkmCount2;
					int distinctPkmCount3;
					for(int objPos=0;objPos<statesLists.blockedList.trainerList[trainerPos].parameters.objetivesCount;objPos++){
						log_debug(logger,"se comparán %i objetivos y %i pokemons atrapados ",statesLists.blockedList.trainerList[trainerPos].parameters.objetivesCount,statesLists.blockedList.trainerList[trainerPos].parameters.pokemonsCount);
						distinctPkmCount=0;
						for(int objPosAux=0;objPosAux<statesLists.blockedList.trainerList[trainerPos].parameters.objetivesCount;objPosAux++){
							log_debug(logger,"1 se compará %s con %s ",statesLists.blockedList.trainerList[trainerPos].parameters.objetives[objPos].name,statesLists.blockedList.trainerList[trainerPos].parameters.objetives[objPosAux].name);
							if(0==strcmp(statesLists.blockedList.trainerList[trainerPos].parameters.objetives[objPos].name,statesLists.blockedList.trainerList[trainerPos].parameters.objetives[objPosAux].name)){
								distinctPkmCount++;
							}
						}
						for(int pkmPos=0;pkmPos<statesLists.blockedList.trainerList[trainerPos].parameters.pokemonsCount;pkmPos++){
							log_debug(logger,"2 se compará %s con %s ",statesLists.blockedList.trainerList[trainerPos].parameters.objetives[objPos].name,statesLists.blockedList.trainerList[trainerPos].parameters.pokemons[pkmPos].name);
							if(0==strcmp(statesLists.blockedList.trainerList[trainerPos].parameters.objetives[objPos].name,statesLists.blockedList.trainerList[trainerPos].parameters.pokemons[pkmPos].name)){
								distinctPkmCount--;
							}
						}
						if(distinctPkmCount>0){

							for(int pkmPos2=0;pkmPos2<statesLists.blockedList.trainerList[trainerPos2].parameters.pokemonsCount;pkmPos2++){
								if(0==strcmp(statesLists.blockedList.trainerList[trainerPos].parameters.objetives[objPos].name,statesLists.blockedList.trainerList[trainerPos2].parameters.pokemons[pkmPos2].name)){
									log_debug(logger,"se comparán %i objetivos y %i pokemons atrapados ",statesLists.blockedList.trainerList[trainerPos2].parameters.pokemonsCount,statesLists.blockedList.trainerList[trainerPos2].parameters.pokemonsCount);
									distinctPkmCount2=0;
									for(int pkmPos2Aux=0;pkmPos2Aux<statesLists.blockedList.trainerList[trainerPos2].parameters.pokemonsCount;pkmPos2Aux++){
										log_debug(logger,"1 se compará %s con %s ",statesLists.blockedList.trainerList[trainerPos2].parameters.pokemons[pkmPos2].name,statesLists.blockedList.trainerList[trainerPos2].parameters.pokemons[pkmPos2Aux].name);
										if(0==strcmp(statesLists.blockedList.trainerList[trainerPos2].parameters.pokemons[pkmPos2].name,statesLists.blockedList.trainerList[trainerPos2].parameters.objetives[pkmPos2Aux].name)){
											distinctPkmCount2++;
										}
									}
									for(int objPos2=0;objPos2<statesLists.blockedList.trainerList[trainerPos].parameters.objetivesCount;objPos2++){
										log_debug(logger,"2 se compará %s con %s ",statesLists.blockedList.trainerList[trainerPos2].parameters.pokemons[pkmPos2].name,statesLists.blockedList.trainerList[trainerPos2].parameters.objetives[objPos2].name);
										if(0==strcmp(statesLists.blockedList.trainerList[trainerPos2].parameters.pokemons[pkmPos2].name,statesLists.blockedList.trainerList[trainerPos2].parameters.objetives[objPos2].name)){
											distinctPkmCount2--;
										}
									}
									if(distinctPkmCount2>0){
										//busco el pokemon que le sobra a 1
										for(int i=0;i<statesLists.blockedList.trainerList[trainerPos].parameters.objetivesCount;i++){
											log_debug(logger,"se comparán %i objetivos y %i pokemons atrapados ",statesLists.blockedList.trainerList[trainerPos].parameters.objetivesCount,statesLists.blockedList.trainerList[trainerPos].parameters.pokemonsCount);
											distinctPkmCount3=0;
											for(int j=0;j<statesLists.blockedList.trainerList[trainerPos].parameters.objetivesCount;j++){
												log_debug(logger,"1 se compará %s con %s ",statesLists.blockedList.trainerList[trainerPos].parameters.objetives[i].name,statesLists.blockedList.trainerList[trainerPos].parameters.objetives[j].name);
												if(0==strcmp(statesLists.blockedList.trainerList[trainerPos].parameters.objetives[i].name,statesLists.blockedList.trainerList[trainerPos].parameters.objetives[j].name)){
													distinctPkmCount3++;
												}
											}
											for(int k=0;k<statesLists.blockedList.trainerList[trainerPos].parameters.pokemonsCount;k++){
												log_debug(logger,"2 se compará %s con %s ",statesLists.blockedList.trainerList[trainerPos].parameters.objetives[i].name,statesLists.blockedList.trainerList[trainerPos].parameters.pokemons[k].name);
												if(0==strcmp(statesLists.blockedList.trainerList[trainerPos].parameters.objetives[i].name,statesLists.blockedList.trainerList[trainerPos].parameters.pokemons[k].name)){
													distinctPkmCount3--;
												}
											}
											if(distinctPkmCount3<0){
												statesLists.blockedList.trainerList[trainerPos].parameters.scheduledTrainerId=statesLists.blockedList.trainerList[trainerPos2].id;
												statesLists.blockedList.trainerList[trainerPos].parameters.scheduledPokemon = statesLists.blockedList.trainerList[trainerPos].parameters.pokemons[i];
												statesLists.blockedList.trainerList[trainerPos2].parameters.scheduledPokemon = statesLists.blockedList.trainerList[trainerPos].parameters.objetives[objPos];
												planificateDeadlockTrainer(&(statesLists.blockedList.trainerList[trainerPos]));
												if(checkTrainerState(statesLists.blockedList.trainerList[trainerPos])==0){
													addToExit(statesLists.blockedList.trainerList[trainerPos]);
													removeFromExec();
												}
												if(checkTrainerState(statesLists.blockedList.trainerList[trainerPos2])==0){
													addToExit(statesLists.blockedList.trainerList[trainerPos2]);
													removeFromBlocked(trainerPos2);
												}
												pthread_create(thread,NULL,(void*)resolveDeadlock,NULL);
												pthread_detach(*thread);
												pthread_exit(NULL);

											}
										}

									}
								}
							}
						}
					}
				}
			}
		}
	}
	//TODO: RESOLVER DEADLOCKS
	//TODO: Por cada entrenador que se resueve, sem_post(&exitCount_sem);
	//TODO: Cada vez que se resuelve un deadlocl -> solvedDeadlocks++;
	pthread_exit(NULL);
}

void planificateDeadlockTrainer(t_trainer* trainer){

	addToReady(trainer);
	sem_post(&(readyTrainer_sem));

}

void exchangePokemon(t_trainer* trainer){
	int positionPkmTrainerOne, positionPkmTrainerTwo;
	for(int i=0;trainer->parameters.objetivesCount;i++){
		if(0==strcmp(
				trainer->parameters.scheduledPokemon.name,
				trainer->parameters.pokemons[i].name))
		{
			positionPkmTrainerOne = i;
			break;
		}
	}
	for(int j=0; statesLists.blockedList.trainerList[trainer->parameters.scheduledTrainerId].parameters.objetivesCount;j++){
		if(0==strcmp(
				statesLists.blockedList.trainerList[trainer->parameters.scheduledTrainerId].parameters.scheduledPokemon.name,
				statesLists.blockedList.trainerList[trainer->parameters.scheduledTrainerId].parameters.pokemons[j].name
				))
		{
			positionPkmTrainerTwo = j;
			break;
		}
	}

	char* auxPkmName = trainer->parameters.pokemons[positionPkmTrainerOne].name;

	trainer->parameters.pokemons[positionPkmTrainerOne].name = statesLists.blockedList.trainerList[trainer->parameters.scheduledTrainerId].parameters.pokemons[positionPkmTrainerTwo].name;

	statesLists.blockedList.trainerList[trainer->parameters.scheduledTrainerId].parameters.pokemons[positionPkmTrainerTwo].name = auxPkmName;

	trainer->parameters.scheduledPokemon.name = "NULL";
	statesLists.blockedList.trainerList[trainer->parameters.scheduledTrainerId].parameters.scheduledPokemon.name = "NULL";
}

int getDistanceToTrainerToExchange(t_trainer trainerGoing){
	int distanceInX = calculateDifference(trainerGoing.parameters.position.x, statesLists.blockedList.trainerList[trainerGoing.parameters.scheduledTrainerId].parameters.position.x);
	int distanceInY = calculateDifference(trainerGoing.parameters.position.y, statesLists.blockedList.trainerList[trainerGoing.parameters.scheduledTrainerId].parameters.position.y);
	int distance = getClockTimeToNewPosition(distanceInX, distanceInY);
	log_debug(logger,"Entrenador: %i en X=%i Y=%i, Entrenador a ir a intercambiar: %i en X=%i Y=%i distancia %i",
			trainerGoing.id,
			trainerGoing.parameters.position.x,
			trainerGoing.parameters.position.y,
			trainerGoing.parameters.scheduledTrainerId,
			statesLists.blockedList.trainerList[trainerGoing.parameters.scheduledTrainerId].parameters.position.x,
			statesLists.blockedList.trainerList[trainerGoing.parameters.scheduledTrainerId].parameters.position.y,
			distance);
	return distance;

}

void moveTrainerToObjectiveDeadlock(t_trainer* trainer){
	int difference_x = calculateDifference(trainer->parameters.position.x, trainer->parameters.scheduledPokemon.position.x);
	int difference_y = calculateDifference(trainer->parameters.position.y, trainer->parameters.scheduledPokemon.position.y);
	moveTrainerToTarget(trainer, difference_x, difference_y);
}


void* finishTeam(){
	sem_wait(&exitCount_sem);
	log_error(logger,"-----Proceso Finalizado-----");
	log_error(logger,"Cantidad de ciclos de CPU totales: %i",cpuClocksCount);
	log_error(logger,"Cantidad de cambios de contesto realizados: %i",switchContextCount);
	for(int i = 0;i<statesLists.exitList.count;i++){
		log_error(logger,"Cantidad de ciclos de CPU totales por entrenador: Trainer %u -> %i",statesLists.exitList.trainerList[i].id,statesLists.exitList.trainerList[i].parameters.cpuClocksCount);
	}
	log_error(logger,"Deadlocks producidos: %i",deadlockCount);
	log_error(logger,"Deadlocks resueltos: %i",solvedDeadlocks);
	return EXIT_SUCCESS;
}

void* startCloseScheduling(){
	while(1){

		sem_wait(&(availableTrainersCount_sem));
		sem_wait(&(availablePokemonsCount_sem));
		sem_wait(&availablePokemons_sem);
		log_debug(logger,"Se comienza a planificar planificador por cercanía");
		scheduleByDistance();
		log_debug(logger,"Se termina de planificar planificador por cercania");
		sem_post(&availablePokemons_sem);
	}
}

void* startAlgorithmScheduling(){
	while(1){

		sem_wait(&(readyTrainer_sem));
		sem_wait(&(execTrainer_sem));
		log_debug(logger,"Se comienza a planificar la lista de ready");
		schedule();
		log_debug(logger,"Se termina de planificar la lista de ready");
	}
}





void initializeLists(){
	statesLists.blockedList.count = 0;
	statesLists.readyList.count = 0;
	statesLists.newList.count = 0;
	statesLists.exitList.count = 0;
	statesLists.execTrainer.boolean = 0;
}

void initTrainersPokemonLists(){
	for(int i=0;i<trainersCount;i++){
		statesLists.newList.trainerList[i].parameters.pokemonsCount=0;
		statesLists.newList.trainerList[i].parameters.objetivesCount=0;
	}

}

void readConfigClockSimulationTime(t_config* config){
	if (config_has_property(config,"RETARDO_CICLO_CPU")){
		clockSimulationTime=config_get_int_value(config,"RETARDO_CICLO_CPU");
	}else{
		exit(-3);
	}
}


void readConfigReconnectWaiting(t_config* config){
	if (config_has_property(config,"TIEMPO_RECONEXION")){
		retryConnectionTime=config_get_int_value(config,"TIEMPO_RECONEXION");
	}else{
		exit(-3);
	}
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
	log_debug(logger,"processGameBoyMessage");
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

int startServer()
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
    log_debug(logger,"IP %s y puerto %s configurado",teamServerAttr.ip, teamServerAttr.port);
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


int getGlobalObjetivesCount(t_trainer* trainers, int trainersCount){
	int objcount = 0;
	for(int trainer=0;trainer<trainersCount;trainer++){
		objcount = objcount+trainers[trainer].parameters.objetivesCount;
	}
	return objcount;
}

void subscribeToBroker(){

	pthread_create(&(subs[0]),NULL,subscribeToBrokerCaught,NULL);
	pthread_create(&(subs[1]),NULL,subscribeToBrokerAppeared,NULL);
	pthread_create(&(subs[2]),NULL,subscribeToBrokerLocalized,NULL);
}

void* subscribeToBrokerLocalized(){
	log_debug(logger,"Creando thread Localized Subscriptions Handler");
	int socketLocalized;
	int flagConnected = 0;

	while(!flagConnected){
		socketLocalized = connectBroker(broker.ip,broker.port);
		if(socketLocalized!=-1){
			if (-1==SendSubscriptionRequest(LOCALIZED_POKEMON,socketLocalized)){
				log_debug(logger,"Error en subscripcion de Localized");
			}else{
				log_debug(logger,"Se subscribió a Localized");
			}
		}else{
			log_debug(logger,"Se intentará reconectarse al broker para subscribirse a Localized cada %i",retryConnectionTime);
			sleep(retryConnectionTime);
			pthread_create(&(subs[2]),NULL,subscribeToBrokerLocalized,NULL);
			pthread_detach(subs[2]);
			pthread_exit(NULL);
		}
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
	int socketAppeared;
	int flagConnected = 0;

	while(!flagConnected){
		socketAppeared = connectBroker(broker.ip,broker.port);
		if(socketAppeared!=-1){
			if (-1==SendSubscriptionRequest(APPEARED_POKEMON,socketAppeared)){
				log_debug(logger,"Error en subscripcion de Localized");
			}else{
				log_debug(logger,"Se subscribió a Localized");
			}
		}else{
			log_debug(logger,"Se intentará reconectarse al broker para subscribirse a Appeared cada %i",retryConnectionTime);
			sleep(retryConnectionTime);
			pthread_create(&(subs[1]),NULL,subscribeToBrokerAppeared,NULL);
			pthread_detach(*thread);
			pthread_exit(NULL);
		}
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
	int socketCaught;
	int flagConnected = 0;

	while(!flagConnected){
		socketCaught = connectBroker(broker.ip,broker.port);
		if(socketCaught!=-1){
			if (-1==SendSubscriptionRequest(CAUGHT_POKEMON,socketCaught)){
				log_debug(logger,"Error en subscripcion de caught");
			}else{
				log_debug(logger,"Se subscribió a Caught");
			}
		}else{
			log_debug(logger,"Se intentará reconectarse al broker para subscribirse a Caught cada %i",retryConnectionTime);
			sleep(retryConnectionTime);
			pthread_create(&(subs[0]),NULL,subscribeToBrokerCaught,NULL);
			pthread_detach(*thread);
			pthread_exit(NULL);
		}
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


	op_code type;
	void* content = malloc(sizeof(void*));

	int resultado= RecievePackage(suscription,&type,&content);

	if (!resultado)
	{
		deli_message* message = (deli_message*)content;

		t_args_process_message* argsProcessMessage= (t_args_process_message*) malloc (sizeof (t_args_process_message));
		argsProcessMessage->message = message;

		pthread_create(thread,NULL,(void*)processMessage,argsProcessMessage);
		thread = (pthread_t*)malloc(sizeof(pthread_t));

		pthread_create(thread,NULL,(void*)waitForMessage,args);
		pthread_detach(*thread);

	}
	else{
		connectedToBroker = 0;
		switch (queueType) {

			case LOCALIZED_POKEMON: {
				subscribeToBrokerLocalized();
				break;
			}
			case APPEARED_POKEMON:{
				subscribeToBrokerAppeared();
				break;
			}
			case CAUGHT_POKEMON:{
				subscribeToBrokerCaught();
				break;
			}
		}
	}
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
		sem_wait(&availablePokemons_sem);
		void* temp = realloc(availablePokemons.pokemons,sizeof(t_pokemon)*(availablePokemons.count+localizedPokemon->ammount));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(9);
		}
		availablePokemons.pokemons=temp;
		for(int i=0;i<localizedPokemon->ammount;i++){
			availablePokemons.pokemons[availablePokemons.count].name=malloc(strlen(localizedPokemon->pokemonName)+1);
			availablePokemons.pokemons[availablePokemons.count].name=localizedPokemon->pokemonName;
			availablePokemons.pokemons[availablePokemons.count].position.x=localizedPokemon->coordinates->x;
			availablePokemons.pokemons[availablePokemons.count].position.y=localizedPokemon->coordinates->y;
			availablePokemons.count++;
			sem_post(&availablePokemons_sem);
			sem_post(&(availablePokemonsCount_sem));
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
	log_debug(logger,"Before AddMessageAppeared: availablePokemonCount: %i",availablePokemons.count);
	appeared_pokemon* appearedPokemon = (appeared_pokemon*)message->messageContent;
	int resultMissingPokemon = findNameInMissingPokemons(appearedPokemon->pokemonName);
		if(resultMissingPokemon==1){//TODO: Agregar en el planificador que valide si ya hay un entrenador planificado para ese pokemon que llego por otro appeared o localized
			sem_wait(&availablePokemons_sem);
			void* temp = realloc(availablePokemons.pokemons,sizeof(t_pokemon)*(availablePokemons.count+1));
			if (!temp){
				log_debug(logger,"error en realloc");
				exit(9);
			}
			availablePokemons.pokemons=temp;
			availablePokemons.pokemons[availablePokemons.count].name=(char*)malloc(strlen(appearedPokemon->pokemonName)+1);
			availablePokemons.pokemons[availablePokemons.count].name=appearedPokemon->pokemonName;
			availablePokemons.pokemons[availablePokemons.count].position.x=appearedPokemon->horizontalCoordinate;
			availablePokemons.pokemons[availablePokemons.count].position.y=appearedPokemon->verticalCoordinate;
			availablePokemons.count++;
			log_debug(logger,"After processMessageAppeared: Pokemon sirve availablePokemonCount: %i",availablePokemons.count);
			sem_post(&availablePokemons_sem);
			sem_post(&(availablePokemonsCount_sem));
		}else{
			log_debug(logger,"After AddMessageAppeared: Pokemon no sirve availablePokemonCount: %i",availablePokemons.count);
		}

}

int findIdInCatchList(uint32_t cid){
	int position=-1;
	for(int i=0;i<catchList.count;i++){
		int compare = memcmp(catchList.catchMessage[i].catchId,cid,sizeof(uint32_t));
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
	uint32_t trainerPos;
	if(resultCatchId>=0){
		for(int i = 0;i<statesLists.blockedList.count;i++){
			if(statesLists.blockedList.trainerList[i].id==resultCatchId){
				trainerPos=i;;
			}
		}
		removeFromMissingPkms(statesLists.blockedList.trainerList[trainerPos].parameters.scheduledPokemon);
		addToPokemonList(&(statesLists.blockedList.trainerList[trainerPos]));
		log_debug(logger,"Se captura el pokemon por Caught message");

		if(statesLists.blockedList.trainerList[trainerPos].parameters.objetivesCount==statesLists.blockedList.trainerList[trainerPos].parameters.pokemonsCount){
			if(checkTrainerState(statesLists.blockedList.trainerList[trainerPos])==1){
				statesLists.blockedList.trainerList[trainerPos].blockState = DEADLOCK;
				deadlockCount++;
				sem_post(&(deadlockCount_sem));
			}else{
				addToExit(statesLists.execTrainer.trainer);
				removeFromBlocked(trainerPos);
				deadlockCount++;
				sem_post(&(deadlockCount_sem));
			}
		}else{
			statesLists.execTrainer.trainer.blockState = AVAILABLE;
			sem_post(&(availableTrainersCount_sem));
		}
	}else{
			//catchList.catchMessage[resultCatchId].trainer.blockState = AVAILABLE;
		//TODO: ACá habría que chequear si hay pokemons disponibles para atrapar, si Sí, ; si no, fin de la función.SACAR PKM de scheduledPokemon
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
	log_debug(logger,"Se solicitará el pokemon %s", missingPkm.name);
	int socketGet = connectBroker(broker.ip, broker.port);
	if(socketGet != -1){
		get_pokemon get;
		get.pokemonName=(char*)malloc(strlen(missingPkm.name)+1);
		strcpy(get.pokemonName,missingPkm.name);
		log_debug(logger,"Se enviará el send para el pokemon %s", missingPkm.name);
		Send_GET(get, socketGet);
		sleep(clockSimulationTime);
		log_debug(logger,"Pokemon requested: %s",missingPkm.name);


		op_code type;
		void* content = malloc(sizeof(void*));
		int result;
		int cut = 0;
		while(cut != 1){
			result = RecievePackage(socketGet,&type,&content);
			if(type == ACKNOWLEDGE){
				cut=1;
			}
		}
		uint32_t originMessageType = GET_POKEMON;
		if(!result){
				processAcknowledge(content,originMessageType,statesLists.execTrainer.trainer.id);
		}
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
				globalObjetivesDistinctCount++;
			}else{
				int added=0;
				for(int objCmp=0;objCmp<globalObjetivesDistinctCount;objCmp++){
					if(0==strcmp(missingPkms[objCmp].pokemon.name,trainers[trainer].parameters.objetives[obj].name)){
						added=1;
						missingPkms[objCmp].count++;
					}

				}
				if(added==0){
					missingPkms[globalObjetivesDistinctCount].pokemon=trainers[trainer].parameters.objetives[obj];
					missingPkms[globalObjetivesDistinctCount].count++;
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
							for(int new=total;new<globalObjetivesDistinctCount;new++){
								missingPkms[new]=missingPkms[new+1];
							}
							globalObjetivesCount--;
							globalObjetivesDistinctCount--;
						}else{
							missingPkms[total].count--;
							globalObjetivesCount--;
						}
						total=globalObjetivesCount;
					}
				}
			}
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
	*config = config_create("/home/utnso/workspace/tp-2020-1c-MATE-OS/Team/Team.config");
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

void readConfigTeamValues(t_config *config){
	log_debug(logger,"2. Comienza lectura de config de teamAttr");
	if (config_has_property(config,teamServerAttr.ipKey)){
		teamServerAttr.ip=config_get_string_value(config,teamServerAttr.ipKey);
		log_debug(logger,"2. Se leyó la IP: %s",teamServerAttr.ip);
	}else{
		exit(-3);
	}

	if (config_has_property(config,teamServerAttr.portKey)){
		teamServerAttr.port=config_get_string_value(config,teamServerAttr.portKey);
		log_debug(logger,"2. Se leyó el puerto: %s",teamServerAttr.port);
	}else{
		exit(-3);
	}
	log_debug(logger,"2. Finaliza lectura de config de teamAttr");
}

void readConfigSchedulerValues(t_config *config,struct SchedulingAlgorithm *schedulingAlgorithm){
	log_debug(logger,"3. Comienza lectura de config de scheduler");
	if (config_has_property(config,schedulingAlgorithm->algorithmKey)){
		schedulingAlgorithm->algorithm=config_get_string_value(config,schedulingAlgorithm->algorithmKey);
		if(schedulingAlgorithm->algorithm==SJFCD || schedulingAlgorithm->algorithm==FIFO || schedulingAlgorithm->algorithm==SJFSD || schedulingAlgorithm->algorithm==RR){
			log_debug(logger,"3. Algoritmo de planificación reconocido: %s", schedulingAlgorithm->algorithm);
		}else{
			log_debug(logger,"3. No se reconoce el algoritmo de planificación %s", schedulingAlgorithm->algorithm);
		}
	}else{
		exit(-3);
	}
	if (config_has_property(config, schedulingAlgorithm->quantumKey)){
		schedulingAlgorithm->quantum=config_get_int_value(config,schedulingAlgorithm->quantumKey);
		log_debug(logger,"3. Se leyó el quantum: %i",schedulingAlgorithm->quantum);
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

double readConfigAlphaValue(t_config *config){
	if(config_has_property(config,"ALPHA")){
		double alpha;
		alpha=config_get_double_value(config,"ALPHA");
		return alpha;
	}else{
		return 0;
	}
}

int readConfigInitialEstimatedValue(t_config* config){
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
	initTrainersPokemonLists();
	getTrainerAttr(trainersPosition,trainersPokemons,trainersObjetives,trainersCount);
	statesLists.newList.count=trainersCount;
	initBurstScheduledPokemon();
	initTrainerName();
	initCPUClocksCountForTrainers();
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
		initPreviousBurst(&(statesLists.newList.trainerList[trainer]));
		initScheduledPokemon(&(statesLists.newList.trainerList[trainer]));
	}
	log_debug(logger,"valor del count %u", statesLists.newList.count);
}

void initTrainerName(){
	for(int trainer = 0; trainer < statesLists.newList.count; trainer++){
		statesLists.newList.trainerList[trainer].id =  trainer;
	}
}

void initPreviousBurst(t_trainer* trainer){
	trainer->parameters.previousBurst=-1;

}

void initCPUClocksCountForTrainers(){
	for(int trainer = 0; trainer < statesLists.newList.count; trainer++){
		statesLists.newList.trainerList[trainer].parameters.cpuClocksCount=0;
	}
}

void initScheduledPokemon(t_trainer* trainer){
	trainer->parameters.scheduledPokemon.name="NULL";
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
		statesLists.newList.trainerList[actualTrainer].parameters.pokemonsCount =0;

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
			  statesLists.newList.trainerList[actualTrainer].parameters.pokemons[counter].name=malloc(strlen((char*)element)+1);
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
			  statesLists.newList.trainerList[actualTrainer].parameters.objetives[counter].name=malloc(strlen((char*)element)+1);
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

void addToPokemonList(t_trainer* trainer){
	if((*trainer).parameters.pokemonsCount==0){
		((*trainer).parameters.pokemons)=(t_pokemon*)malloc(sizeof(t_pokemon));
	}else{
		void* temp = realloc((*trainer).parameters.pokemons,sizeof(t_pokemon)*(((*trainer).parameters.pokemonsCount)+1));
		if (!temp){
			log_debug(logger,"error en realloc");
					exit(9);
				}
		((*trainer).parameters.pokemons)=temp;
		}
	(*trainer).parameters.pokemons[(*trainer).parameters.pokemonsCount]=(*trainer).parameters.scheduledPokemon;
	(*trainer).parameters.pokemonsCount++;
}

void addToReady(t_trainer* trainer){
	void* temp = realloc(statesLists.readyList.trainerList,sizeof(t_trainer)*((statesLists.readyList.count)+1));
	if (!temp){
		log_debug(logger,"error en realloc");
				exit(9);
			}
	(statesLists.readyList.trainerList)=temp;
	sem_wait(&countReady_semaphore);
	statesLists.readyList.trainerList[statesLists.readyList.count]=(*trainer);
	(statesLists.readyList.count)++;
	sem_post(&countReady_semaphore);
	sem_post(&readyTrainer_sem);
}

void removeFromReady(int trainerPositionInList){

	for(int i=trainerPositionInList;i<(statesLists.readyList.count)-1; i++){
		statesLists.readyList.trainerList[i] = statesLists.readyList.trainerList[i+1];

	}
	sem_wait(&countReady_semaphore);
	(statesLists.readyList.count)--;
	sem_post(&countReady_semaphore);
	if(statesLists.readyList.count){
		void* temp = realloc(statesLists.readyList.trainerList,sizeof(t_trainer)*((statesLists.readyList.count)));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(9);
		}
		(statesLists.readyList.trainerList)=temp;
	}
}

void removeFromNew(int trainerPositionInList){
	log_debug(logger,"Se removera el entrenador %i de New. Total: %i",statesLists.newList.trainerList[trainerPositionInList].id,statesLists.newList.count);
	for(int i=trainerPositionInList;i<(statesLists.newList.count)-1; i++){
		statesLists.newList.trainerList[i] = statesLists.newList.trainerList[i+1];

	}
	(statesLists.newList.count)--;
	if(statesLists.newList.count){
	void* temp = realloc(statesLists.newList.trainerList,sizeof(t_trainer)*((statesLists.newList.count)));
	if (!temp){
		log_debug(logger,"error en realloc");
		exit(9);
	}
	(statesLists.newList.trainerList)=temp;
	}
	log_debug(logger,"Se removio el entrenador de new. Total: %i",statesLists.newList.count);
}

void removeFromMissingPkms(t_pokemon pkm){
	for(int i=0;i<missingPokemonsCount;i++){
		if(0==strcmp(missingPkms[i].pokemon.name,pkm.name)){
			if(missingPkms[i].count==1){
				for(int j=i;j<(missingPokemonsCount)-1; j++){
					missingPkms[j] = missingPkms[j+1] ;
					}
				missingPokemonsCount--;
			}else{
				missingPkms[i].count--;
			}
		}
	}
	if(missingPokemonsCount>0){
			void* temp = realloc(missingPkms,sizeof(t_objetive)*missingPokemonsCount);
			if (!temp){
				log_debug(logger,"error en realloc");
				exit(9);
			}
		(missingPkms)=temp;
		}
}

void removeFromAvailable(int pkmPosition){
	log_debug(logger,"Before removeMessageAppeared: availablePokemonCount: %i",availablePokemons.count);
	for(int i=pkmPosition;i<(availablePokemons.count)-1; i++){
		availablePokemons.pokemons[i] = availablePokemons.pokemons[i+1] ;

	}

	(availablePokemons.count)--;

	if(availablePokemons.count){
		void* temp = realloc(availablePokemons.pokemons,sizeof(t_trainer)*((availablePokemons.count)));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(9);
		}
	(availablePokemons.pokemons)=temp;
	}
	log_debug(logger,"After removeMessageAppeared: availablePokemonCount: %i",availablePokemons.count);
}


void removeFromBlocked(int trainerPositionInList){

	for(int i=trainerPositionInList;i<(statesLists.blockedList.count)-1; i++){
		statesLists.blockedList.trainerList[i] = statesLists.blockedList.trainerList[i+1];

	}
	sem_wait(&countBlocked_semaphore);
	(statesLists.blockedList.count)--;
	sem_post(&countBlocked_semaphore);
	if(statesLists.blockedList.count){
		void* temp = realloc(statesLists.blockedList.trainerList,sizeof(t_trainer)*((statesLists.blockedList.count)));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(9);
		}

		(statesLists.blockedList.trainerList)=temp;
	}
}

void addToBlocked(t_trainer trainer){
	void* temp = realloc(statesLists.blockedList.trainerList,sizeof(t_trainer)*((statesLists.blockedList.count)+1));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(10);
		}
		(statesLists.blockedList.trainerList)=temp;
		sem_wait(&countBlocked_semaphore);
		statesLists.blockedList.trainerList[statesLists.blockedList.count]=trainer;
		(statesLists.blockedList.count)++;
		sem_post(&countBlocked_semaphore);
}


void addToExec(t_trainer trainer){
	statesLists.execTrainer.trainer=trainer;
	statesLists.execTrainer.boolean = 1;
}

void removeFromExec(){
	statesLists.execTrainer.boolean = 0;
	sem_post(&execTrainer_sem);
}

void addToExit(t_trainer trainer){
	void* temp = realloc(statesLists.exitList.trainerList,sizeof(t_trainer)*((statesLists.exitList.count)+1));
		if (!temp){
			log_debug(logger,"error en realloc");
			exit(10);
		}
		(statesLists.exitList.trainerList)=temp;
		sem_wait(&countExit_semaphore);
		statesLists.exitList.trainerList[statesLists.exitList.count]=trainer;
		(statesLists.exitList.count)++;
		sem_post(&countExit_semaphore);
		sem_post(&exitCount_sem);
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

void getClosestTrainer(t_pokemon* pkmAvailable, int pkmAvailablePos){
	log_debug(logger,"obteniendo el entrenador mas cercano");
	int closestNew;
	int closestBlocked;
	log_debug(logger,"%i",getCountBlockedAvailable());
	if(getCountBlockedAvailable()==0){
		log_debug(logger,"entre");
		closestNew = getClosestTrainerNew(pkmAvailable);
		statesLists.newList.trainerList[closestNew].parameters.scheduledPokemon=*pkmAvailable;
		removeFromAvailable(pkmAvailablePos);
		addToReady(&statesLists.newList.trainerList[closestNew]);
		removeFromNew(closestNew);
	}else if(statesLists.newList.count==0){
		closestBlocked = getClosestTrainerBlocked(pkmAvailable);
		log_debug(logger, "posición del closestBloqued %i", closestBlocked);
		statesLists.blockedList.trainerList[closestBlocked].parameters.scheduledPokemon=*pkmAvailable;
		removeFromAvailable(pkmAvailablePos);
		addToReady(&statesLists.blockedList.trainerList[closestBlocked]);
		removeFromBlocked(closestBlocked);
	}else{

		closestNew = getClosestTrainerNew(pkmAvailable);
		closestBlocked = getClosestTrainerBlocked(pkmAvailable);

		int distanceNew = getDistanceToPokemonTarget(statesLists.newList.trainerList[closestNew],*pkmAvailable);
		int distanceBlocked = getDistanceToPokemonTarget(statesLists.blockedList.trainerList[closestBlocked],*pkmAvailable);
			if(distanceNew<=distanceBlocked ){
				statesLists.newList.trainerList[closestNew].parameters.scheduledPokemon=*pkmAvailable;
				removeFromAvailable(pkmAvailablePos);
				addToReady(&statesLists.newList.trainerList[closestNew]);
				removeFromNew(closestNew);
			}else{
				statesLists.blockedList.trainerList[closestBlocked].parameters.scheduledPokemon=*pkmAvailable;
				removeFromAvailable(pkmAvailablePos);
				addToReady(&statesLists.blockedList.trainerList[closestBlocked]);
				removeFromBlocked(closestBlocked);
			}
	}
}

int getClosestTrainerNew(t_pokemon* pkmAvailable){
	int clockTimeToPokemon;
	int pos=-1;
	int clockTimeToPokemonAux;
	if(statesLists.newList.count>0){
		clockTimeToPokemon = getDistanceToPokemonTarget(statesLists.newList.trainerList[0],*pkmAvailable);
		pos = 0;
		for(int i=1;i<statesLists.newList.count;i++){
			if(statesLists.newList.trainerList[i].parameters.objetivesCount>statesLists.newList.trainerList[i].parameters.pokemonsCount){
				clockTimeToPokemonAux = getDistanceToPokemonTarget(statesLists.newList.trainerList[i],*pkmAvailable);
				if(clockTimeToPokemon > clockTimeToPokemonAux){
					clockTimeToPokemon = clockTimeToPokemonAux;
					pos=i;
				}
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
		log_debug(logger, "entrenador seleccionado de la lista bloque %u", statesLists.blockedList.trainerList[i].id);
		clockTimeToPokemon = getDistanceToPokemonTarget(statesLists.blockedList.trainerList[i],*pkmAvailable);
		i++;
		int clockTimeToPokemonAux;
		for(;i<statesLists.blockedList.count;i++){
			if(statesLists.blockedList.trainerList[i].blockState==AVAILABLE){
				clockTimeToPokemonAux = getDistanceToPokemonTarget(statesLists.blockedList.trainerList[i],*pkmAvailable);
				if(clockTimeToPokemon > clockTimeToPokemonAux){
					clockTimeToPokemon = clockTimeToPokemonAux;
					pos=i;
				}
			}
		}
	}
	return pos;
}

void scheduleByDistance(){
	if(getCountBlockedAvailable()+statesLists.newList.count){
		int i;
		int selectedMissingPokemonCount;
		for(int pkm=0;pkm<availablePokemons.count;pkm++){
			//log_debug(logger,"1. Available Itera en %i, max: %i",pkm,availablePokemons.count);
			for(i=0; (i<missingPokemonsCount); i++){
				//log_debug(logger,"2. missingPokemons Itera en %i,max: %i",i,missingPokemonsCount);
				selectedMissingPokemonCount = 0;
				log_debug(logger,"a. Comparo: %s %s",availablePokemons.pokemons[pkm].name,missingPkms[i].pokemon.name);
				if(0==strcmp(availablePokemons.pokemons[pkm].name,missingPkms[i].pokemon.name)){
					//log_debug(logger,"entré");
					selectedMissingPokemonCount = missingPkms[i].count;
					log_debug(logger,"selectedMissingPokemonCount: %i", selectedMissingPokemonCount);
					//log_debug(logger,"b. valido: readyList %i + blockedwaiting %i + exectrainer %i < missingPokemons %i",statesLists.readyList.count,getCountBlockedWaiting(),statesLists.execTrainer.boolean,missingPokemonsCount);
					if(statesLists.readyList.count+getCountBlockedWaiting()+statesLists.execTrainer.boolean < missingPokemonsCount){
						//log_debug(logger,"entré");
						if(statesLists.execTrainer.boolean!=0){
							log_debug(logger,"c. Comparo: %s %s",statesLists.execTrainer.trainer.parameters.scheduledPokemon.name,availablePokemons.pokemons[pkm].name);
							if(0==strcmp(statesLists.execTrainer.trainer.parameters.scheduledPokemon.name,availablePokemons.pokemons[pkm].name)){
								log_debug(logger,"entré 1");
								selectedMissingPokemonCount--;
							}
						}
						for(int j=0;j<statesLists.readyList.count;j++){
							//log_debug(logger,"3. readyListCount Itera en %i, max: %i",j,statesLists.readyList.count);
							//log_debug(logger,"d. Comparo: %s %s",statesLists.readyList.trainerList[j].parameters.scheduledPokemon.name,availablePokemons.pokemons[pkm].name);
							if(0==strcmp(statesLists.readyList.trainerList[j].parameters.scheduledPokemon.name,availablePokemons.pokemons[pkm].name)){
								log_debug(logger,"entré 2 con entrenado %u",statesLists.readyList.trainerList[j].id);
								selectedMissingPokemonCount--;
							}
						}
						for(int j=0;j<statesLists.blockedList.count;j++){
							//log_debug(logger,"4. readyListCount Itera en %i, max: %i",j,statesLists.readyList.count);
							//log_debug(logger,"e. Comparo: %s %s",statesLists.blockedList.trainerList[j].blockState,WAITING);
							if(statesLists.blockedList.trainerList[j].blockState==WAITING){
								//log_debug(logger,"entré");
								//log_debug(logger,"e. Comparo: %s %s",statesLists.blockedList.trainerList[j].parameters.scheduledPokemon.name,availablePokemons.pokemons[pkm].name);
								if(0==strcmp(statesLists.blockedList.trainerList[j].parameters.scheduledPokemon.name,availablePokemons.pokemons[pkm].name)){
									log_debug(logger,"entré 3");
									selectedMissingPokemonCount--;
								}
							}
						}

						if(selectedMissingPokemonCount>0){
							//log_debug(logger,"Se buscara al enrrenador mas cercano");
							getClosestTrainer(&(availablePokemons.pokemons[pkm]),pkm);
							return;
						}

					}

				}
			}
		}
	}
	sem_post(&(availableTrainersCount_sem));
}



void scheduleFifo(){
	if(statesLists.readyList.count  && statesLists.execTrainer.boolean==0){
		addToExec(statesLists.readyList.trainerList[0]);
		removeFromReady(0);
		int valueOfExecuteClock = 1;
		while(valueOfExecuteClock){
			valueOfExecuteClock = executeClock();
			if(valueOfExecuteClock==1){
				statesLists.execTrainer.trainer.parameters.cpuClocksCount++;
			}
		}
	}
}


void scheduleRR(){
	if(statesLists.readyList.count  && statesLists.execTrainer.boolean==0){
		int valueOfExecuteClock = 1;
		addToExec(statesLists.readyList.trainerList[0]);
		removeFromReady(0);
		for(int i=0;i<schedulingAlgorithm.quantum;i++){
			if(valueOfExecuteClock == 1){
				valueOfExecuteClock = executeClock();
				statesLists.execTrainer.trainer.parameters.cpuClocksCount += valueOfExecuteClock;
			}else{
				break;
			}
		}
		if(valueOfExecuteClock == 1){
			removeFromExec();
			addToReady(&statesLists.execTrainer.trainer);
		}/*else{
			removeFromExec();
			addToBlocked(statesLists.execTrainer.trainer);
		}*/
	}
}


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
					statesLists.execTrainer.trainer.parameters.cpuClocksCount++;
					statesLists.execTrainer.trainer.parameters.previousBurst++;
				}
			}
			if(cutWhile == 0){
				removeFromExec();
				addToBlocked(statesLists.execTrainer.trainer);
			}

		}
}


void scheduleSJFCD(){
	if(statesLists.readyList.count  && statesLists.execTrainer.boolean==0){
		int pos = getTrainerWithBestEstimatedBurst();
		log_debug(logger, "ráfaga ejecutada anteriormente %i ráfaga estimada anterior %f para entrenador %u ", statesLists.readyList.trainerList[pos].parameters.previousBurst,statesLists.readyList.trainerList[pos].parameters.previousEstimate,statesLists.readyList.trainerList[pos].id);
		float estimatedBurstTimeForCPU = estimatedTimeForNextBurstCalculation(pos);
		log_debug(logger,"estimatedBurstTimeForCPU %f para entrenador %u ", estimatedBurstTimeForCPU,statesLists.readyList.trainerList[pos].id);
		addToExec(statesLists.readyList.trainerList[pos]);
		removeFromReady(pos);
		int cutWhile = 1;
		statesLists.execTrainer.trainer.parameters.previousBurst = 0;
		statesLists.execTrainer.trainer.parameters.previousEstimate = estimatedBurstTimeForCPU;
		while(cutWhile && (differenceBetweenEstimatedBurtsAndExecutedClocks(estimatedBurstTimeForCPU, statesLists.execTrainer.trainer.parameters.previousBurst)) <= estimatedTimeForNextBurstCalculation(getTrainerWithBestEstimatedBurst())){
			estimatedBurstTimeForCPU--;
			cutWhile = executeClock();
			if(cutWhile==1){
				statesLists.execTrainer.trainer.parameters.cpuClocksCount++;
				statesLists.execTrainer.trainer.parameters.previousBurst++;
			}
		}
		if(cutWhile == 0){
			log_debug(logger,"ráfaga ejecutada %i para entrenador %u ", statesLists.execTrainer.trainer.parameters.previousBurst, statesLists.execTrainer.trainer.id);
			log_debug(logger,"rafaga estimada %f para entrenador %u ", statesLists.execTrainer.trainer.parameters.previousEstimate, statesLists.execTrainer.trainer.id);
			removeFromExec();
			addToBlocked(statesLists.execTrainer.trainer);
		}else{
			addToReady(&statesLists.execTrainer.trainer);
			removeFromExec();
		}

	}
}

void deadlockForTrainer(){
	int flag;
	for(int i = 0; i<statesLists.blockedList.count; i++){
		if(statesLists.blockedList.trainerList[i].parameters.objetivesCount == statesLists.blockedList.trainerList[i].parameters.pokemonsCount){
			for(int j = 0; j<statesLists.blockedList.trainerList[i].parameters.objetivesCount;j++){
				for(int k = 0; k<statesLists.blockedList.trainerList[i].parameters.pokemonsCount;k++){
					flag = strcmp(statesLists.blockedList.trainerList[i].parameters.pokemons[j].name,statesLists.blockedList.trainerList[i].parameters.objetives[k].name);

				}
			}
		}
	}
}


int evaluateDeadlockCondition(){
	int deadlock=0;
	int blockedDeadlocks = blockedInDeadlock();
	if((statesLists.readyList.count == 0) && (statesLists.newList.count == 0) && blockedDeadlocks && (statesLists.execTrainer.boolean == 0)){
		deadlock = 1;
	}
	return deadlock;
}

int blockedInDeadlock(){
	int blockedTrainers = statesLists.blockedList.count;
	int flagBlockedInDeadlock = 1;
	for(int i=0; i<blockedTrainers; i++){
		if(statesLists.blockedList.trainerList[i].blockState == WAITING || statesLists.blockedList.trainerList[i].blockState == AVAILABLE){
			flagBlockedInDeadlock = 0;
			break;
		}
	}
	return flagBlockedInDeadlock;
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

	if(statesLists.execTrainer.trainer.blockState!=DEADLOCK){
	if(getDistanceToPokemonTarget(statesLists.execTrainer.trainer,statesLists.execTrainer.trainer.parameters.scheduledPokemon)!=0){
		moveTrainerToObjective(&(statesLists.execTrainer.trainer));
		return 1;
	}else if(getDistanceToPokemonTarget(statesLists.execTrainer.trainer,statesLists.execTrainer.trainer.parameters.scheduledPokemon)==0){
		catchPokemon();
		return 0;
	}
		}else{
			if(getDistanceToTrainerToExchange(statesLists.execTrainer.trainer)!=0){
				moveTrainerToObjectiveDeadlock(&statesLists.execTrainer.trainer);
				return 1;
			}else if(getDistanceToTrainerToExchange(statesLists.execTrainer.trainer)==0){
				exchangePokemon(&statesLists.execTrainer.trainer);
				return 0;
			}
	}
	return -1;
}

void catchPokemon(){
	if(connectedToBroker){
		log_debug(logger,"Se enviará el catch");
		catch_pokemon catch;
		catch.pokemonName = statesLists.execTrainer.trainer.parameters.scheduledPokemon.name;
		catch.horizontalCoordinate = statesLists.execTrainer.trainer.parameters.scheduledPokemon.position.x;
		catch.verticalCoordinate = statesLists.execTrainer.trainer.parameters.scheduledPokemon.position.y;
		int socketCatch = connectBroker(broker.ip, broker.port);
		Send_CATCH(catch, socketCatch);
		op_code type;
		void* content = malloc(sizeof(void*));
		int result;
		int cut = 0;
		while(cut != 1){
			result = RecievePackage(socketCatch,&type,&content);
			if(type == ACKNOWLEDGE){
				cut=1;
			}
		}
		uint32_t originMessageType = CATCH_POKEMON;
		if(!result){
				processAcknowledge(content,originMessageType,statesLists.execTrainer.trainer.id);
		}
		sleep(clockSimulationTime);
		log_debug(logger,"Se envió el catchdel pokemon %s por elentrenador %i",catch.pokemonName,statesLists.execTrainer.trainer.id);
		statesLists.execTrainer.trainer.blockState = WAITING;
		addToBlocked(statesLists.execTrainer.trainer);
		removeFromExec();
	}else{
		removeFromMissingPkms(statesLists.execTrainer.trainer.parameters.scheduledPokemon);
		addToPokemonList(&(statesLists.execTrainer.trainer));
		log_debug(logger,"Se captura el pokemon por falta de conexión con broker");

		if(statesLists.execTrainer.trainer.parameters.objetivesCount==statesLists.execTrainer.trainer.parameters.pokemonsCount){
			if(checkTrainerState(statesLists.execTrainer.trainer)==1){
				statesLists.execTrainer.trainer.blockState = DEADLOCK;
				addToBlocked(statesLists.execTrainer.trainer);
				removeFromExec();
				sem_post(&(deadlockCount_sem));
			}else{
				addToExit(statesLists.execTrainer.trainer);
				removeFromExec();
				sem_post(&(deadlockCount_sem));
			}
		}else{
			statesLists.execTrainer.trainer.blockState = AVAILABLE;
			addToBlocked(statesLists.execTrainer.trainer);
			removeFromExec();
			sem_post(&(availableTrainersCount_sem));
		}

	}

}

void processAcknowledge(void*buffer,uint32_t type ,uint32_t trainerId){
	switch(type){
		case CATCH_POKEMON:
		{
			uint32_t* id = (uint32_t*)buffer;
			if(catchList.count==0){
				catchList.catchMessage[catchList.count].catchId = (*id);
				catchList.catchMessage[catchList.count].trainerId = trainerId;
				catchList.count++;
			}else{
				void* temp = realloc(catchList.catchMessage,sizeof(t_catchMessage)*((catchList.count)+1));
				if (!temp){
					log_debug(logger,"error en realloc");
					exit(9);
				}
				(catchList.catchMessage)=temp;
				catchList.catchMessage[catchList.count].catchId = (*id);
				catchList.catchMessage[catchList.count].trainerId = trainerId;
				catchList.count++;

			}
			break;
		}
		case GET_POKEMON:
		{
			uint32_t* id = (uint32_t*)buffer;
			if(getList.count==0){
						getList.id[getList.count] = (*id);
						getList.count++;
			}else{
				void* temp = realloc(getList.id,sizeof(uint32_t)*((getList.count)+1));
				if (!temp){
					log_debug(logger,"error en realloc");
					exit(9);
				}
				(getList.id)=temp;
				getList.id[getList.count]=(*id);
				getList.count++;
			}
		}
	}
}

int checkTrainerState(t_trainer trainer){
	int distinctPkmCount;
	for(int i=0;i<trainer.parameters.objetivesCount;i++){
		log_debug(logger,"se comparán %i objetivos y %i pokemons atrapados ",trainer.parameters.objetivesCount,trainer.parameters.pokemonsCount);
		distinctPkmCount=0;
		for(int j=0;j<trainer.parameters.objetivesCount;j++){
			log_debug(logger,"1 se compará %s con %s ",trainer.parameters.objetives[i].name,trainer.parameters.objetives[j].name);
			if(0==strcmp(trainer.parameters.objetives[i].name,trainer.parameters.objetives[j].name)){
				distinctPkmCount++;
			}
		}
		for(int k=0;k<trainer.parameters.pokemonsCount;k++){
			log_debug(logger,"2 se compará %s con %s ",trainer.parameters.objetives[i].name,trainer.parameters.pokemons[k].name);
			if(0==strcmp(trainer.parameters.objetives[i].name,trainer.parameters.pokemons[k].name)){
				distinctPkmCount--;
			}
		}
		if(distinctPkmCount!=0){
			log_debug(logger,"Entrenador en deadlock");
			deadlockCount++;
			return 1;//deadlock
		}
	}
	log_debug(logger,"Entrenador finalizado");
	return 0;//exit
}

//TODO - Función que mueve al entrenador - Falta ver como implementaremos los semáforos
void moveTrainerToObjective(t_trainer* trainer){

	int difference_x = calculateDifference(trainer->parameters.position.x, trainer->parameters.scheduledPokemon.position.x);
	int difference_y = calculateDifference(trainer->parameters.position.y, trainer->parameters.scheduledPokemon.position.y);
	moveTrainerToTarget(trainer, difference_x, difference_y);

}

//TODO - funcion que mueve una posición al entrenador - Falta Definir como haremos el CATCH
void moveTrainerToTarget(t_trainer* trainer, int distanceToMoveInX, int distanceToMoveInY){
	if(distanceToMoveInX > 0){
		trainer->parameters.position.x--;
		sleep(clockSimulationTime);
		log_debug(logger,"Se movió al entrenador a la posición %d %d", trainer->parameters.position.x, trainer->parameters.position.y);
	}
	else if(distanceToMoveInX < 0){
		trainer->parameters.position.x++;
		sleep(clockSimulationTime);
		log_debug(logger,"Se movió al entrenador a la posición %d %d", trainer->parameters.position.x, trainer->parameters.position.y);
	}
	else if(distanceToMoveInY > 0){
		trainer->parameters.position.y--;
		sleep(clockSimulationTime);
		log_debug(logger,"Se movió al entrenador a la posición %d %d", trainer->parameters.position.x, trainer->parameters.position.y);
	}
	else if(distanceToMoveInY < 0){
		trainer->parameters.position.y++;
		sleep(clockSimulationTime);
		log_debug(logger,"Se movió al entrenador a la posición %d %d", trainer->parameters.position.x, trainer->parameters.position.y);
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
	clockTime += (difference_x >= 0) ? difference_x : (difference_x*(-1));
	clockTime += (difference_y >= 0) ? difference_y : (difference_y*(-1));
	return clockTime;
}


int getDistanceToPokemonTarget(t_trainer trainer,  t_pokemon targetPokemon){
	int distanceInX = calculateDifference(trainer.parameters.position.x, targetPokemon.position.x);
	int distanceInY = calculateDifference(trainer.parameters.position.y, targetPokemon.position.y);
	int distance = getClockTimeToNewPosition(distanceInX, distanceInY);
	log_debug(logger,"Entrenador: %i en X=%i Y=%i, Pokemon %s en X=%i Y=%i distancia %i",trainer.id,trainer.parameters.position.x, trainer.parameters.position.y,targetPokemon.name,targetPokemon.position.x,targetPokemon.position.y,distance);
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
