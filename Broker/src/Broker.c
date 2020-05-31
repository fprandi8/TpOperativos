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

int main(void) {

	t_log* logger;
	t_config* config;


	char* ip;
	char* puerto;
	int servidor,cliente;
//	char* tam_men, tam_min_part, algo_mem, algo_reem, algo_part, frec_comp;
//	char* nombre;

	pthread_t* thread;

	thread = (pthread_t*)malloc(sizeof(pthread_t));

	//nombre = (char*)malloc(strlen("mutex_id")+1);

	//attach to semaphore, if not exist will be created
	//mutex_id = sem_open(nombre, O_CREAT, S_IRWXU, 1);

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

		args->cliente = &cliente;
		args->broker = broker;

		printf("Crea un hilo para el cliente %d \n", cliente);

		pthread_create(thread,NULL,(void*)serve_client,args);
		pthread_detach(*thread);

	}

	Broker_destroy(broker);

	return EXIT_SUCCESS;
}



void serve_client(void* variables)
{
	char* msg = "CONECTADO \n";
	t_args* args= (t_args*)variables;
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

	free(args);

}


void recive_message(t_package* package, int cliente, t_Broker* broker){

	puts("recibió el mensaje");
	switch (package->operationCode){
		case SUBSCRIPTION:
			puts("Va a suscribir al cliente");
			Broker_Suscribe_Process(package->buffer, cliente, broker);
			break;
		case MESSAGE:
			Broker_Process_Message(package->buffer, cliente, broker);
			break;
		case ACKNOWLEDGE:
			Broker_Get_Acknowledge(package->buffer, cliente, broker);
			break;
	}
	free(package);
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

	uint32_t* stream = (uint32_t*)malloc(sizeof(uint32_t));
	memcpy(stream, buffer->stream, buffer->bufferSize);


	uint32_t QueueType = *(stream);

	printf("Cola a la que se quiere suscribir %d \n", QueueType );


	t_queue_handler* queueToSuscribe = Broker_Get_Specific_Queue(*(broker), QueueType);

	if (queueToSuscribe != NULL){
		t_suscriptor* aux= (t_suscriptor*)malloc(sizeof(t_suscriptor));
		aux->suscripted = cliente;
		list_add(queueToSuscribe->suscriptors,aux);
	}

}

//TODO - implementar los métodos para procesar un acknowledge y un mensaje
void Broker_Get_Acknowledge(t_buffer* buffer, int cliente, t_Broker* broker){

}

void Broker_Process_Message(t_buffer* buffer, int cliente, t_Broker* broker){

	deli_message* message;

	t_message* recievedMessage = DeserializeMessage(buffer->stream);
	message = (deli_message*)malloc(sizeof(recievedMessage));
	message->id = recievedMessage->id;

	//TODO logica de mutex para la asignacion de ID
	//el broker puede tener el semáforo en su estructura y podemos hacer
	// sem_wait(broker->semaforo)
	// ID ++;
	// message->id = ID;
	// sem_post(broker->semaforo);
	// Broker_Assign_id(broker, *message);
	ID++;
	message->id = ID;

	//TODO ver que hacer con el correlation ID, podríamos saber si es necesario hacer algo por el tipo de mensaje
	message->correlationId = (uint32_t)malloc(sizeof(uint32_t));
	message->correlationId = recievedMessage->correlationId;

	message->messageType = (uint32_t)malloc(sizeof(uint32_t));
	message->messageType = recievedMessage->messageType;

	message->messageContent = malloc(sizeof(void*));
	message->messageContent = DeserializeMessageContent(recievedMessage->messageType, recievedMessage->messageBuffer->stream);

	t_queue_handler* queue = Broker_Get_Specific_Queue(*broker,message->messageType);

	queue_handler_process_message(queue,message);

}

//QUEUE LOGIC

void queue_handler_process_message(t_queue_handler* queue, deli_message* message){

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

}


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


