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
} t_reciever;

int main(int argc, char **argv) {
	puts("GameBoy (Publicador)"); /* prints GameBoy (Publicador) */

	/*
	for(int i = 0; i < argc; i++)
	{
		printf("%s\n",argv[i]);
	}
	printf("-------\n");
	printf("%i\n",argc);
	puts("////////////\n\n");
	// check if config file is exists and is valid
	*/

	if(argc < 3)
	{
		printf("Incorrect number of parameters for any function\n");
		return 1;
	}

	//Check if reciever is valid
	t_reciever reciver;

	if(strcmp(argv[1], "BROKER")) 		{ reciver = BROKER; }
	else if(strcmp(argv[1],"GAMECARD")) { reciver = GAMECARD; }
	else if(strcmp(argv[1],"GAMECARD")) { reciver = TEAM; }
	else
	{
		printf("Invalid message reciever\n");
		return 1;
	}

	//Check if message type is valid
	message_type messageType;

	if(strcmp(argv[2], "NEW_POKEMON") == 0) 				{ messageType = NEW_POKEMON; }
	else if(strcmp(argv[2], "LOCALIZED_POKEMON")) 	{ messageType = LOCALIZED_POKEMON; }
	else if(strcmp(argv[2], "GET_POKEMON")) 		{ messageType = GET_POKEMON; }
	else if(strcmp(argv[2], "APPEARED_POKEMON")) 	{ messageType = APPEARED_POKEMON; }
	else if(strcmp(argv[2], "CATCH_POKEMON")) 		{ messageType = CATCH_POKEMON; }
	else if(strcmp(argv[2], "CAUGHT_POKEMON")) 		{ messageType = CAUGHT_POKEMON; }
	else
	{
		printf("Message type is invalid or not yet supported\n");
		return 1;
	}

	char* pokemonName = argv[3];
	int numberOfIntegerArguments = argc-4;
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
	}

	/*
	for(int i = 0; i < numberOfArguments; i++)
	{
		printf("%i\n",messageIntegerArguments[i]);
	}
	return 0;
	 */

	void* message = NULL;

	//For the message, try to create the message
	switch(messageType)
	{
		case NEW_POKEMON:
		{
			if(numberOfIntegerArguments < 3 || numberOfIntegerArguments > 3)
			{
				printf("Incorrect number of arguments for new pokemon\n");
				return 1;
			}
			new_pokemon new = { "", 0, 0, 0};
			new.pokemonName = pokemonName;
			new.horizontalCoordinate = messageIntegerArguments[0];
			new.verticalCoordinate = messageIntegerArguments[1];
			new.ammount = messageIntegerArguments[2];
			message = &new;
			break;
		}
		default:
			printf("Message type not supported\n");
			return 1;
	}

	//Get IP from config
	t_config* config = config_create("gameboy.config");

	char* brokerIp = config_get_string_value(config, "IP_BROKER");
	char* teamIp = config_get_string_value(config, "IP_TEAM");
	char* gameCardIp = config_get_string_value(config, "IP_GAMECARD");

	char* brokerPort = config_get_string_value(config, "PUERTO_BROKER");
	char* teamPort = config_get_string_value(config, "PUERTO_TEAM");
	char* gameCardPort = config_get_string_value(config, "PUERTO_GAMECARD");


	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(brokerIp, brokerPort, &hints, &server_info);

	int server_socket = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	//Connect
	int result = connect(server_socket, server_info->ai_addr, server_info->ai_addrlen);

	if(result == -1)
	{
		Printf("Could not connect");
		return -1;
	}

	free(server_info);

	//////////////////////// ENVIAR ////////////////////////////////

	char* mensaje = "Hello";
	int socket_cliente = server_socket;

	t_buffer* buffer = (t_buffer*) malloc(sizeof(t_buffer));
	buffer->bufferSize = strlen(mensaje) + 1;
	buffer->stream = mensaje;

	t_package* paquete = (t_package*) malloc(sizeof(t_package));

	paquete->operationCode = MESSAGE;
	paquete->buffer = buffer;

	void* aEnviar = malloc(buffer->bufferSize + sizeof(paquete->operationCode) + sizeof(int));

	int offset = 0;
	memcpy(aEnviar + offset, &(paquete->operationCode), sizeof(paquete->operationCode));
	offset += sizeof(paquete->operationCode);
	memcpy(aEnviar + offset, &(paquete->buffer->bufferSize), sizeof(int));
	offset += sizeof(int);
	memcpy(aEnviar + offset, paquete->buffer->stream, paquete->buffer->bufferSize);

	send(socket_cliente, aEnviar, buffer->bufferSize +  sizeof(int) + sizeof(paquete->operationCode), 0);
	printf("Mensaje Enviado\n");

	free(aEnviar);
	free(paquete->buffer);
	free(paquete);

	//////////////////////// RECIBIR ////////////////////////////////

	op_code operacion;
	recv(socket_cliente, &operacion, sizeof(operacion), 0);
	int messageSize;
	recv(socket_cliente, &messageSize, sizeof(messageSize), 0);
	char* message2 = malloc(messageSize);
	recv(socket_cliente, message2, messageSize, 0);

	printf("%s\n",message2);

	//////////////////////// END ////////////////////////////////

	//Send message

	//Wait for response

	//On any fail,Inform it and terminate the program

	//On succes, show the acknowledge and terminate the program


	return EXIT_SUCCESS;
}
