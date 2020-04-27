/*
 ============================================================================
 Name        : Team.c
 Author      : Mauro
 Version     :
 Copyright   : Your copyright notice
 Description : Team Process
 ============================================================================
 */


#include "Team.h"

int main(void) {
	t_config* config;
	t_log* logger;
	int teamSocket;
	char* ip;
	char* port;
	char* logFilename;
	void* mensaje = "Mensaje de prueba";
	pthread_t tid[2];
	sem_t sem;

	config = leer_config();
	if (config_has_property(config,"LOG_FILE")){
		logFilename=config_get_string_value(config,"LOG_FILE");
		removeLogger(logFilename);
		logger = startLogger(logFilename);
		log_debug(logger,config_get_string_value(config,"LOG_FILE"));
	}


	if (config_has_property(config,"IP_BROKER")){
		log_debug(logger,config_get_string_value(config,"IP_BROKER"));
		ip=config_get_string_value(config,"IP_BROKER");
	}

	if (config_has_property(config,"PUERTO_BROKER")){
		log_debug(logger,config_get_string_value(config,"PUERTO_BROKER"));
		port=config_get_string_value(config,"PUERTO_BROKER");
	}

	log_debug(logger,"Process Team Started");

	//Cuando el team funciona como server

	teamSocket = iniciar_cliente(ip, port,logger);
	log_debug(logger,"El socket del proceso Team es %i",teamSocket);
	enviar_mensaje(mensaje,strlen(mensaje),teamSocket);
	serve_client(&teamSocket,logger);
	deleteLogger(logger);
	return EXIT_SUCCESS;
}

t_log* startLogger(char* logFilename)
{
	t_log* logger;
	logger = log_create(logFilename, "Team", 1, LOG_LEVEL_DEBUG);
	if (logger == NULL){
		printf("No se pudo crear el logger\n");
		exit(1);
	}
	return logger;
}
void deleteLogger(t_log* logger)
{
	if (logger != NULL){
		log_destroy(logger);
	}
}
void removeLogger(char* logFilename)
{
	remove(logFilename);
}
t_config* leer_config(void)
{
	t_config* config;
	config = config_create("Team.config");
	if (config == NULL){
		printf("No se pudo crear el config\n");
		exit(2);
	}
	return config;

}
