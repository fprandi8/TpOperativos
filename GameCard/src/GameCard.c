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

	int result = create_directory("/home/utnso/desktop/");

	result = create_directory(ptoMnt);

	log_debug(logger, "Resultado de la creacion del directorio: %s - %d",ptoMnt, result);
//TODO: HACER ESTO LINDO...
	char* metadata = (char*)malloc(strlen(ptoMnt)+strlen("/metadata/") + 1);
	strcpy(metadata,ptoMnt);
	strcat(metadata,"/metadata/");
	result = create_directory(metadata);

	log_debug(logger, "Resultado de la creacion del directorio: %s - %d", metadata, result);

	char* metadataFile = (char*) malloc(strlen(metadata) + strlen("metadata.bin") +1);
	strcpy(metadataFile,metadata);
	free(metadata);

	strcat(metadataFile,"metadata.bin");

	t_values* values = (t_values*)malloc(sizeof(t_values));

	values->values= list_create();
	char* value1 = "64";
	char* value2 = "5192";
	char* value3 = "TALL_GRASS";
	list_add(values->values,value1);
	list_add(values->values,value2);
	list_add(values->values,value3);

	result = create_file(metadataFile, METADATA,values);

	list_destroy(values->values);
	free(values);
	free(ptoMnt);

	return EXIT_SUCCESS;
}

t_GameCard* GameCard_initialize(t_log* logger, char* ptoMnt){
	log_debug(logger,"Inicializa la GameCard");
	t_GameCard* aux = (t_GameCard*)malloc(sizeof(t_GameCard));
	aux->ptoMnt = (char*)malloc(strlen(ptoMnt));
	strcpy(aux->ptoMnt,ptoMnt);
	aux->logger = logger;
	return aux;
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
	f= fopen(filename,"w");
	if (f==NULL){
		log_debug(GameCard->logger,"Error en la creacion del archivo %d");
		return -1;
	}

	switch(fileType){
		case METADATA:
		{
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
			break;
		}
		case POKE_METADATA:
			break;

		case POKE_FILE:
			break;

		case BITMAP:
			break;
	}

	log_debug(GameCard->logger,"Archivo: %s", filename);

	return 0;
}
