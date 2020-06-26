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
//	pthread_t* subs;

//	char* ip,puerto;
	char* ptoMnt = (char*)malloc(sizeof(char)*255);
	//	char* ptoMnt, retryConnection, retryOperation, delayTime;

//	int server,client;

	logger = iniciar_logger();
	log_info(logger,"PROCESO GAMECARD ONLINE");


	config = read_config();

//	ip= obtener_valor_config(config,logger,IP_BROKER);
//	puerto = obtener_valor_config(config,logger,PUERTO_BROKER);
	ptoMnt = get_config_value(config,logger,PUNTO_MONTAJE_TALLGRASS);
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

	deli_message* message = (deli_message*)malloc(sizeof(deli_message));
	message->id = 88;
	message->correlationId=99;
	message->messageType=NEW_POKEMON;

	new_pokemon* newPokemon = (new_pokemon*)malloc(sizeof(new_pokemon));
	newPokemon->ammount=10;
	newPokemon->verticalCoordinate = 8;
	newPokemon->horizontalCoordinate = 4;
	newPokemon->pokemonName = "Pikachu";

	message->messageContent = (void*)newPokemon;

	GameCard_Process_Message(message);

//	munmap(GameCard->fileMapped, (GameCard->block_size*GameCard->blocks));

	bitarray_destroy(GameCard->bitArray);
//	free(GameCard->bitArray);
	free(ptoMnt);
	free(message);

	return EXIT_SUCCESS;
}

t_GameCard* GameCard_initialize(t_log* logger, char* ptoMnt){
	log_debug(logger,"Inicializa la GameCard");
	t_GameCard* aux = (t_GameCard*)malloc(sizeof(t_GameCard));

	aux->ptoMnt = (char*)malloc(strlen(ptoMnt)+1);
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

		t_values* values = (t_values*)malloc(sizeof(t_values));

		values->values= list_create();
		char* value1 = "Y";
		list_add(values->values,value1);

		result = create_file(METADATA_DIRECTORY,values);

		list_destroy(values->values);

		// CREATE BLOCKS DIRECTORY
		result = create_directory(GameCard->blocksPath);

		log_debug(GameCard->logger, "1.3 Mount FS - Resultado de la creacion del directorio: %s - %d", GameCard->blocksPath, result);

		// CREATE METADATA FOR METADA DIRECTORY

		result = create_directory(GameCard->metadataPath);

		log_debug(GameCard->logger, "1.4 Mount FS - Resultado de la creacion del directorio: %s - %d", GameCard->metadataPath, result);

		char* blocksize = (char*)malloc(strlen(get_config_value(config,GameCard->logger,BLOCK_SIZE))  +1 );
		blocksize= get_config_value(config,GameCard->logger,BLOCK_SIZE);

		char* blocks=(char*)malloc(strlen(get_config_value(config,GameCard->logger,BLOCKS)) + 1 );
		blocks=get_config_value(config,GameCard->logger,BLOCKS);

		char* magicNumber = (char*)malloc(strlen(get_config_value(config,GameCard->logger,MAGIC_NUMBER)) + 1);
		magicNumber = get_config_value(config,GameCard->logger,MAGIC_NUMBER);

		values->values= list_create();

		list_add(values->values,blocksize);
		list_add(values->values,blocks);
		list_add(values->values,magicNumber);

		result = create_file(METADATA,values);

		GameCard->block_size = atoi(blocksize);
		GameCard->blocks = atoi(blocks);

		list_destroy_and_destroy_elements(values->values,free);

		GameCard_Initialize_bitarray();

		result = create_file(BITMAP,values);

		free(values);

		return result;
}

