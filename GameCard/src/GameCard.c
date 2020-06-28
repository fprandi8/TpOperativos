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
t_dictionary *pokeSemaphore;

int main(void) {

	t_log* logger;
	t_config* config;
//	pthread_t* subs;

//	char* ip,puerto;
	char* ptoMnt;
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
	free(message->messageContent);
	free(message);

//	message = (deli_message*)malloc(sizeof(deli_message));
//	message->id = 88;
//	message->correlationId=99;
//	message->messageType=NEW_POKEMON;
//
//	newPokemon = (new_pokemon*)malloc(sizeof(new_pokemon));
//	newPokemon->ammount=10;
//	newPokemon->verticalCoordinate = 8;
//	newPokemon->horizontalCoordinate = 4;
//	newPokemon->pokemonName = "Pikachu";
//
//	message->messageContent = (void*)newPokemon;
//
//	GameCard_Process_Message(message);
//	free(message->messageContent);
//	free(message);

	munmap(GameCard->fileMapped, (GameCard->blocks/8));

	config_destroy(config);

	GameCard_Destroy(GameCard);

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
void GameCard_Destroy(t_GameCard* GameCard){
	free(GameCard->blocksPath);
	free(GameCard->filePath);
	free(GameCard->metadataPath);
	free(GameCard->ptoMnt);
	free(GameCard->bitArray->bitarray);
	bitarray_destroy(GameCard->bitArray);
	log_destroy(GameCard->logger);
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

		char* blocksize= get_config_value(config,GameCard->logger,BLOCK_SIZE);

		char* blocks=get_config_value(config,GameCard->logger,BLOCKS);

		char* magicNumber = get_config_value(config,GameCard->logger,MAGIC_NUMBER);

		values->values= list_create();

		list_add(values->values,blocksize);
		list_add(values->values,blocks);
		list_add(values->values,magicNumber);

		result = create_file(METADATA,values);

		GameCard->block_size = atoi(blocksize);
		GameCard->blocks = atoi(blocks);

		list_destroy(values->values);

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
//	list_destroy_and_destroy_elements(values->values,free);
	list_destroy(values->values);
	free(values);
	free(directory);
}

void GameCard_Initialize_bitarray(){
//	t_bitarray* aux = (t_bitarray*)malloc(sizeof(t_bitarray));
	t_bitarray* aux;
	int size = (GameCard->blocks/8);

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

	config_destroy(metadataConfig);

	// Levantar el archivo leyendo todos los bloques
	char* fileContent = get_file_content(metadataFile);

	log_debug(GameCard->logger,"Contenido del archivo %s", fileContent);

	new_pokemon* newPokemon = (new_pokemon*)list_get(values->values,0);

	char* horCoordinate = string_itoa(newPokemon->horizontalCoordinate);
	char* verCoordinate = string_itoa(newPokemon->verticalCoordinate);

	char* coordinate=(char*)malloc(strlen(horCoordinate)
			+strlen(verCoordinate)+2);

	strcpy(coordinate,horCoordinate);
	strcat(coordinate,"-");
	strcat(coordinate,verCoordinate);

	if (string_contains(fileContent,coordinate))
	{
		int pos= get_string_file_position(fileContent,coordinate);
		increase_pokemon_amount(fileContent,pos,newPokemon->ammount);
		rewrite_blocks(metadataFile, fileContent);

	}
	else
	{
		//TODO: Revisar si el último bloque tiene espacio libre en el bit array
		//TODO: Si el bloque tiene espacio llenarlo primero
		//TODO: Luego, si aún queda mensaje por grabar crearlo

		int size = get_message_size(newPokemon);

		int amountOfBlocks=get_amount_of_blocks(size);

		char* buffer=serialize_data(atoi(metadataFile->size),newPokemon);

		write_blocks(metadataFile, amountOfBlocks, buffer);

		free(buffer);
	}

	free(horCoordinate);
	free(verCoordinate);
	free(coordinate);

	free(fileContent);
	free(file);

	Metadata_File_Destroy(metadataFile);

	return 0;
}

int create_poke_file(t_values* values){

	new_pokemon* newPokemon = (new_pokemon*)list_get(values->values,0);

	t_file_metadata* metadataFile = (t_file_metadata*)malloc(sizeof(t_file_metadata));

	metadataFile->block= list_create();

	int size = get_message_size(newPokemon);

	int amountOfBlocks=get_amount_of_blocks(size);

	metadataFile->size =  string_itoa(size);

	log_debug(GameCard->logger, "Amout of blocks needed %d", amountOfBlocks);

	int result;

	char* buffer=serialize_data(atoi(metadataFile->size),newPokemon);

	write_blocks(metadataFile, amountOfBlocks, buffer);

	metadataFile->directory='N';
	metadataFile->open='N';

	list_add(values->values,metadataFile);

	result=create_file(POKE_METADATA,values);

	Metadata_File_Destroy(metadataFile);

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
	free(filename);
	fclose(f);
	return 0;
}

int create_file_bin(t_values* values){

	int bit = (int)list_get(values->values,0);

	char* blockChar =string_itoa(bit);

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

	turn_bit_on(bit);
	free(filename);
	free(blockChar);

	fclose(f);
	return 0;
}


int create_file_bitmap(){

	char* filename = (char*)malloc(strlen(GameCard->metadataPath) + strlen("bitmap.bin") + 1);
	strcpy(filename,GameCard->metadataPath);
	strcat(filename,"bitmap.bin");

	int f = open(filename, O_RDWR | O_CREAT, (mode_t)0600);

//	FILE* f= fopen(filename,"ab");

	if (f==-1){
		log_debug(GameCard->logger,"Error en la creacion del archivo %s", filename);
		return -1;
	}

	int max = bitarray_get_max_bit(GameCard->bitArray);
	log_debug(GameCard->logger,"El último bit es %d", max);

	for (int i = 0; i <= max ; i++){

		write(f,&GameCard->bitArray->bitarray[i],1);
	}

//	GameCard->fileMapped = (char*)malloc(sizeof(char));
	if ((GameCard->fileMapped = mmap(0,(GameCard->blocks/8),PROT_READ|PROT_WRITE,MAP_SHARED,f,0)) ==  MAP_FAILED)
		{
			log_debug(GameCard->logger, "Error mapping the file");
			perror("nmap");
			printf("\n");
			close(f);
		}

	log_debug(GameCard->logger,"Archivo Mapeado ");
	free(filename);
//	fclose(f);
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

	line = (char*)malloc(strlen(BLOCKS) + strlen("=") + strlen((char*)list_get(values->values,1))+1);
	strcpy(line,BLOCKS);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,1));
	fwrite(line, strlen(line), 1, f);
	fwrite(&salto, 1, 1, f);
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);

	line = (char*)malloc(strlen(MAGIC_NUMBER) + strlen("=") + strlen((char*)list_get(values->values,2))+1);
	strcpy(line,MAGIC_NUMBER);
	strcat(line, "=");
	strcat(line, (char*)list_get(values->values,2));
	fwrite(line, strlen(line), 1, f);
	fwrite(&salto, 1, 1, f);
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);

	free(filename);

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
	fwrite(&salto,1,1,f);
	log_debug(GameCard->logger,"Linea del archivo : %s", line);
	free(line);

	free(filename);
	fclose(f);
	return 0;
}


