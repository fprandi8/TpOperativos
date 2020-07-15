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


uint32_t ID = 0;
t_Broker* broker;

int main(void) {

	start_cache();
	t_log* logger;
	t_config* config;


	char* ip;
	char* puerto;
	int server,cliente;
//	char* tam_men, tam_min_part, algo_mem, algo_reem, algo_part, frec_comp;

	signal(SIGINT,signaltHandler);

	pthread_t* thread;

	thread = (pthread_t*)malloc(sizeof(pthread_t));

	logger = iniciar_logger();
	log_info(logger,"PROCESO BROKER ONLINE");

	config = leer_config();

	ip= get_config_value(config,logger,IP_BROKER);
	puerto = get_config_value(config,logger,PUERTO_BROKER);
//	tam_men = obtener_valor_config(config,logger,TAMANO_MEMORIA);
//	tam_min_part =(char*) obtener_valor_config(config,logger,TAMANO_MINIMO_PARTICION);
//	algo_mem=  obtener_valor_config(config,logger,ALGORITMO_MEMORIA);
//	algo_reem=  obtener_valor_config(config,logger,ALGORITMO_REEMPLAZO);
//	algo_part=  obtener_valor_config(config,logger,ALGORITMO_PARTICION_LIBRE);
//	frec_comp=  obtener_valor_config(config,logger,FRECUENCIA_COMPACTACION);

	broker= (t_Broker*) malloc(sizeof(t_Broker));

	server = iniciar_servidor(ip, puerto);

	broker_initialize(broker, server, logger);

	log_debug(logger,"SERVIDOR INICIADO");

	while (1){

		cliente = esperar_cliente(server);

		t_args* args= (t_args*) malloc (sizeof (t_args));

		args->cliente = &cliente;
		args->broker = broker;

		log_debug(logger,"Va a atender un cliente");

		pthread_create(thread,NULL,(void*)serve_client,args);
		pthread_detach(*thread);

	}

	broker_destroy(broker);

	return EXIT_SUCCESS;
}



void serve_client(void* variables)
{
	t_args* args= (t_args*)variables;
	int cliente = *((t_args*)variables)->cliente;
	t_Broker* broker = ((t_args*)variables)->broker;
	log_info(broker->logger,"NUEVO PROCESO CONECTADO CLIENTE %d", cliente);

	uint32_t type;
	void* content = malloc(sizeof(void*));

	int resultado= RecievePackage(cliente,&type,&content);

	if (!resultado)
		recive_message(type, cliente, broker, content);
	else
		log_debug(broker->logger,"Resultado de envio del mensaje: %d", resultado);

	free(args);

}


void recive_message(uint32_t type, int cliente, t_Broker* broker, void* content ){

	log_debug(broker->logger,"Tipo de mensaje recibido; %d", type);
	switch (type){
		case SUBSCRIPTION:
			log_debug(broker->logger, "Suscripción");
			broker_suscribe_process(content, cliente, broker);
			break;
		case MESSAGE:
			log_debug(broker->logger, "Mensaje");
			broker_process_message(content, cliente, broker);
			break;
		case ACKNOWLEDGE:
			broker_get_acknowledge(content, cliente, broker);
			break;
		default:
		{
			log_info(broker->logger,"Tipo de mensaje invalido: %d", type);
		}
	}
	free(content);
}


t_log* iniciar_logger(void)
{
	return log_create("Broker.log","Broker",1,LOG_LEVEL_DEBUG);
}

t_config* leer_config(void)
{
	return config_create("/home/utnso/workspace/tp-2020-1c-MATE-OS/Broker/Broker.config");

}

char* get_config_value(t_config* config, t_log* logger, char* propiedad){

	if (config_has_property(config,propiedad)){
		log_info(logger,config_get_string_value(config,propiedad));
		return config_get_string_value(config, propiedad);
	}
	return NULL;
}


void broker_initialize(t_Broker* broker, int server, t_log* logger){

	t_list* aux;
	aux= list_create();

	for (int i=1; i < 7; i++){
		t_queue_handler* queue_handler;
		queue_handler= queue_handler_initialize((message_type)i);
		list_add(aux,queue_handler);
	}

	broker->queues = aux;
	broker->logger = logger;
	sem_init(&(broker->semaphoreID),0,1);
	broker->servidor = server;
}