void GameCard_Process_Message(deli_message* message){

	switch (message->messageType){
		case NEW_POKEMON: {
			GameCard_Process_Message_New(message);
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

void GameCard_Process_Message_New(deli_message* message){
	new_pokemon* newPokemon = (new_pokemon*)message->messageContent;

	char * directory = (char*)malloc(strlen(GameCard->filePath) + strlen(newPokemon->pokemonName) + 1 );
	strcpy(directory,GameCard->filePath);
	strcat(directory,newPokemon->pokemonName);

	log_debug(GameCard->logger,"Directorio de Pokemon a crear: %s", directory);
	int result = create_directory(directory);

	t_values* values= (t_values*)malloc(sizeof(t_values));
	values->values= list_create();

	list_add(values->values,newPokemon);

	int resultMessageProces;

	switch (result){
		case 2:
		{
			resultMessageProces= modify_poke_file(values, directory);
			break;
		}

		case 0:
		{
			resultMessageProces= create_poke_file(values);
			break;
		}

		default:
		{
			break;
		}
	}

	log_debug(GameCard->logger, "Resultado del procesamiento del mensaje: %d", resultMessageProces);
	free(directory);
}

void GameCard_Initialize_bitarray(){
	t_bitarray* aux = (t_bitarray*)malloc(sizeof(t_bitarray*));

	int size = (GameCard->blocks*GameCard->block_size);

	log_debug(GameCard->logger,"Se crea el bit array con una memoria de %d", size);

	//TODO: revisar el +1 lo puse por valgrin
	char* bytes = (char*)malloc(sizeof(char)*size+1);
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

int create_file(fileType fileType, t_values* values){
	int result;

	switch(fileType){

		case METADATA:
		{
			result = create_file_metadata(values);
			break;
		}

		case BIN_FILE:
		{
			result = create_file_bin(values);
			break;
		}

		case BITMAP:
		{
			result = create_file_bitmap();
			break;
		}

		case METADATA_DIRECTORY:
		{
			result = create_file_metadata_directory(values);
			break;
		}

		case POKE_METADATA:
		{
			result= create_file_metadata_poke(values);
			break;
		}
	}

	return result;
}

int modify_poke_file(t_values* values, char* directory){;

	char* file = (char*)malloc(strlen(directory) + strlen("/metadata.bin") + 1);

	strcpy(file,directory);
	strcat(file,"/metadata.bin");

	t_file_metadata* metadataFile = (t_file_metadata*)malloc(sizeof(t_file_metadata));

	t_config* metadataConfig;

	log_debug(GameCard->logger, "Crea el config para leer el archivo %s", file);

	metadataConfig = read_metadata(file);

	read_metadata_file(metadataFile, metadataConfig);

	// Levantar el archivo leyendo todos los bloques
	char* fileContent = get_file_content(metadataFile);

	log_debug(GameCard->logger,"Contenido del archivo %s", fileContent);

	return 0;
}

int create_poke_file(t_values* values){

	new_pokemon* newPokemon = (new_pokemon*)list_get(values->values,0);

	t_file_metadata* metadataFile = (t_file_metadata*)malloc(sizeof(t_file_metadata));

	metadataFile->block= list_create();

	int amountOfBlocks;

	//Sumo 2 por el \n
	int auxSize = (strlen(string_itoa(newPokemon->horizontalCoordinate))) + 1
			 + (strlen(string_itoa(newPokemon->verticalCoordinate))) + 1
			 + (strlen(string_itoa(newPokemon->ammount))) + 1;

	metadataFile->size =  string_itoa(auxSize);


	if (auxSize % GameCard->block_size == 0)
	    amountOfBlocks = auxSize/GameCard->block_size;
	else
	    amountOfBlocks = (auxSize/GameCard->block_size) + 1;


	log_debug(GameCard->logger, "Amout of blocks needed %d", amountOfBlocks);

	int result;

	write_blocks(metadataFile, amountOfBlocks, newPokemon);

	metadataFile->directory='N';
	metadataFile->open='N';

	list_add(values->values,metadataFile);

	result=create_file(POKE_METADATA,values);
	list_destroy(metadataFile->block);
	list_destroy_and_destroy_elements(values->values,free);
	free(values);

	return result;
}

int create_file_metadata_poke(t_values* values){

	new_pokemon* newPokemon = (new_pokemon*)list_get(values->values,0);

	char rc='\n';
	char sepRight = ']';
	char sepLeft = '[';
	char comma = ',';
	char equal ='=';
	char* filename = (char*)malloc(strlen(GameCard->filePath) + strlen(newPokemon->pokemonName) + strlen("/metadata.bin") + 1);
	strcpy(filename,GameCard->filePath);
	strcat(filename,newPokemon->pokemonName);
	strcat(filename,"/metadata.bin");

	FILE* f= fopen(filename,"wb");

	if (f==NULL){
		log_debug(GameCard->logger,"Error en la creacion del archivo %s", filename);
		return -1;
	}
	t_file_metadata* metadataFile = (t_file_metadata*)list_get(values->values,1);

	char* line = (char*)malloc(strlen(DIRECTORY) + 1 + 2 + 1);
	strcpy(line,DIRECTORY);
	strcat(line,"=");
	fwrite(line, strlen(line),1,f);
	fwrite(&(metadataFile->directory),sizeof(char),1,f);
	fwrite(&rc,sizeof(char),1,f);

	free(line);

	line=(char*)malloc(strlen(SIZE) + strlen(metadataFile->size) + 2 + 1);
	strcpy(line,SIZE);
	strcat(line,"=");
	strcat(line,metadataFile->size);
	fwrite(line, strlen(line),1,f);
	fwrite(&rc,sizeof(char),1,f);

	fwrite(&BLOCKS,strlen(BLOCKS),1,f);
	fwrite(&equal,1,1,f);
	fwrite(&sepLeft,sizeof(char),1,f);

	int index = 0;
	while(list_get(metadataFile->block, index) != NULL){
		fwrite(list_get(metadataFile->block, index), strlen(list_get(metadataFile->block, index)),1,f);
		index++;
		if (list_get(metadataFile->block, index) != NULL)
			fwrite(&comma,sizeof(char),1,f);
	}
	fwrite(&sepRight,sizeof(char),1,f);
	fwrite(&rc,sizeof(char),1,f);

	free(line);

	line = (char*)malloc(strlen(OPEN) + 1 + 2 + 1);
	strcpy(line,OPEN);
	strcat(line,"=");
	fwrite(line, strlen(line),1,f);
	fwrite(&(metadataFile->open),sizeof(char),1,f);
	fwrite(&rc,sizeof(char),1,f);

	free(line);

	fclose(f);
	return 0;
}

int create_file_bin(t_values* values){

	int bit = (int)list_get(values->values,0);

	char* blockChar =(char*)malloc(strlen(string_itoa((bit/GameCard->block_size))) + 1 );
	strcpy(blockChar,string_itoa(bit/(GameCard->block_size*8)));

	char* filename = (char*)malloc(strlen(GameCard->blocksPath) + strlen(blockChar) + strlen(".bin") + 1 );
	strcpy(filename,GameCard->blocksPath);
	strcat(filename,blockChar);
	strcat(filename,".bin");

	FILE* f= fopen(filename,"wb");

	if (f==NULL){
		log_debug(GameCard->logger,"Error en la creacion del archivo %s", filename);
		return -1;
	}

	char* bytes = (char*)list_get(values->values,1);
	int tam =(int)list_get(values->values,2);
	fwrite(bytes,tam, 1, f);

	turn_a_set_of_bits_on(bit,(int)list_get(values->values,2));
	free(filename);
	free(blockChar);

	fclose(f);
	return 0;
}


int create_file_bitmap(){

	char zero = '0';
	char one = '1';
	char rc='\n';

	char* filename = (char*)malloc(strlen(GameCard->metadataPath) + strlen("bitmap.bin") + 1);
	strcpy(filename,GameCard->metadataPath);
	strcat(filename,"bitmap.bin");

	FILE* f= fopen(filename,"ab");

	if (f==NULL){
		log_debug(GameCard->logger,"Error en la creacion del archivo %s", filename);
		return -1;
	}

	int max = bitarray_get_max_bit(GameCard->bitArray);
	log_debug(GameCard->logger,"El último bit es %d", max);

	for (int i = 0; i <= max; i= i + 1*8*GameCard->block_size){
		if(bitarray_test_bit(GameCard->bitArray,i)){
			fwrite(&one,sizeof(char),1,f);
		}else{
			fwrite(&zero,sizeof(char),1,f);
		}
	}

	fwrite(&rc,sizeof(char),1,f);

//	GameCard->fileMapped = (char*)malloc(sizeof(char));
//	if ((GameCard->fileMapped = mmap(0,(GameCard->blocks*GameCard->block_size),PROT_READ,MAP_SHARED,(int)f,0)) ==  MAP_FAILED)
//		{
//			log_debug(GameCard->logger, "Error mapping the file");
//			log_debug(GameCard->logger, "Mensaje de error %s", perror("nmap");
//		}
//
//	log_debug(GameCard->logger,"Archivo Mapeado: %s ", GameCard->fileMapped);
	free(filename);
	fclose(f);
	return 0;
}

int create_file_metadata(t_values* values){
	char salto = '\n';
	char* filename = (char*)malloc(strlen(GameCard->metadataPath) + strlen("metadata.bin") + 1 );
	strcpy(filename,GameCard->metadataPath);
	strcat(filename,"metadata.bin");

	FILE* f= fopen(filename,"wb");

	if (f==NULL){
		log_debug(GameCard->logger,"Error en la creacion del archivo %s", filename);
		return -1;
	}

	char* line=(char*)malloc(strlen(BLOCK_SIZE) + strlen("=") + strlen((char*)list_get(values->values,0)) + 1 ) ;
	strcpy(line,BLOCK_SIZE);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,0));
	fwrite(line, strlen(line), 1, f);
	fwrite(&salto, 1, 1, f);
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);

	line = (char*)malloc(strlen(BLOCKS) + strlen("=") + strlen((char*)list_get(values->values,1)));
	strcpy(line,BLOCKS);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,1));
	fwrite(line, strlen(line), 1, f);
	fwrite(&salto, 1, 1, f);
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);

	line = (char*)malloc(strlen(MAGIC_NUMBER) + strlen("=") + strlen((char*)list_get(values->values,2)));
	strcpy(line,MAGIC_NUMBER);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,2));
	fwrite(line, strlen(line), 1, f);
	fwrite(&salto, 1, 1, f);
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);


	fclose(f);
	return 0;

}