//METADATA

void Metadata_File_Destroy(t_file_metadata* metadataFile){
	free(metadataFile->size);
	list_destroy_and_destroy_elements(metadataFile->block,free);
	free(metadataFile);
}

void read_metadata_file(t_file_metadata* metadataFile, t_config* metadataConfig){

	//TODO: agregar logica de diccionario de semaforos, wait,
	//      revisar el flag, si aplica cambiar flag y signal,
	//      sino signal y manejo de archivo no disponible

	metadataFile->open = *(get_config_value(metadataConfig,GameCard->logger,OPEN));
//	fileMetadata->open = get_config_value(metadataConfig,GameCard->logger,OPEN);

	metadataFile->directory= *(get_config_value(metadataConfig,GameCard->logger,DIRECTORY));
//	fileMetadata->directory = get_config_value(metadataConfig,GameCard->logger,DIRECTORY);

	metadataFile->size= (char*)malloc(strlen(get_config_value(metadataConfig,GameCard->logger,SIZE))+1);
	strcpy(metadataFile->size,get_config_value(metadataConfig,GameCard->logger,SIZE));
//	fileMetadata->size = get_config_value(metadataConfig,GameCard->logger,SIZE);

	metadataFile->block = list_create();

	int blocksAmount = get_amount_of_blocks(atoi(metadataFile->size));

//	char** blocks = (char**)malloc(sizeof(char*)*blocksAmount);

	char** blocks = get_config_value_array(metadataConfig,GameCard->logger,BLOCKS);

	for(int i=0; i < blocksAmount; i++){
		char* aux = string_duplicate(blocks[i]);
		list_add(metadataFile->block,aux);
	}

	free(blocks);

}