void broker_destroy(t_Broker* broker){
	list_destroy_and_destroy_elements(broker->queues,(void*)destroy_queue_handler);
	free(broker->cacheMemory);
	free(broker);
}

t_queue_handler* broker_get_specific_Queue(t_Broker broker, message_type queueType ){

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


void broker_suscribe_process(void* buffer, int cliente, t_Broker* broker){

	message_type* QueueType = (message_type*)buffer;

	t_queue_handler* queueToSuscribe = broker_get_specific_Queue(*(broker), *QueueType);

	if (queueToSuscribe != NULL){
		t_suscriptor* aux= (t_suscriptor*)malloc(sizeof(t_suscriptor));
		aux->suscripted = cliente;
		list_add(queueToSuscribe->suscriptors,aux);
		log_info(broker->logger,"NUEVA SUSCRIPCIÓN AL BROKER QUEUE: %d", *QueueType);
		//TODO: agregar logica para enviar los mensajes cacheados al subscriptor - generar hilos para enviar cada mensaje al nuevo suscriptor (con todo lo que conlleva)
	}

}

void broker_get_acknowledge(void* buffer, int cliente, t_Broker* broker){

}

void broker_assign_id(t_Broker* broker, deli_message* message){
	sem_wait(&(broker->semaphoreID));
	ID++;
	message->id = ID;
	sem_post(&(broker->semaphoreID));

}

void broker_process_message(void* buffer, int cliente, t_Broker* broker){

	deli_message* message = (deli_message*)buffer;

	broker_assign_id(broker, message);

	log_debug(broker->logger, "El ID del mensaje es %d", message->id);

	//TODO ver que hacer con el correlation ID, podríamos saber si es necesario hacer algo por el tipo de mensaje
	//en principio no hacer nada, ver en reunion

	t_queue_handler* queue = broker_get_specific_Queue(*broker,message->messageType);

	queue_handler_process_message(queue,message, broker);

}

//QUEUE LOGIC

void queue_handler_process_message(t_queue_handler* queue, deli_message* message, t_Broker* broker){

	//TODO lógica de cada mensaje, guardar en cache
	save_message(message);

	//TODO estructura para administrar mensajes, a quienes se enviaron si fueron recibidos


	log_info(broker->logger,"NUEVO MENSAJE PARA LA QUEUE: %d", queue->type);

	pthread_t* thread;

	thread = (pthread_t*)malloc(sizeof(pthread_t));

	//TODO La cache tiene que guardar el mensaje
	//queue_push(queue->queue,message->messageContent);


	t_message_administrator* messageAdmnistrator = message_administrator_initialize(message->id);

	list_add(queue->messagesAdministrator,messageAdmnistrator);

	t_args_queue* args= (t_args_queue*) malloc (sizeof(t_args_queue));
	args->messageAdministrator = (t_message_administrator*) malloc(sizeof(t_message_administrator));
	args->message =message;
	args->queue = queue;
	args->broker = broker;

	int index = 0;

	t_suscriptor* suscriptor = (t_suscriptor*) malloc (sizeof(t_suscriptor));

	while(list_get(queue->suscriptors, index) != NULL){
		suscriptor=(t_suscriptor*)list_get(queue->suscriptors, index);
		log_debug(broker->logger, "Envio mensaje al suscriptor: %d", suscriptor->suscripted);
		t_message_administrator* messageAdministrator = messege_administrator_get_administrator(queue->messagesAdministrator, message->id);
		message_administrator_pending_acknowledge(messageAdministrator);

		args->cliente = suscriptor->suscripted;
		args->messageAdministrator = messageAdmnistrator;

		pthread_create(thread,NULL,(void*)queue_handler_send_message,args);
		pthread_detach(*thread);

		index ++;
	}
	free(suscriptor);

}

void queue_handler_send_message(void* args){

	t_queue_handler* queue =((t_args_queue*)args)->queue;
	deli_message* message= ((t_args_queue*)args)->message;
	t_message_administrator* messageAdministrator = ((t_args_queue*)args)->messageAdministrator;
	int client = ((t_args_queue*)args)->cliente;

	switch (message->messageType) {

		case NEW_POKEMON:

			puts("new pokemon");
			break;

		case LOCALIZED_POKEMON:
			puts("localized pokemon");
			break;

		case GET_POKEMON:
			puts("get pokemon");
			break;

		case APPEARED_POKEMON:
			puts("appeared pokemon");
			break;

		case CATCH_POKEMON:
			puts("catch pokemon");
			break;

		case CAUGHT_POKEMON:
			puts("caught pokemon");
			break;
	}

	int result = SendMessage(*message,client);
	log_debug(broker->logger, "SE ENVIO EL MENSAJE AL CLIENTE %d", client);
	log_debug(broker->logger, "RESULTADO DEL ENVIO: %d", result);
	uint32_t op_code;
	void* content =malloc(sizeof(void*));

	if (!result){
		log_debug(broker->logger,"ESPERA EN ACKNOWLEDGE");
		result = RecievePackage(client,&op_code,&content);
		log_debug(broker->logger,"RESULTADO DEL ACKNOWLEDGE: %d", result);
		if (op_code == ACKNOWLEDGE ){
			message_administrator_receive_acknowledge(messageAdministrator);
			if (messageAdministrator->amountPendingAcknowledge == 0){
				//TODO: logica de la cache para eliminar el mensaje
				log_debug(broker->logger,"TODOS LOS SUSCRIPTORES RECIBIERON EL MENSAJE; SE ELIMINA");
				//void* element = queue_pop(queue->queue);
				//free(element);
			}
		}
	}

	free(content);
	free(messageAdministrator);
	free(args);
}

int destroy_queue_list(t_list* self){
	list_destroy_and_destroy_elements(self,(void*)destroy_queue_handler);
	return 0;
}

t_queue_handler* queue_handler_initialize(message_type type){
	t_queue_handler* aux = (t_queue_handler*)malloc(sizeof(t_queue_handler));
	aux->queue = queue_create();
	aux->suscriptors = list_create();
	aux->type=type;
	aux->messagesAdministrator = list_create();
	return aux;
}

void destroy_queue_handler(t_queue_handler* self){
	queue_destroy(self->queue);
	list_destroy(self->suscriptors);
	list_destroy(self->messagesAdministrator);
}

t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* self,int pos){
	return list_get(self->suscriptors,pos);
}