int create_file_metadata_directory(t_values* values){

	char salto = '\n';
	char* filename = (char*)malloc(strlen(GameCard->filePath) + strlen("metadata.bin") + 1 );
	strcpy(filename,GameCard->filePath);
	strcat(filename,"metadata.bin");

	FILE* f= fopen(filename,"wb");

	if (f==NULL){
		log_debug(GameCard->logger,"Error en la creacion del archivo %s", filename);
		return -1;
	}

	char* line=(char*)malloc(strlen(DIRECTORY) + 4);
	strcpy(line,DIRECTORY);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,0));
	fwrite(line, strlen(line), 1, f);
//	fprintf(f,"%s",line);
	fwrite(&salto,1,1,f);
//	fprintf(f,"%c",'\n');
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);

	fclose(f);
	return 0;
}


//METADATA

void read_metadata_file(t_file_metadata* fileMetadata, t_config* metadataConfig){
	fileMetadata->directory = get_config_value(metadataConfig,GameCard->logger,DIRECTORY);

	fileMetadata->open = get_config_value(metadataConfig,GameCard->logger,OPEN);

	fileMetadata->size= (char*)malloc(strlen(get_config_value(metadataConfig,GameCard->logger,SIZE))+1);
	fileMetadata->size = get_config_value(metadataConfig,GameCard->logger,SIZE);

	fileMetadata->block = list_create();

	int blocksAmount = (atoi(fileMetadata->size)/GameCard->block_size);

	char** blocks = (char**)malloc(sizeof(char*)*blocksAmount);

	blocks = get_config_value_array(metadataConfig,GameCard->logger,BLOCKS);

	for(int i=0; i < blocksAmount; i++){
		list_add(fileMetadata->block,blocks[i]);
	}

}

