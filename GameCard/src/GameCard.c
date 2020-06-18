/*
 ============================================================================
 Name        : GamerCard.c
 Author      : Mauro
 Version     :
 Copyright   : Your copyright notice
 Description : GameCard Process
 ============================================================================
 */

#include "GameCard.h"

t_GameCard* GameCard;

int main(void) {

	t_log* logger;
	t_config* config;
	pthread_t* subs;

	char* ip,puerto;
	char* ptoMnt = (char*)malloc(sizeof(char)*255);
	//	char* ptoMnt, retryConnection, retryOperation, delayTime;

	int server,client;

	logger = iniciar_logger();
	log_info(logger,"PROCESO GAMECARD ONLINE");


	config = leer_config();

	ip= obtener_valor_config(config,logger,IP_BROKER);
	puerto = obtener_valor_config(config,logger,PUERTO_BROKER);
	ptoMnt = obtener_valor_config(config,logger,PUNTO_MONTAJE_TALLGRASS);
	// retryConnection = obtener_valor_config(config.logger,TIEMPO_DE_REINTENTO_CONEXION);
	// retryOperation = obtener_valor_config(config.logger,TIEMPO_DE_REINTENTO_OPERACION);
	// delayTime = obtener_valor_config(config.logger,TIEMPO_RETARDO_OPERACION);

	GameCard=GameCard_initialize(logger,ptoMnt);
	GameCard_mountFS(config);

// ESTO YA FUNCIONA PERO NO LO VOY A PROBAR POR AHORA
//	initBroker(&broker);
//	readConfigBrokerValues(config,logger,&broker);
//
//	subs=(pthread_t*)malloc(sizeof(pthread_t)*3);
//
//	subscribeToBroker(broker,subs);

	free(ptoMnt);

	return EXIT_SUCCESS;
}

t_GameCard* GameCard_initialize(t_log* logger, char* ptoMnt){
	log_debug(logger,"Inicializa la GameCard");
	t_GameCard* aux = (t_GameCard*)malloc(sizeof(t_GameCard));
	aux->ptoMnt = (char*)malloc(strlen(ptoMnt));
	strcpy(aux->ptoMnt,ptoMnt);
	aux->logger = logger;

	char* files = (char*)malloc(strlen(ptoMnt)+strlen("/files/") + 1);
	strcpy(files,aux->ptoMnt);
	strcat(files,"/Files/");
	aux->filePath= files;

	char* metadata = (char*)malloc(strlen(ptoMnt)+strlen("/metadata/") + 1);
	strcpy(metadata,aux->ptoMnt);
	strcat(metadata,"/Metadata/");
	aux->metadataPath = metadata;

	char* blocks = (char*)malloc(strlen(ptoMnt)+strlen("/blocks/") + 1);
	strcpy(blocks,aux->ptoMnt);
	strcat(blocks,"/Blocks/");
	aux->blocksPath = blocks;

	return aux;
}

int GameCard_mountFS(t_config* config){

	// CREATE FIRST DIRECTORY
	int result = create_directory("/home/utnso/desktop/");

		result = create_directory(GameCard->ptoMnt);

		log_debug(GameCard->logger, "1.1 Mount FS - Resultado de la creacion del directorio: %s - %d",GameCard->ptoMnt, result);

	// CREATE METADA FOR FILES & FILES DIRECTORY
		result = create_directory(GameCard->filePath);

		log_debug(GameCard->logger, "1.2 Mount FS - Resultado de la creacion del directorio: %s - %d", GameCard->filePath, result);

		char* metadataForFiles = (char*) malloc(strlen(GameCard->filePath) + strlen("metadata.bin"));
		strcpy(metadataForFiles,GameCard->filePath);
		strcat(metadataForFiles,"metadata.bin");

		t_values* values = (t_values*)malloc(sizeof(t_values));

		values->values= list_create();
		char* value1 = "Y";
		list_add(values->values,value1);

		result = create_file(metadataForFiles, METADATA_DIRECTORY,values);

		list_destroy(values->values);
		free(metadataForFiles);

		// CREATE BLOCKS DIRECTORY
		result = create_directory(GameCard->blocksPath);

		log_debug(GameCard->logger, "1.3 Mount FS - Resultado de la creacion del directorio: %s - %d", GameCard->blocksPath, result);

		// CREATE METADATA FOR METADA DIRECTORY

		result = create_directory(GameCard->metadataPath);

		log_debug(GameCard->logger, "1.4 Mount FS - Resultado de la creacion del directorio: %s - %d", GameCard->metadataPath, result);


		char* metadataForMetadata = (char*) malloc(strlen(GameCard->metadataPath) + strlen("metadata.bin") +1);
		strcpy(metadataForMetadata,GameCard->metadataPath);
		strcat(metadataForMetadata,"metadata.bin");

		values->values= list_create();
		char* blocksize = (char*)malloc(strlen(obtener_valor_config(config,GameCard->logger,BLOCK_SIZE)));
		blocksize= obtener_valor_config(config,GameCard->logger,BLOCK_SIZE);

		char* blocks=(char*)malloc(strlen(obtener_valor_config(config,GameCard->logger,BLOCKS)));
		blocks=obtener_valor_config(config,GameCard->logger,BLOCKS);

		char* magicNumber = (char*)malloc(strlen(obtener_valor_config(config,GameCard->logger,MAGIC_NUMBER)));
		magicNumber = obtener_valor_config(config,GameCard->logger,MAGIC_NUMBER);

		list_add(values->values,blocksize);
		list_add(values->values,blocks);
		list_add(values->values,magicNumber);

		result = create_file(metadataForMetadata, METADATA,values);
		list_destroy(values->values);
		free(metadataForMetadata);
		free(values);

		GameCard->block_size = atoi(blocksize);
		GameCard->blocks = atoi(blocks);

		GameCard_Initialize_bitarray();
		char* bitmap = (char*) malloc(strlen(GameCard->metadataPath) + strlen("bitmap.bin") +1);
		strcpy(bitmap,GameCard->metadataPath);
		strcat(bitmap,"bitmap.bin");
		result = create_file(bitmap, BITMAP,values);
		free(bitmap);


		return result;
}