// Message admnistrator logic
t_message_administrator* message_administrator_initialize(uint32_t id)
{
	t_message_administrator* aux =(t_message_administrator*) malloc(sizeof(t_message_administrator));

	aux->messageId = id;
	sem_init(&(aux->semaphoreACK),0,1);
	aux->amountPendingAcknowledge = 0;

	return aux;
}

t_message_administrator* messege_administrator_get_administrator(t_list* list, uint32_t id){
	int index = 0;
	int match = 0 ;

	t_message_administrator* aux = (t_message_administrator*)malloc(sizeof(t_message_administrator));

	while((list_get(list, index) != NULL)&&(!match)){

		aux = (t_message_administrator*)list_get(list,index);

		if (aux->messageId == id)
			match = 1;
		index ++;
	}

	return aux;
}

void message_administrator_pending_acknowledge(t_message_administrator* self){
	sem_wait(&(self->semaphoreACK));
	self->amountPendingAcknowledge++;
	sem_post(&(self->semaphoreACK));

}

void message_administrator_receive_acknowledge(t_message_administrator* self){
	sem_wait(&(self->semaphoreACK));
	self->amountPendingAcknowledge--;
	sem_post(&(self->semaphoreACK));

}

uint32_t message_administrator_get_amountACK(t_message_administrator* self){
	return self->amountPendingAcknowledge;
}

//int message_administrator_compare(t_message_administrator* am ){
//
//}

// Manage an interupt or segmentation fault
void signaltHandler(int sig_num){
	log_debug(broker->logger, "SE INTERRUMPIO EL PROCESO");
	broker_destroy(broker);
	exit(-5);
}


