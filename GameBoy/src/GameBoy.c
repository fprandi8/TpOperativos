/*
 ============================================================================
 Name        : GameBoy.c
 Author      : Mauro
 Version     :
 Copyright   : Your copyright notice
 Description : GameBoy process
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <delibird/comms/messages.h>
#include <delibird/comms/pokeio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include <pthread.h>
#include <poll.h>
#include <unistd.h>

typedef enum
{
	BROKER = 1,
	GAMECARD = 2,
	TEAM = 3,
	SUSCRIPTOR = 4
} t_reciever;

struct Broker
{
	char* ipKey;
	char* ip;
	char* portKey;
	char* port;
} broker;

void initBroker(struct Broker*);
void readConfigBrokerValues(t_config*,struct Broker*);
void SleepAndClose(void* args);
void GetKeysFor(t_reciever reciever, char* keys[]);
void ReadConfigValues(t_config *config, char* keys[]);
void RecieveAcknowledge(int server_socket);
void RecieveMessage(int server_socket, message_type expectedType);

int main(int argc, char **argv) {
	puts("GameBoy (Publicador)\n");

/*
	for(int i = 0; i < argc; i++)
	{
		printf("%s\n",argv[i]);
	}
	printf("-------\n");
	printf("%i\n",argc);
	puts("////////////\n\n");*/
	// TODO check if config file is exists and is valid



	if(argc < 2)
	{
		printf("Incorrect number of parameters for any function\n");
		return 1;
	}

	//Check if reciever is valid
	t_reciever reciver;
	message_type messageType;
	int subscriptionTime = 0; //Time used to control suscription disconnection

	if(strcmp(argv[1], "BROKER") == 0){
		reciver = BROKER;
		if(strcmp(argv[2], "NEW_POKEMON") == 0){
			if(argc == 7){
				messageType = NEW_POKEMON;
			}else{
				printf("Incorrect number of parameters for function BROKER - NEW POKEMON, must be 6\n");
				return 1;
			}
		}else if(strcmp(argv[2], "APPEARED_POKEMON") == 0){
			if(argc == 7){
				messageType = APPEARED_POKEMON;
			}else{
				printf("Incorrect number of parameters for function BROKER - APPEARED_POKEMON, must be 6\n");
				return 1;
			}
		}else if(strcmp(argv[2], "CATCH_POKEMON") == 0){
			if(argc == 6){
				messageType = CATCH_POKEMON;
			}else{
				printf("Incorrect number of parameters for function BROKER - CATCH_POKEMON, must be 5\n");
				return 1;
			}
		}else if(strcmp(argv[2], "CAUGHT_POKEMON") == 0){
			if(argc == 5){
				messageType = CAUGHT_POKEMON;
			}else{
				printf("Incorrect number of parameters for function BROKER - CAUGHT_POKEMON, must be 4\n");
				return 1;
			}
		}else if(strcmp(argv[2], "GET_POKEMON") == 0){
			if(argc == 4){
				messageType = GET_POKEMON;
			}else{
				printf("Incorrect number of parameters for function BROKER - GET_POKEMON, must be 3\n");
				return 1;
			}
		}else{

			printf("Incorrect message, for Broker must be one of: GET_POKEMON, CAUGHT_POKEMON, CATCH_POKEMON, NEW POKEMON, APPEARED_POKEMON \n");
			return 1;
		}

	}else if(strcmp(argv[1],"GAMECARD") == 0) {
		reciver = GAMECARD;
		if(strcmp(argv[2], "NEW_POKEMON") == 0){
					if(argc == 8){
						messageType = NEW_POKEMON;
					}else{
						printf("Incorrect number of parameters for function GAMECARD - NEW POKEMON, must be 7\n");
						return 1;
					}
				}else if(strcmp(argv[2], "CATCH_POKEMON") == 0){
					if(argc == 7){
						messageType = CATCH_POKEMON;
					}else{
						printf("Incorrect number of parameters for function GAMECARD - CATCH_POKEMON, must be 6\n");
						return 1;
					}
				}else if(strcmp(argv[2], "GET_POKEMON") == 0){
					if(argc == 5){
						messageType = GET_POKEMON;
					}else{
						printf("Incorrect number of parameters for function GAMECARD - GET_POKEMON, must be 4\n");
						return 1;
					}
				}else{

					printf("Incorrect message, for GAMECARD must be one of: GET_POKEMON, CATCH_POKEMON, NEW POKEMON \n");
					return 1;
				}
	}
	else if(strcmp(argv[1],"TEAM") == 0) {
		reciver = TEAM;
		if(strcmp(argv[2], "APPEARED_POKEMON") == 0){
			if(argc == 6){
				messageType = APPEARED_POKEMON;
			}else{
				printf("Incorrect number of parameters for function TEAM - APPEARED_POKEMON, must be 5\n");
				return 1;
			}
		}else{

			printf("Incorrect message, for TEAM must be APPEARED_POKEMON \n");
			return 1;
		}
	}else if(strcmp(argv[1],"SUSCRIPTOR") == 0) {
		reciver = SUSCRIPTOR;
		if(argc == 4){
			if(strcmp(argv[2], "NEW_POKEMON") == 0){
				messageType = NEW_POKEMON;
			}else if(strcmp(argv[2], "LOCALIZED_POKEMON") == 0){
				messageType = LOCALIZED_POKEMON;
			}else if(strcmp(argv[2], "GET_POKEMON") == 0){
				messageType = GET_POKEMON;
			}else if(strcmp(argv[2], "APPEARED_POKEMON") == 0){
				messageType = APPEARED_POKEMON;
			}else if(strcmp(argv[2], "CATCH_POKEMON") == 0){
				messageType = CATCH_POKEMON;
			}else if(strcmp(argv[2], "CAUGHT_POKEMON") == 0){
				messageType = CAUGHT_POKEMON;
			}else{
				printf("Incorrect message, for SUSCRIPTOR must be GET_POKEMON, CAUGHT_POKEMON, CATCH_POKEMON, NEW POKEMON, APPEARED_POKEMON, LOCALIZED_POKEMON \n");
				return 1;
			}
			char* ptr;
			subscriptionTime = strtol(argv[3], &ptr, 10);
			printf("%d\n",subscriptionTime);
		}else{

			printf("Incorrect number of parameters for function SUSCRIPTOR, must be 3\n");
			return 1;
		}
	}
	else
	{
		printf("Invalid message reciever, must be one of: BROKER, TEAM, GAMECARD, SUSCRIPTOR \n");
		return 1;
	}

	//Get IP from config
	t_config* config = config_create("gameboy.config");

	char* keys[2];
	GetKeysFor(reciver, keys);
	ReadConfigValues(config, keys);

	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(keys[0], keys[1], &hints, &server_info);

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	int result = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

	if(result == -1)
	{
		printf("Could not connect\n");
		return -1;
	}

	free(server_info);

	printf("Connected\n");

	//////////////////////// ENVIAR ////////////////////////////////

	if(reciver == SUSCRIPTOR)
	{
		SendSubscriptionRequest(messageType, server_socket);

		pthread_t* thread;
		thread = (pthread_t*)malloc(sizeof(pthread_t));

		int args[] = {server_socket, subscriptionTime};

		pthread_create(thread,NULL,(void*)SleepAndClose,args);
		pthread_detach(*thread);

		int isRunning  = 1;

		struct pollfd pfds[1]; // More if you want to monitor more

		pfds[0].fd = server_socket;
		pfds[0].events = POLLIN | POLLHUP; // Tell me when ready to read

		while(isRunning)
		{
			 int num_events = poll(pfds, 1, subscriptionTime);
			 if (num_events == 0)
			 {
			    continue;
			 }
			 else
			 {
				 int pollin_happened = pfds[0].revents & POLLIN;

				 if (pollin_happened)
				 {
					 RecieveMessage(server_socket, messageType);
				 }
				 else
				 {
					 printf("Exit after %d seconds", subscriptionTime);
					 return EXIT_SUCCESS;
				 }
			 }
		}


	}


	deli_message deliMessage;
	deliMessage.messageType = messageType;
	void* message = NULL;
	char* ptr_strtol; //pinter to char ised for strtol

	switch(reciver){
		case BROKER:
		{
				switch(messageType){
					case NEW_POKEMON:
					{
						new_pokemon new;
						new.pokemonName = argv[3];
						new.horizontalCoordinate = strtol(argv[4], &ptr_strtol, 10);
						new.verticalCoordinate = strtol(argv[5], &ptr_strtol, 10);;
						new.ammount = strtol(argv[6], &ptr_strtol, 10);;
						message = &new;
						deliMessage.messageContent = message;
						deliMessage.id = 0;
						deliMessage.correlationId = 0;
						break;
					}
					case APPEARED_POKEMON:
					{
						appeared_pokemon app;
						app.pokemonName=argv[3];
						app.horizontalCoordinate = strtol(argv[4], &ptr_strtol, 10);;
						app.verticalCoordinate = strtol(argv[5], &ptr_strtol, 10);;
						message = &app;
						deliMessage.messageContent = message;
						deliMessage.id = 0;
						deliMessage.correlationId = strtol(argv[6], &ptr_strtol, 10);;
						break;
					}
					case CATCH_POKEMON:
					{
						catch_pokemon cat;
						cat.pokemonName = argv[3];
						cat.horizontalCoordinate = strtol(argv[4], &ptr_strtol, 10);;
						cat.verticalCoordinate = strtol(argv[5], &ptr_strtol, 10);;
						message = &cat;
						deliMessage.messageContent = message;
						deliMessage.id = 0;
						deliMessage.correlationId = 0;
						break;
					}
					case CAUGHT_POKEMON:
					{
						caught_pokemon cau;
						cau.caught = strtol(argv[4], &ptr_strtol, 10);;
						message = &cau;
						deliMessage.messageContent = message;
						deliMessage.id = 0;
						deliMessage.correlationId = strtol(argv[3], &ptr_strtol, 10);;
						break;
					}
					case GET_POKEMON:
					{
						get_pokemon get;
						get.pokemonName = argv[3];
						message = &get;
						deliMessage.messageContent = message;
						deliMessage.id = 0;
						deliMessage.correlationId = 0;
						break;
					}
					default:
					{
						return -1;
					}
				}
				break;
	}
			case TEAM:
			{
				switch(messageType){
					case APPEARED_POKEMON:
					{
						appeared_pokemon app;
						app.pokemonName = argv[3];
						app.horizontalCoordinate = strtol(argv[4], &ptr_strtol, 10);;
						app.verticalCoordinate = strtol(argv[5], &ptr_strtol, 10);;
						message = &app;
						deliMessage.messageContent = message;
						deliMessage.id = 0;
						deliMessage.correlationId = 0;
						break;
					}
				default:
				{
					return -1;
				}
				}
				break;
			}
			case GAMECARD:
			{
				switch(messageType){
					case NEW_POKEMON:
					{
						new_pokemon new;
						new.pokemonName = argv[3];
						new.horizontalCoordinate = strtol(argv[4], &ptr_strtol, 10);;
						new.verticalCoordinate = strtol(argv[5], &ptr_strtol, 10);;
						new.ammount = strtol(argv[6], &ptr_strtol, 10);;
						message = &new;
						deliMessage.messageContent = message;
						deliMessage.id = strtol(argv[7], &ptr_strtol, 10);
						deliMessage.correlationId = 0;
						break;
					}
					case CATCH_POKEMON:
					{
						catch_pokemon cat;
						cat.pokemonName = argv[3];
						cat.horizontalCoordinate = strtol(argv[4], &ptr_strtol, 10);;
						cat.verticalCoordinate = strtol(argv[5], &ptr_strtol, 10);;
						message = &cat;
						deliMessage.messageContent = message;
						deliMessage.id = strtol(argv[6], &ptr_strtol, 10);;
						deliMessage.correlationId = 0;
						break;
					}
					case GET_POKEMON:
					{
						get_pokemon get;
						get.pokemonName = argv[3];
						message = &get;
						deliMessage.messageContent = message;
						deliMessage.id = strtol(argv[4], &ptr_strtol, 10);;
						deliMessage.correlationId = 0;
						break;
					}
					default:
					{
						return -1;
					}
				}
				break;
			}
			default:
				printf("Message type not supported\n");
				return -1;
			}




	SendMessage(deliMessage,server_socket);
	/// Test Sending, TODO Remove
	//Vector2 coordinates[] = {{1,2},{89,34},{75,13}};
	//localized_pokemon localized = {"squirtle", 3, coordinates};
	//Send_LOCALIZED(localized, server_socket);
	//SendSubscriptionRequest(APPEARED_POKEMON, server_socket);
	//SendMessageAcknowledge(319, server_socket);


	//////////////////////// RECIBIR ACKNOWLEDGE ///////////////////
	printf("Waiting for messaje acknowledge\n");
	RecieveAcknowledge(server_socket);

	//////////////////////// END ////////////////////////////////

	close(server_socket);


	return EXIT_SUCCESS;
}