void GameCard_Process_Message(deli_message* message){

	switch (message->messageType){
		case NEW_POKEMON: {
			break;
		}

		case GET_POKEMON:{
			break;
		}

		case CATCH_POKEMON:{
			break;
		}
	}
}

void GameCard_Process_New(deli_message* message){
	new_pokemon* newPokemon = (new_pokemon*)message->messageContent;

	char * directory = (char*)malloc(strlen(GameCard->ptoMnt) + ("/files/") + strlen(newPokemon->pokemonName) );
	strcat(directory,"/files/");
	strcat(directory,newPokemon->pokemonName);
	int result = create_directory(directory);
}

void GameCard_Initialize_bitarray(){
	t_bitarray* aux;
	int size = (GameCard->blocks*GameCard->block_size);
	log_debug(GameCard->logger,"Se crea el bit array con una memoria de %d", size);
	char* bytes = (char*)malloc(sizeof(char)*size);
	aux = bitarray_create_with_mode(bytes, size, LSB_FIRST);
	log_debug(GameCard->logger,"Bitarray Creado");
	GameCard->bitArray=aux;
}

int create_directory(char* directory){
	struct stat st = {0};
	int result;

	if (stat(directory, &st) == -1) {
		result=mkdir(directory, 0700);
	    if (result!=0){
			log_debug(GameCard->logger,"Error en la creacion del directorio - ErrorCode: %d Description: %s",  errno, strerror(errno));
	    	return errno;
	    }
	    else{
		    log_debug(GameCard->logger,"Directorio %s -- Creado", directory);
		    return 0;
	    }
	}
	else
	{
		log_debug(GameCard->logger,"El directorio ya existe");
		return 2;
	}
}

int create_file(char* filename, fileType fileType, t_values* values){
	FILE * f;
	int result;
	f= fopen(filename,"wb");
	if (f==NULL){
		log_debug(GameCard->logger,"Error en la creacion del archivo %d");
		return -1;
	}

	switch(fileType){
		case METADATA:
		{
			result = create_file_metadata(f, values);
			break;
		}

		case POKE_METADATA:
			break;

		case POKE_FILE:
			break;

		case BITMAP:{
			result = create_file_bitmap(f);
			break;
		}

		case METADATA_DIRECTORY:{
			result = create_file_metadata_directory(f,values);
			break;
		}
	}

	log_debug(GameCard->logger,"Archivo: %s", filename);

	return result;
}

int create_file_bitmap(FILE* f){
	int max = bitarray_get_max_bit(GameCard->bitArray);
	log_debug(GameCard->logger,"El último bit es %d", max);

	for (int i = 0; i <= max; i= i + 1*8*GameCard->block_size){
		log_debug(GameCard->logger, "Bloque %d: ", (i/(8*GameCard->block_size)));
		if(bitarray_test_bit(GameCard->bitArray,i)){
			fprintf(f,"%c",'1');
		}else{
			fprintf(f,"%c",'0');
		}
	}
	fprintf(f,"%c",'\n');;
	return 0;
}

int create_file_metadata(FILE* f, t_values* values){
	char* line=(char*)malloc(strlen(BLOCK_SIZE) + 5);
	strcpy(line,BLOCK_SIZE);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,0));
	fprintf(f,"%s",line);
	fprintf(f,"%c",'\n');
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);

	line=(char*)malloc(strlen(BLOCKS) + 7);
	strcpy(line,BLOCKS);
	strcat(line,"=");
	strcat(line, (char*)list_get(values->values,1));
	fprintf(f,"%s",line);
	fprintf(f,"%c",'\n');
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);

	line =(char*)malloc(strlen(MAGIC_NUMBER) + 13 );
	strcpy(line,MAGIC_NUMBER);
	strcat(line,"=");
	strcat(line, (char*)list_get(values->values,2));
	fprintf(f,"%s",line);
	fprintf(f,"%c",'\n');
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);

	return 0;

}


