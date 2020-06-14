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


	char* ip;
	char* puerto;
	//	char* ptoMnt, retryConnection, retryOperation, delayTime;

	int server,client;

	logger = iniciar_logger();
	log_info(logger,"PROCESO GAMECARD ONLINE");

	GameCard=GameCard_initialize(logger);

	config = leer_config();

	ip= obtener_valor_config(config,logger,IP_BROKER);
	puerto = obtener_valor_config(config,logger,PUERTO_BROKER);
	// ptoMnt = obtener_valor_config(config.logger,PUNTO_MONTAJE_TALLGRASS);
	// retryConnection = obtener_valor_config(config.logger,TIEMPO_DE_REINTENTO_CONEXION);
	// retryOperation = obtener_valor_config(config.logger,TIEMPO_DE_REINTENTO_OPERACION);
	// delayTime = obtener_valor_config(config.logger,TIEMPO_RETARDO_OPERACION);


	puts("GameCard (Filesystem)"); /* prints GameCard (Filesystem) */
	return EXIT_SUCCESS;
}

t_GameCard* GameCard_initialize(t_log* logger){
	t_GameCard* aux = (t_GameCard*)malloc(sizeof(t_GameCard));
	aux->logger = logger;
	return aux;
}