char* get_file_content(t_file_metadata* fileMetadata){

	char* aux = (char*)malloc(sizeof(char)*GameCard->block_size);

	int index = 0;

	while(list_get(fileMetadata->block, index) != NULL){
		char* block =(char*)malloc(strlen(list_get(fileMetadata->block, index)+1));
		strcpy(block,list_get(fileMetadata->block, index));

		char* blockFile = (char*)malloc(strlen(GameCard->blocksPath) + strlen(block) + strlen(".bin") + 1 );
		strcpy(blockFile,GameCard->blocksPath);
		strcat(blockFile,block);
		strcat(blockFile,".bin");

		FILE* f= fopen(blockFile,"rb");

		if (f==NULL){
			log_debug(GameCard->logger,"Error en la creacion del archivo %s", blockFile);
			return '-1';
		}

		char* buffer=malloc(sizeof(GameCard->block_size));

		fread(buffer,GameCard->block_size,1,f);

		buffer[GameCard->block_size] = '\0';

		if (index==0)
			strcpy(aux,buffer);
		else
			strcat(aux,(char*)buffer);

		free(buffer);
		fclose(f);
		free(block);
		free(blockFile);
		index++;

	}

	return aux;
}
//BITARRAY LOGIC

int get_first_free_block(){
	int max = bitarray_get_max_bit(GameCard->bitArray);
	int i = 0;
	while((bitarray_test_bit(GameCard->bitArray,i)) && (i<=max))
		i = i + 1*8*GameCard->block_size;
	return i;
}

