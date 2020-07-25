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
sem_t mutexSuscription;

int main(void) {

	t_log* logger;
	t_config* config;

	char* ip;
	char* puerto;
	int server,cliente;

	sem_init(&(mutexSuscription),0,1);
	signal(SIGINT,signaltHandler);
	signal(SIGUSR1,cacheSigHandler);

	pthread_t* thread;

	thread = (pthread_t*)malloc(sizeof(pthread_t));

	logger = iniciar_logger();

	start_cache(logger);
	//log_info(logger,"PROCESO BROKER ONLINE");

	config = leer_config();

	ip= get_config_value(config,logger,IP_BROKER);
	puerto = get_config_value(config,logger,PUERTO_BROKER);

	broker= (t_Broker*) malloc(sizeof(t_Broker));

	server = iniciar_servidor(ip, puerto);

	broker_initialize(broker, server, logger);

	//log_debug(logger,"SERVIDOR INICIADO");

	while (1){

		cliente = esperar_cliente(server);

		t_args* args= (t_args*) malloc (sizeof (t_args));

		args->cliente = &cliente;
		args->broker = broker;

		//log_debug(logger,"Va a atender un cliente");

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
	void* content;

	int resultado= RecievePackage(cliente,&type,&content);

	if (resultado == 0)
	{
		recive_message(type, cliente, broker, content);
	}
		//log_debug(broker->logger,"Resultado de envio del mensaje: %d", resultado);

	free(args);

}


void recive_message(uint32_t type, int cliente, t_Broker* broker, void* content ){

	//log_debug(broker->logger,"Tipo de mensaje recibido; %d", type);
	switch (type){
		case SUBSCRIPTION:
			//log_debug(broker->logger, "Suscripción");
			broker_suscribe_process(content, cliente, broker);
			break;
		case MESSAGE:
			//log_debug(broker->logger, "Mensaje");
			broker_process_message(content, cliente, broker);
			break;
		case ACKNOWLEDGE:
			broker_get_acknowledge(content, cliente, broker);
			break;
		default:
		{
			//log_info(broker->logger,"Tipo de mensaje invalido: %d", type);
		}
	}
	free(content);
}


t_log* iniciar_logger(void)
{
	return log_create("Broker.log","Broker",1,LOG_LEVEL_INFO);
}

t_config* leer_config(void)
{
	return config_create("/home/utnso/workspace/tp-2020-1c-MATE-OS/Broker/Broker.config");

}

char* get_config_value(t_config* config, t_log* logger, char* propiedad){

	if (config_has_property(config,propiedad)){
		//log_info(logger,config_get_string_value(config,propiedad));
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


void broker_suscribe_process(void* buffer, int cliente, t_Broker* broker)
{
	message_type* queueType = (message_type*)buffer;


	t_queue_handler* queueToSuscribe = broker_get_specific_Queue(*(broker), *queueType);

	if (queueToSuscribe != NULL){
		t_suscriptor* aux= (t_suscriptor*)malloc(sizeof(t_suscriptor));
		aux->suscripted = cliente;
		sem_wait(&(mutexSuscription));
		list_add(queueToSuscribe->suscriptors,aux);
		sem_post(&(mutexSuscription));
		log_info(broker->logger,"NUEVA SUSCRIPCIÓN AL BROKER QUEUE: %s", GetStringFromMessageType(*queueType));
		
		//GetAllMessagesFromQueue that were not sent to him:
		log_info(broker->logger, "Cliente: %i Queue: %i ", cliente, *queueType);
		t_list* messagesToSend = GetAllMessagesForSuscriptor(cliente,*queueType);

		if(!list_is_empty(messagesToSend))
		{
			pthread_t* thread;
			thread = (pthread_t*)malloc(sizeof(pthread_t));

			int index = 0;

			while(list_get(messagesToSend, index) != NULL)
			{
				deli_message* message = (deli_message*)list_get(messagesToSend, index);

				//log_debug(broker->logger, "Envio mensaje al suscriptor: %d", cliente);

				t_args_queue* args= (t_args_queue*) malloc (sizeof(t_args_queue));
				args->messageId = message->id;
				args->queue = queueToSuscribe;
				args->broker = broker;
				args->cliente = cliente;

				pthread_create(thread,NULL,(void*)queue_handler_send_message,args);
				pthread_detach(*thread);

				index++;
			}

		}

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

	//log_debug(broker->logger, "El ID del mensaje es %d", message->id);

	SendMessageAcknowledge(message->id, cliente);

	//TODO ver que hacer con el correlation ID, podríamos saber si es necesario hacer algo por el tipo de mensaje
	//en principio no hacer nada, ver en reunion

	t_queue_handler* queue = broker_get_specific_Queue(*broker,message->messageType);

	queue_handler_process_message(queue,message, broker);

}

//QUEUE LOGIC

void queue_handler_process_message(t_queue_handler* queue, deli_message* message, t_Broker* broker){

	save_message(*message);

	log_info(broker->logger,"NUEVO MENSAJE PARA LA QUEUE: %d", queue->type);

	pthread_t* thread;

	thread = (pthread_t*)malloc(sizeof(pthread_t));


	int index = 0;

	t_suscriptor* suscriptor;

	while(list_get(queue->suscriptors, index) != NULL)
	{
		suscriptor = (t_suscriptor*)list_get(queue->suscriptors, index);

		//log_debug(broker->logger, "Envio mensaje al suscriptor: %d", suscriptor->suscripted);

		t_args_queue* args= (t_args_queue*) malloc (sizeof(t_args_queue));
		args->messageId = message->id;
		args->queue = queue;
		args->broker = broker;
		args->cliente = suscriptor->suscripted;

		pthread_create(thread,NULL,(void*)queue_handler_send_message,args);
		pthread_detach(*thread);

		index++;
	}

}

void queue_handler_send_message(void* args){

	t_queue_handler* queue =((t_args_queue*)args)->queue;
	deli_message* message = GetMessage(((t_args_queue*)args)->messageId);
	int client = ((t_args_queue*)args)->cliente;


	int result = SendMessage(*message,client);
	if(result == -1)
	{
		RemoveClient(client);
		return;
	}

	log_info(broker->logger, "SE ENVIO EL MENSAJE %d AL CLIENTE %d", message->id, client);
	//log_debug(broker->logger, "RESULTADO DEL ENVIO: %d", result);

	struct pollfd pfds[1];
	pfds[0].fd = client;
	pfds[0].events = POLLIN | POLLHUP; // Tell me when ready to read
	int num_events = poll(pfds, 1, 1);

	//TODO re-intentar, quiza tambien checkquear que el cliente siga ahi

	AddASentSubscriberToMessage(message->id, client);

	if (!result)
	{
		//log_debug(broker->logger,"ESPERA EN ACKNOWLEDGE");
		uint32_t op_code;
		void* content;
		result = RecievePackage(client,&op_code,&content);
		if(result == -1)
		{
			RemoveClient(client);
			return;
		}
		log_info(broker->logger,"RECIBIDO ACKNOWLEDGE PARA MENSAGE %d", message->id);

		if (op_code == ACKNOWLEDGE )
		{
			if(*(int*)content == message->id)
			{
				AddAcknowledgeToMessage(message->id);
			}
			free((int*)content);
		}
		else if(op_code == MESSAGE)
		{
			//log_debug(broker->logger,"SE RECIBIO UN MESSAGE CUANDO SE ESPERABA UN ACKNOWLEDGE", result);
			Free_deli_message((deli_message*)content);
		}
		else
		{
			//log_debug(broker->logger,"SE RECIBIO UNA SOLICITUD DE SUSCRIPCION CUANDO SE ESPERABA UN ACKNOWLEDGE", result);
			free((message_type*)content);
		}
	}
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
	free(self);
}

t_suscriptor* queue_handler_get_suscriptor(t_queue_handler* self,int pos){
	return list_get(self->suscriptors,pos);
}

void RemoveClient(int client)
{
	bool is_disconnected_suscriber(t_suscriptor* suscriptor) { return suscriptor->suscripted == client; }

	void remove_disconnected_from_queues(t_queue_handler* queue_handler)
	{
		t_suscriptor* suscriptor = list_remove_by_condition(queue_handler->suscriptors, (void*)is_disconnected_suscriber);
		free(suscriptor);
	}

	list_iterate(broker->queues, (void*) remove_disconnected_from_queues);
	close(client);
}

// Manage an interupt or segmentation fault
void signaltHandler(int sig_num){
	//log_debug(broker->logger, "SE INTERRUMPIO EL PROCESO");
	broker_destroy(broker);
	exit(-5);
}

void cacheSigHandler(int sig_num)
{
	PrintDumpOfCache();
}