int create_file_metadata_directory(FILE* f, t_values* values){
	char* line=(char*)malloc(strlen(DIRECTORY) + 4);
	strcpy(line,DIRECTORY);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,0));
	fprintf(f,"%s",line);
	fprintf(f,"%c",'\n');
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);
	return 0;
}


//BROKER LOGIC
void readConfigBrokerValues(t_config *config,t_log *logger,struct Broker *broker){
	log_debug(logger,"2.Set up Broker - Comienza lectura de config de broker");
	if (config_has_property(config,broker->ipKey)){
		broker->ip=config_get_string_value(config,broker->ipKey);
		log_debug(logger,"2.1 Set up Broker - Se leyó la IP: %s",broker->ip);
	}else{
		exit(-3);
	}

	if (config_has_property(config,broker->portKey)){
		broker->port=config_get_string_value(config,broker->portKey);
		log_debug(logger,"2.2 Set up Broker - Se leyó el puerto: %s",broker->port);
	}else{
		exit(-3);
	}
	log_debug(logger,"2.3 Set up Broker - Finaliza lectura de config de broker");
}

void subscribeToBroker(struct Broker broker,pthread_t* subs){

	pthread_create(&(subs[0]),NULL,subscribeToBrokerNew,(void*)&broker);
	sleep(2);
	pthread_create(&(subs[1]),NULL,subscribeToBrokerCatch,(void*)&broker);
	sleep(2);
	pthread_create(&(subs[2]),NULL,subscribeToBrokerGet,(void*)&broker);
	sleep(10);
}

void* subscribeToBrokerNew(void *brokerAdress){
	log_debug(GameCard->logger,"3.1 Suscribe Queue - Creando thread New Subscriptions Handler");
	struct Broker broker = *((struct Broker*) brokerAdress);
	int socketLocalized = connectBroker(broker.ip,broker.port,GameCard->logger);
	if (-1==SendSubscriptionRequest(NEW_POKEMON,socketLocalized)){
		log_debug(GameCard->logger,"3.1.1 Suscribe Queue - Error en subscripcion de New");
	}else{
		log_debug(GameCard->logger,"3.1.2 Suscribe Queue - Se subscribió a New");
	}
	pthread_exit(NULL);
}

void* subscribeToBrokerCatch(void *brokerAdress){
	log_debug(GameCard->logger,"3.2 Suscribe Queue - Creando thread Catch Subscriptions Handler");
	struct Broker broker = *((struct Broker*) brokerAdress);
	int socketAppeared = connectBroker(broker.ip,broker.port,GameCard->logger);
	if(-1==SendSubscriptionRequest(CATCH_POKEMON,socketAppeared)){
		log_debug(GameCard->logger,"3.2,1 Suscribe Queue - Error en subscripcion de Catch");
	}else{
		log_debug(GameCard->logger,"3.2.2 Suscribe Queue - Se subscribió a Catch");
	}
	pthread_exit(NULL);
}

void* subscribeToBrokerGet(void *brokerAdress){
	log_debug(GameCard->logger,"3.3 Suscribe Queue - Creando thread Get Subscriptions Handler");
	struct Broker broker = *((struct Broker*) brokerAdress);
	int socketCaught = connectBroker(broker.ip,broker.port,GameCard->logger);
	if(-1==SendSubscriptionRequest(GET_POKEMON,socketCaught)){
		log_debug(GameCard->logger,"3.3.1 Suscribe Queue - Error en subscripcion de Get");
	}else{
		log_debug(GameCard->logger,"3.3.2 Suscribe Queue - Se subscribió a Get");
	}
	pthread_exit(NULL);
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
    log_debug(logger,"3. Suscribe Queue - IP y puerto configurado");
    for (p = servinfo; p != NULL; p = p->ai_next) {
    	teamSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		log_debug(logger,"3. Suscribe Queue - Socket configurado");
		if (teamSocket == -1){
			log_debug(logger,"3. Suscribe Queue - El socket se configuró incorrectamente");
			return -2;
		}
		log_debug(logger,"3. Suscribe Queue - Se intentará conectar con Broker");
		if (connect(teamSocket, p->ai_addr, p->ai_addrlen)==0) {
			log_debug(logger,"3. Suscribe Queue - La conexión fue realizada");
			freeaddrinfo(servinfo);
			return teamSocket;
		}else{
			log_debug(logger,"3. Suscribe Queue - La conexión falló");
			close(teamSocket);
			return -1;
		}
	}
    return -1;
}