void turn_a_set_of_bits_on(int from,int bytes){
	for (int i=0; i<bytes; i++){
		for (int x=0; x < 8; x++)
			bitarray_set_bit(GameCard->bitArray, x + from);
	}
}

void write_blocks(t_file_metadata* metadataFile, int amountOfBlocks,new_pokemon* newPokemon ){

	int cantBytes=0;
	int index=0;
	int tam;
	int result;

	char* buffer=(char*)malloc(atoi(metadataFile->size));
	char* bufferAux=(char*)malloc(GameCard->block_size);
	strcpy(buffer,string_itoa(newPokemon->horizontalCoordinate));
	strcat(buffer,"-");
	strcat(buffer,string_itoa(newPokemon->verticalCoordinate));
	strcat(buffer,"=");
	strcat(buffer,string_itoa(newPokemon->ammount));;
	tam = strlen(buffer);
	printf("tamaño del buffer % d \n", strlen(buffer));
	buffer[strlen(buffer)] ='\n';
	printf("tamaño del buffer %d \n", strlen(buffer));
	buffer[tam + 1]='\0';
	printf("tamaño del buffer %d \n", strlen(buffer));


	t_values* values = (t_values*)malloc(sizeof(t_values));
	values->values =list_create();


	for (int i = 0; i<amountOfBlocks; i++){

		int bitBlock=get_first_free_block();
		int specificBlock = bitBlock/(8*GameCard->block_size);

		char* charblock=(char*)malloc(strlen(string_itoa(specificBlock))+1);
		charblock = string_itoa(specificBlock);

//		log_debug(GameCard->logger,"Bloque donde va a guardar los datos %s" , charblock);
		list_add(metadataFile->block,(void*)charblock);

		list_add(values->values,(void*)bitBlock);

		index = 0;
		while((GameCard->block_size > index) && (index < tam)){
			printf("Bloque a grabar : %c \n",buffer[cantBytes]);
			bufferAux[index]=buffer[cantBytes];
			cantBytes++;
			index++;
		}

		list_add(values->values,bufferAux);
		list_add(values->values,(void*)index);

		result=create_file(BIN_FILE,values);

		if (result) log_debug(GameCard->logger, "BIN FILE CREADO");

		log_debug(GameCard->logger,"Creo el archivo de bloque del Pokemon %d" , specificBlock);

		list_remove(values->values,2);
		list_remove(values->values,1);
		list_remove(values->values,0);


		free(bufferAux);
		bufferAux=(char*)malloc(GameCard->block_size);

		//TODO:MARCAR EL BITMAP
//		GameCard->fileMapped[specificBlock]=1;
//		log_debug(GameCard->logger, "Archivo mapeado : %s", GameCard->fileMapped);
	}

	list_destroy(values->values);
	free(values);
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