void readConfigBrokerValues(t_config *config,struct Broker *broker){
	printf("2. Comienza lectura de config de broker\n");
	if (config_has_property(config,broker->ipKey)){
		broker->ip=config_get_string_value(config,broker->ipKey);
		printf("2. Se leyó la IP: %s\n",broker->ip);
	}else{
		exit(-3);
	}

	if (config_has_property(config,broker->portKey)){
		broker->port=config_get_string_value(config,broker->portKey);
		printf("2. Se leyó el puerto: %s\n",broker->port);
	}else{
		exit(-3);
	}
	printf("2. Finaliza lectura de config de broker\n");
}

void ReadConfigValues(t_config *config, char* keys[])
{
	printf("2. Comienza lectura de config\n");
	if (config_has_property(config,keys[0])){
		keys[0] = config_get_string_value(config,keys[0]);
		printf("2. Se leyó la IP: %s\n", keys[0]);
	}else{
		exit(-3);
	}

	if (config_has_property(config,keys[1])){
		keys[1] = config_get_string_value(config,keys[1]);
		printf("2. Se leyó el puerto: %s\n", keys[1]);
	}else{
		exit(-3);
	}
	printf("2. Finaliza lectura de config\n\n");
}

void GetKeysFor(t_reciever reciever, char* keys[])
{
	if(reciever == SUSCRIPTOR || reciever == BROKER)
	{
		keys[0] = "IP_BROKER";
		keys[1] = "PUERTO_BROKER";
	}
	else if(reciever == TEAM)
	{
		keys[0] = "IP_TEAM";
		keys[1] = "PUERTO_TEAM";
	}
	else if(reciever == GAMECARD)
	{
		keys[0] = "IP_GAMECARD";
		keys[1] = "PUERTO_GAMECARD";
	}
	else
	{
		keys[0] = "";
		keys[1] = "";
	}
}

