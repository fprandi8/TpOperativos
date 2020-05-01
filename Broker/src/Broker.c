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
	t_queue_handler* colaDePrueba;
	t_list* mensajes;

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

	char* nombre = "FEDE";

	colaDePrueba = inicializar_queue_handler(nombre);

	list_add(colaDePrueba->suscriptores, "un suscriptor");
	printf("El suscriptor que tengo %s \n", list_get(colaDePrueba->suscriptores,0));

	mensajes = list_create();
	char* msj1 = "hola";
	char* msj2 = "chau";

	list_add(mensajes,msj1);
	list_add(mensajes,msj2);

	queue_push(colaDePrueba->queue,mensajes);
	t_list* aux;

	aux = queue_peek(colaDePrueba->queue);

	printf("El mensaje de la lista %s \n", list_get(aux,0));

	list_destroy(mensajes);
	destroy_queue_handler(colaDePrueba);

//	iniciar_servidor(ip, puerto);

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

t_queue_handler* inicializar_queue_handler(char* nombre){
	t_queue_handler* aux;
	aux = malloc(sizeof(t_queue_handler));
	aux->nombre =malloc(sizeof(char) * strlen(nombre));
	aux->queue = queue_create();
	aux->suscriptores = list_create();
	strcpy(aux->nombre,nombre);
	return aux;
}

void destroy_queue_handler(t_queue_handler* self){
	free(self->nombre);
	queue_destroy(self->queue);
	list_destroy(self->suscriptores);
}

t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* self,int pos){
	return list_get(self->suscriptores,pos);
}