char* get_file_content(t_file_metadata* metadataFile){

	log_debug(GameCard->logger, "Cantidad de bloques necesarios %d " , list_size(metadataFile->block));

	char* aux = (char*)malloc(sizeof(char)*GameCard->block_size * list_size(metadataFile->block)  + 1 );

	int index = 0;

	while(list_get(metadataFile->block, index) != NULL){
		char* block =(char*)malloc(strlen(list_get(metadataFile->block, index))+1);
		strcpy(block,list_get(metadataFile->block, index));

		char* blockFile = (char*)malloc(strlen(GameCard->blocksPath) + strlen(block) + strlen(".bin") + 1 );
		strcpy(blockFile,GameCard->blocksPath);
		strcat(blockFile,block);
		strcat(blockFile,".bin");

		FILE* f= fopen(blockFile,"rb");

		if (f==NULL){
			log_debug(GameCard->logger,"Error en la creacion del archivo %s", blockFile);
			return '1';
		}

		char* buffer=malloc(GameCard->block_size + 1);

		fread(buffer,GameCard->block_size,1,f);

		buffer[GameCard->block_size] = '\0';

		if (index==0)
			strcpy(aux,(char*)buffer);
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

void increase_pokemon_amount(char* fileContent, int pos, int amount){

	int index1=0;
	int index2=0;

	while (fileContent[pos + index1] != '=') index1++;

	while (fileContent[pos + index1 + index2] != '\n') index2++;

	char* stringAux = string_substring(fileContent,pos+index1+1,index2-1);

	int chars = strlen(stringAux);

	int auxAmount = atoi(stringAux) + amount;

	char* newAmount = string_itoa(auxAmount);

	if (chars < strlen(newAmount)){
		// Tratamiento para el archivo desplazar los espacios necesarios
		// Debería pedir un bloque mas si excede e insertarlo en la metadata en el medio? eso resolveria pero me queda un bloque con
		// los bytes que exceden y todo basura...
		char* auxBuffer=(char*)malloc(strlen(fileContent) + (strlen(newAmount) - chars));
		memcpy(auxBuffer,fileContent,(pos+index1));
		memcpy(auxBuffer+(pos+index1)+strlen(newAmount),fileContent + pos +index1 + chars,strlen(fileContent)-pos +index1 + chars);
		free(fileContent);
		fileContent = auxBuffer;
	}

	for(int i=0; i<strlen(newAmount); i++)
	{
		fileContent[pos + index1 + 1 + i] = newAmount[i];
	}

	free(stringAux);
	free(newAmount);
}

//BITARRAY LOGIC

int get_first_free_block(){
	int max = bitarray_get_max_bit(GameCard->bitArray);
	int i = 0;
	while((bitarray_test_bit(GameCard->bitArray,i)) && (i<=max))
	{
		log_debug(GameCard->logger, "Resultado del test del bit %d:  -  ", i,  bitarray_test_bit(GameCard->bitArray,i));
		i++;
	}
	return i;
}

void turn_bit_on(int block){
	bitarray_set_bit(GameCard->bitArray, block);
	//TODO:MARCAR EL BITMAP - Ver si esto esta bien...
	GameCard->fileMapped[block]='1';
}

int get_amount_of_blocks(int size){

	if (size % GameCard->block_size == 0)
	    return size/GameCard->block_size;
	else
	    return (size/GameCard->block_size) + 1;

}

int get_message_size(new_pokemon* newPokemon){

	int size=0;

	char* horCoordinate = string_itoa(newPokemon->horizontalCoordinate);
	char* verCoordinate = string_itoa(newPokemon->verticalCoordinate);
	char* amount = string_itoa(newPokemon->ammount);

	//Sumo 1 por el \n
	size = (strlen(horCoordinate)) + 1 + (strlen(verCoordinate)) + 1 + (strlen(amount)) + 1;
	free(horCoordinate);
	free(verCoordinate);
	free(amount);

	return size;
}

char* serialize_data(int size, new_pokemon* newPokemon ){
	int tam;

	char* buffer=(char*)malloc(size + 1);

	char* horCoordinate = string_itoa(newPokemon->horizontalCoordinate);
	char* verCoordinate = string_itoa(newPokemon->verticalCoordinate);
	char* amount = string_itoa(newPokemon->ammount);

	strcpy(buffer,horCoordinate);
	strcat(buffer,"-");
	strcat(buffer,verCoordinate);
	strcat(buffer,"=");
	strcat(buffer,amount);;

	tam = strlen(buffer);
	buffer[strlen(buffer)] ='\n';
	buffer[tam + 1]='\0';

	free(horCoordinate);
	free(verCoordinate);
	free(amount);

	return buffer;
}

void write_blocks(t_file_metadata* metadataFile, int amountOfBlocks, char* buffer ){

	int cantBytes=0;
	int index=0;
	int result;

	int tam = strlen(buffer);


	char* bufferAux=(char*)malloc(GameCard->block_size);

	t_values* values = (t_values*)malloc(sizeof(t_values));
	values->values =list_create();


	for (int i = 0; i<amountOfBlocks; i++){

		int bitBlock=get_first_free_block();

		char* charblock = string_itoa(bitBlock);

		log_debug(GameCard->logger,"Bloque donde va a guardar los datos %s" , charblock);
		list_add(metadataFile->block,(void*)charblock);

		list_add(values->values,(void*)bitBlock);

		index = 0;
		while((GameCard->block_size > index) && (index < tam) && (buffer[cantBytes] != '\0')){
			printf("Bloque a grabar : %c \n",buffer[cantBytes]);
			bufferAux[index]=buffer[cantBytes];
			cantBytes++;
			index++;
		}

		list_add(values->values,bufferAux);
		list_add(values->values,(void*)index);

		result=create_file(BIN_FILE,values);

		if (result) log_debug(GameCard->logger, "BIN FILE CREADO");

		log_debug(GameCard->logger,"Creo el archivo de bloque del Pokemon %d" , bitBlock);

		list_remove(values->values,2);
		list_remove(values->values,1);
		list_remove(values->values,0);


		free(bufferAux);
		bufferAux=(char*)malloc(GameCard->block_size);
	}

	free(bufferAux);
	list_destroy(values->values);
	free(values);
}

void rewrite_blocks(t_file_metadata* metadataFile, char* fileContent ){

	int index = 0;
	int currentBlock = 0;

	while(list_get(metadataFile->block, index) != NULL){

		char* auxblock= (char*)list_get(metadataFile->block, index);

		char* block =(char*)malloc(strlen(auxblock)+1);
		strcpy(block,list_get(metadataFile->block, index));

		char* blockFile = (char*)malloc(strlen(GameCard->blocksPath) + strlen(block) + strlen(".bin") + 1 );
		strcpy(blockFile,GameCard->blocksPath);
		strcat(blockFile,block);
		strcat(blockFile,".bin");

		FILE* f= fopen(blockFile,"wb");

		if (f==NULL){
			log_debug(GameCard->logger,"Error en la creacion del archivo %s", blockFile);
			return ;
		}

//		char* bufferAux=malloc(GameCard->block_size);

		char* bufferAux = string_substring(fileContent,currentBlock,GameCard->block_size);

		fwrite(bufferAux,GameCard->block_size,1,f);
		log_debug(GameCard->logger, "Re escribio el block %s", block);

		currentBlock = currentBlock + GameCard->block_size;

		free(bufferAux);
		fclose(f);
		free(block);
		free(blockFile);
		index++;

	}

}

int get_string_file_position(char* fileContent, char* coordinates){


	int line = 0;
	int match = 0;

	while(!match)
	{
		int pos=0;

		while(fileContent[pos + line] != '\n') pos++;

		char* stringAux = string_substring(fileContent,line,pos);

		if (string_contains(fileContent,coordinates))
			match=1;
		else
			line = line+pos+1;

		free(stringAux);
	}

	return line;
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

