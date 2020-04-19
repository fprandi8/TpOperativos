/*
 ============================================================================
 Name        : Broker.c
 Author      : Mauro
 Version     :
 Copyright   : Your copyright notice
 Description : Broker process
 ============================================================================
 */

#include "Broker.h"

int main(void) {

	t_log* logger;
	t_config* config;

	char* ip;
	char* puerto;

	logger = iniciar_logger();

	log_debug(logger,"PROCESO BROKER ONLINE");

	config = leer_config();

	if (config_has_property(config,"IP_BROKER")){
		log_debug(logger,config_get_string_value(config,"IP_BROKER"));
		ip=config_get_string_value(config,"IP_BROKER");
	}

	if (config_has_property(config,"PUERTO_BROKER")){
		log_debug(logger,config_get_string_value(config,"PUERTO_BROKER"));
		puerto=config_get_string_value(config,"PUERTO_BROKER");
	}

	iniciar_servidor(ip, puerto);

	return EXIT_SUCCESS;
}

t_log* iniciar_logger(void)
{
	return log_create("Broker.log","Broker",1,LOG_LEVEL_DEBUG);
}

t_config* leer_config(void)
{
	return config_create("/home/utnso/workspace/tp-2020-1c-MATE-OS/Broker/Broker.config");

}