void RecieveAcknowledge(int server_socket)
{
	op_code operationCode;
	void* content;
	if(RecievePackage(server_socket, &operationCode, &content) == 0)
	{
		switch (operationCode){
			case SUBSCRIPTION:
				puts("Error, recieved a suscription request while waiting for an acknowledge");
				break;
			case MESSAGE:
				puts("Error, recieved a message while waiting for an acknowledge");
				break;
			case ACKNOWLEDGE:
				puts("Recieved Acknowledge");
				break;
			default:
				puts("Error, recieved a packet of unkown type");
				break;
		}
	} 
	else 
	{
		puts("Error, failed to recieve packet");
	}
}

void RecieveMessage(int server_socket, message_type expectedType)
{
	op_code operationCode;
	void* content;
	if(RecievePackage(server_socket, &operationCode, &content) == 0)
	{
		switch (operationCode){
			case SUBSCRIPTION:
				puts("Error, recieved a suscription request while waiting for a message");
				break;
			case MESSAGE:
				//TODO log message, give logger to function
				if(((deli_message*)content)->messageType == expectedType) puts("Recieved Message of correct type"); else puts("Recieved Message with an incorrect type");
				break;
			case ACKNOWLEDGE:
				puts("Error, recieved a suscription request while waiting for a message");
				break;
			default:
				puts("Error, recieved a packet of unkown type");
				break;
		}
	} 
	else 
	{
		puts("Error, failed to recieve packet");
	}
}


void SleepAndClose(void* args)
{
	sleep(((int*)args)[1]);
	close(((int*)args)[0]);
}

void initBroker(struct Broker *broker){
	broker->ipKey="IP_BROKER";
	broker->portKey="PUERTO_BROKER";

}
