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


sem_t* mutex_id;
uint32_t ID = 0;

int main(void) {

	t_log* logger;
	t_config* config;


	char* ip;
	char* puerto;
	int servidor,cliente;
	char* tam_men, tam_min_part, algo_mem, algo_reem, algo_part, frec_comp;
	char* nombre;

	pthread_t* thread;

	thread = (pthread_t*)malloc(sizeof(pthread_t));

	nombre = (char*)malloc(strlen("mutex_id")+1);

	//attach to semaphore, if not exist will be created
	mutex_id = sem_open(nombre, O_CREAT, S_IRWXU, 1);

	logger = iniciar_logger();
	log_info(logger,"PROCESO BROKER ONLINE");

	config = leer_config();

	ip= obtener_valor_config(config,logger,IP_BROKER);
	puerto = obtener_valor_config(config,logger,PUERTO_BROKER);
	tam_men = obtener_valor_config(config,logger,TAMANO_MEMORIA);
	tam_min_part = obtener_valor_config(config,logger,TAMANO_MINIMO_PARTICION);
	algo_mem= obtener_valor_config(config,logger,ALGORITMO_MEMORIA);
	algo_reem= obtener_valor_config(config,logger,ALGORITMO_REEMPLAZO);
	algo_part= obtener_valor_config(config,logger,ALGORITMO_PARTICION_LIBRE);
	frec_comp= obtener_valor_config(config,logger,FRECUENCIA_COMPACTACION);

	t_list* queue_list;

	queue_list = inicializar_queues();

	servidor = iniciar_servidor(ip, puerto);

	log_debug(logger,"SERVIDOR INICIADO");

	while (1){

		cliente = esperar_cliente(servidor);

		t_args* args= (t_args*) malloc (sizeof (t_args*));

		args->cliente = (int*)malloc (sizeof(int*));;
		args->mutex = (sem_t*)malloc (sizeof(sem_t*));
		args->queues = (t_list*) malloc (sizeof(t_list*));

		args->cliente = &cliente;
		args->mutex=mutex_id;
		args->queues = queue_list;

		printf("Crea un hilo para el cliente %d \n", cliente);

		pthread_create(thread,NULL,(void*)serve_client,args);
		pthread_detach(*thread);

		free(args->mutex);
		//free(args->cliente);
		//free(args);
	}

	int resultado = destroy_queue_list(queue_list);

	if (resultado == 0){
		log_debug(logger,"SE LIBERARON LOS RECURSOS DE LAS QUEUE");
	}


	sem_close(mutex_id);
	sem_unlink(nombre);
	return EXIT_SUCCESS;
}



void serve_client(void* variables)
{
	char* msg = "CONECTADO \n";
	int cliente = *((t_args*)variables)->cliente;
	t_list* queue_list = ((t_args*)variables)->queues;
	send(cliente, msg, strlen(msg), 0);

//	sem_wait(mutex_id);
//	printf("Cliente %d \n", (*((t_arg_get_id*)variables)->cliente));
//	printf("ID del mensaje %d \n", ID);
//	printf("Mensaje recibido del thread--> %s \n" ,  ((t_arg_get_id*)variables)->asd);
//	ID++;
//	sem_post(mutex_id);

	puts("espera el paquete");

	t_package* package = GetPackage(cliente);
	recibir_mensaje(package, cliente, queue_list);

}


void recibir_mensaje(t_package* package, int cliente, t_list* queue_list){

	puts("recibió el mensaje");
	switch (package->operationCode){
		case SUBSCRIPTION:
			procesar_suscripcion(package->buffer, cliente, queue_list);
			break;
		case MESSAGE:
			procesar_mensaje(package->buffer, queue_list);
			break;
		case ACKNOWLEDGE:
			recibir_acknowledge(package->buffer);
			break;
	}
}


void procesar_suscripcion(t_buffer* buffer, int cliente, t_list* queue_list){


}

void procesar_mensaje(t_buffer* buffer,t_list* queue_list){
	t_message* msj = malloc(buffer->bufferSize);
	memcpy(msj,buffer->stream,buffer->bufferSize);
	switch (msj->messageType){
		case NEW_POKEMON:
			break;
		case LOCALIZED_POKEMON:
			break;
		case GET_POKEMON:
			break;
		case APPEARED_POKEMON:
			break;
		case CATCH_POKEMON:
			break;
		case CAUGHT_POKEMON:
			break;
	}

	free(msj);
}

void recibir_acknowledge(t_buffer* buffer){

}

t_log* iniciar_logger(void)
{
	return log_create("Broker.log","Broker",1,LOG_LEVEL_DEBUG);
}

t_config* leer_config(void)
{
	return config_create("/home/utnso/workspace/tp-2020-1c-MATE-OS/Broker/Broker.config");

}

char* obtener_valor_config(t_config* config, t_log* logger, char* propiedad){

	if (config_has_property(config,propiedad)){
		log_info(logger,config_get_string_value(config,propiedad));
		return config_get_string_value(config, propiedad);
	}
	return NULL;
}

t_list* inicializar_queues(){
	t_list* aux;
	aux= list_create();

	for (int i=0; i < 6; i++){
		t_queue_handler* queue_handler;
		queue_handler= inicializar_queue_handler((message_type)i);
		list_add(aux,queue_handler);
	}

	return aux;
}

int destroy_queue_list(t_list* self){
	list_destroy_and_destroy_elements(self,(void*)destroy_queue_handler);
	return 0;
}

t_queue_handler* inicializar_queue_handler(message_type tipo){
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

void suscribir_proceso(t_queue_handler* queue, int cliente){
	t_suscriptor* aux;
	aux= malloc(sizeof(t_suscriptor));
	aux->suscripto = cliente;
	list_add(queue->suscriptores,aux);
}

