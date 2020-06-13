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

int main(int argc, char **argv) {
	puts("GameBoy (Publicador)"); /* prints GameBoy (Publicador) */


	for(int i = 0; i < argc; i++)
	{
		printf("%s\n",argv[i]);
	}
	printf("-------\n");
	printf("%i\n",argc);
	puts("////////////\n\n");
	// TODO check if config file is exists and is valid



	if(argc < 2)
	{
		printf("Incorrect number of parameters for any function\n");
		return 1;
	}

	//Check if reciever is valid
	t_reciever reciver;
	message_type messageType;

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

	}else if(strcmp(argv[1],"GAMECARD")) {
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
	else if(strcmp(argv[1],"TEAM")) {
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
	}else if(strcmp(argv[1],"SUSCRIPTOR")) {
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

	/*char* pokemonName = argv[3];
	printf("%s \n",pokemonName);
	int numberOfIntegerArguments = argc-4;
	printf("%i \n",numberOfIntegerArguments);
	int* messageIntegerArguments = (int*)malloc(sizeof(int)*numberOfIntegerArguments);
	for(int i = 4, u = 0; u < numberOfIntegerArguments; i++, u++)
	{
		int argument = atoi(argv[i]);
		if(strcmp(argv[i], "0") != 0 && argument == 0)
		{
			printf("Invalid Input\n");
			return 1;
		}
		messageIntegerArguments[u] = argument;
	}*/


	//Get IP from config
	t_config* config = config_create("gameboy.config");

	initBroker(&broker);
	readConfigBrokerValues(config,&broker);

	/*char* brokerIp = config_get_string_value(config, "IP_BROKER");
	char* teamIp = config_get_string_value(config, "IP_TEAM");
	char* gameCardIp = config_get_string_value(config, "IP_GAMECARD");

	char* brokerPort = config_get_string_value(config, "PUERTO_BROKER");
	char* teamPort = config_get_string_value(config, "PUERTO_TEAM");
	char* gameCardPort = config_get_string_value(config, "PUERTO_GAMECARD");
*/

	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(broker.ip, broker.port, &hints, &server_info);

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	//Connect
	int result = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

	if(result == -1)
	{
		printf("Could not connect");
		return -1;
	}

	free(server_info);



	//////////////////////// ENVIAR ////////////////////////////////

	//TODO Create message as requested and send



	void* message = NULL;
	//For the message, try to create the message
	switch(reciver){
		case BROKER:{
				switch(messageType){
					case NEW_POKEMON:
						new_pokemon new = malloc(sizeof(new_pokemon));
						new.pokemonName = malloc(sizeof(argv[3]));
						new.pokemonName = argv[3];
						new.horizontalCoordinate = argv[4];
						new.verticalCoordinate = argv[5];;
						new.ammount = argv[6];
						message = &new;
						break;
					case APPEARED_POKEMON:
						appeared_pokemon app = malloc(sizeof(appeared_pokemon));
						app.pokemonName = malloc(sizeof(argv[3]));
						memcpy(app.pokemonName,argv[3]);
						app.horizontalCoordinate = argv[4];
						app.verticalCoordinate = argv[5];;
						//TODO ver con marcos por el correlation id
						//app.cid = argv[6];
						message = &app;
						break;
					case CATCH_POKEMON:
						catch_pokemon cat = malloc(sizeof(catch_pokemon));
						cat.pokemonName = malloc(sizeof(argv[3]));
						memcpy(cat.pokemonName,argv[3]);
						cat.horizontalCoordinate = argv[4];
						cat.verticalCoordinate = argv[5];;
						message = &cat;
						break;
					case CAUGHT_POKEMON:
						caught_pokemon cau = malloc(sizeof(caught_pokemon ));
						cau.pokemonName = malloc(sizeof(argv[3]));
						memcpy(cau.pokemonName,argv[3]);
						cau.horizontalCoordinate = argv[4];
						cau.verticalCoordinate = argv[5];;
						message = &cau;
						break;
			}
			default:
				printf("Message type not supported\n");
				return 1;
			}
	}


	/// Test Sending, TODO Remove
	Vector2 coordinates[] = {{1,2},{89,34},{75,13}};
	localized_pokemon localized = {"squirtle", 3, coordinates};
	Send_LOCALIZED(localized, server_socket);
	//SendSubscriptionRequest(APPEARED_POKEMON, server_socket);
	//SendMessageAcknowledge(319, server_socket);


	//////////////////////// RECIBIR ACKNOWLEDGE ///////////////////

	//TODO

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

void initBroker(struct Broker *broker){
	broker->ipKey="IP_BROKER";
	broker->portKey="PUERTO_BROKER";

}
