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
	int servidor,cliente;

	logger = iniciar_logger();

	log_info(logger,"PROCESO BROKER ONLINE");

	config = leer_config();

	if (config_has_property(config,"IP_BROKER")){
		log_info(logger,config_get_string_value(config,"IP_BROKER"));
		ip=config_get_string_value(config,"IP_BROKER");
	}

	if (config_has_property(config,"PUERTO_BROKER")){
		log_info(logger,config_get_string_value(config,"PUERTO_BROKER"));
		puerto=config_get_string_value(config,"PUERTO_BROKER");
	}

	t_list* queue_list;

	queue_list = inicializar_queues();
	int resultado = destroy_queue_list(queue_list);

	if (resultado == 0){
		log_debug(logger,"SE LIBERARON LOS RECURSOS DE LAS QUEUE");
	}

	servidor = iniciar_servidor(ip, puerto);

	log_debug(logger,"SERVIDOR INICIADO");

	while (1){
		cliente = esperar_cliente(servidor);
		pthread_create(&thread,NULL,(void*)serve_client,&cliente);
		pthread_detach(thread);
	}

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

t_list* inicializar_queues(){
	t_list* aux;
	aux= list_create();

	for (int i=0; i < 7; i++){
		t_queue_handler* queue_handler;
		queue_handler= inicializar_queue_handler((t_queue_type)i);
		list_add(aux,queue_handler);
	}

	return aux;
}

int destroy_queue_list(t_list* self){
	list_destroy_and_destroy_elements(self,(void*)destroy_queue_handler);
	return 0;
}

t_queue_handler* inicializar_queue_handler(t_queue_type tipo){
	t_queue_handler* aux;
	aux = malloc(sizeof(t_queue_handler));
	aux->queue = queue_create();
	aux->suscriptores = list_create();
	aux->tipo=tipo;
	return aux;
}

void destroy_queue_handler(t_queue_handler* self){
	queue_destroy(self->queue);
	list_destroy(self->suscriptores);
}

t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* self,int pos){
	return list_get(self->suscriptores,pos);
}

