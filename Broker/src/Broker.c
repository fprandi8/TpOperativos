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
//	char* tam_men, tam_min_part, algo_mem, algo_reem, algo_part, frec_comp;
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
//	tam_men = obtener_valor_config(config,logger,TAMANO_MEMORIA);
//	tam_min_part =(char*) obtener_valor_config(config,logger,TAMANO_MINIMO_PARTICION);
//	algo_mem=  obtener_valor_config(config,logger,ALGORITMO_MEMORIA);
//	algo_reem=  obtener_valor_config(config,logger,ALGORITMO_REEMPLAZO);
//	algo_part=  obtener_valor_config(config,logger,ALGORITMO_PARTICION_LIBRE);
//	frec_comp=  obtener_valor_config(config,logger,FRECUENCIA_COMPACTACION);

	t_Broker* broker= (t_Broker*) malloc(sizeof(t_Broker));

	Broker_initialize(broker);

	servidor = iniciar_servidor(ip, puerto);

	log_debug(logger,"SERVIDOR INICIADO");

	while (1){

		cliente = esperar_cliente(servidor);

		t_args* args= (t_args*) malloc (sizeof (t_args*));

		args->cliente = (int*)malloc (sizeof(int*));;
		args->mutex = (sem_t*)malloc (sizeof(sem_t*));
		args->broker = (t_Broker*) malloc (sizeof(t_Broker*));

		args->cliente = &cliente;
		args->mutex=mutex_id;
		args->broker = broker;

		printf("Crea un hilo para el cliente %d \n", cliente);

		pthread_create(thread,NULL,(void*)serve_client,args);
		pthread_detach(*thread);

		free(args->mutex);
		//free(args->cliente);
		//free(args);
	}

	Broker_destroy(broker);

	sem_close(mutex_id);
	sem_unlink(nombre);
	return EXIT_SUCCESS;
}



void serve_client(void* variables)
{
	char* msg = "CONECTADO \n";
	int cliente = *((t_args*)variables)->cliente;
	t_Broker* broker = ((t_args*)variables)->broker;
	send(cliente, msg, strlen(msg), 0);

//	sem_wait(mutex_id);
//	printf("Cliente %d \n", (*((t_arg_get_id*)variables)->cliente));
//	printf("ID del mensaje %d \n", ID);
//	printf("Mensaje recibido del thread--> %s \n" ,  ((t_arg_get_id*)variables)->asd);
//	ID++;
//	sem_post(mutex_id);

	puts("espera el paquete");

	t_package* package = GetPackage(cliente);
	recive_message(package, cliente, broker);

}


void recive_message(t_package* package, int cliente, t_Broker* broker){

	puts("recibió el mensaje");
	switch (package->operationCode){
		case SUBSCRIPTION:
			puts("Va a suscribir al cliente");
			Broker_Suscribe_Process(package->buffer, cliente, broker);
			break;
		case MESSAGE:
			Broker_Process_Message();
			break;
		case ACKNOWLEDGE:
			Broker_Get_Acknowledge();
			break;
	}
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


void Broker_initialize(t_Broker* broker){

	t_list* aux;
	aux= list_create();

	for (int i=0; i < 6; i++){
		t_queue_handler* queue_handler;
		queue_handler= queue_handler_initialize((message_type)i);
		list_add(aux,queue_handler);
	}

	broker->queues = aux;
}

void Broker_destroy(t_Broker* broker){
	list_destroy_and_destroy_elements(broker->queues,(void*)destroy_queue_handler);
	free(broker->cacheMemory);
	free(broker);
}

t_queue_handler* Broker_Get_Specific_Queue(t_Broker broker, message_type queueType ){

	int Match = 0;
	int index = 0;
	t_queue_handler* queue_handler = (t_queue_handler*)list_get(broker.queues,index);

	while((!Match) && (queue_handler != NULL) ){
		if (queue_handler->type == queueType)
			Match=1;
		else
		{
			index++;
			queue_handler = (t_queue_handler*)list_get(broker.queues,index);
		}
	}

	return queue_handler;
}


void Broker_Suscribe_Process(t_buffer* buffer, int cliente, t_Broker* broker){

	void* stream = malloc(sizeof(uint32_t));
	memcpy(stream, buffer->stream, buffer->bufferSize);

	uint32_t QueueType = (uint32_t) *(stream);

	t_queue_handler* queueToSuscribe = Broker_Get_Specific_Queue(*(broker), QueueType);

	if (queueToSuscribe != NULL){
		t_suscriptor* aux= (t_suscriptor*)malloc(sizeof(t_suscriptor));
		aux->suscripted = cliente;
		list_add(queueToSuscribe->suscriptors,aux);
	}

}

//TODO - implementar los métodos para procesar un acknowledge y un mensaje
void Broker_Get_Acknowledge(){

}

void Broker_Process_Message(){

}

//QUEUE LOGIC
int destroy_queue_list(t_list* self){
	list_destroy_and_destroy_elements(self,(void*)destroy_queue_handler);
	return 0;
}

t_queue_handler* queue_handler_initialize(message_type type){
	t_queue_handler* aux;
	aux = malloc(sizeof(t_queue_handler));
	aux->queue = queue_create();
	aux->suscriptors = list_create();
	aux->type=type;
	return aux;
}

void destroy_queue_handler(t_queue_handler* self){
	queue_destroy(self->queue);
	list_destroy(self->suscriptors);
}

t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* self,int pos){
	return list_get(self->suscriptors,pos);
}


