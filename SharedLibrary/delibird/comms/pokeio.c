#include "pokeio.h"

void SendMessageAcknowledge(int messageId, int client_socket)
{
	void* stream = malloc(sizeof(uint32_t));
	memcpy(stream, &(messageId), sizeof(uint32_t));

	t_package* package = (t_package*)malloc(sizeof(t_package));
	package->operationCode = ACKNOWLEDGE;
	package->buffer = (t_buffer*)malloc(sizeof(t_buffer));
	package->buffer->bufferSize = sizeof(uint32_t);
	package->buffer->stream = stream;

	void* serializedPackage = SerializePackage(package);

	send(client_socket, serializedPackage, sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t), 0);

	free(stream);
	free(serializedPackage);
}

void SendSubscriptionRequest(message_type queueType, int client_socket)
{
	void* stream = malloc(sizeof(uint32_t));
	memcpy(stream, &(queueType), sizeof(uint32_t));

	t_package* package = (t_package*)malloc(sizeof(t_package));
	package->operationCode = SUBSCRIPTION;
	package->buffer = (t_buffer*)malloc(sizeof(t_buffer));
	package->buffer->bufferSize = sizeof(uint32_t);
	package->buffer->stream = stream;

	void* serializedPackage = SerializePackage(package);

	send(client_socket, serializedPackage, sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t), 0);

	free(stream);
	free(serializedPackage);
}


void Send_NEW(new_pokemon new, int client_socket)
{
	t_message* message = (t_message*)malloc(sizeof(t_message));
	message->id = 0;
	message->correlationId = 0;
	message->messageType = NEW_POKEMON;
	message->messageBuffer = SerializeNewPokemon(new);

	t_package* package = (t_package*)malloc(sizeof(t_package));
	package->operationCode = MESSAGE;
	package->buffer = SerializeMessage(message);

	void* serializedPackage = SerializePackage(package);

	send(client_socket, serializedPackage, sizeof(uint32_t) + sizeof(uint32_t) + package->buffer->bufferSize, 0);

	free(message->messageBuffer);
	free(message);
	free(package->buffer);
	free(package);
	free(serializedPackage);
}

t_package* GetPackage(int client_socket)
{
	uint32_t op_code;
	if(recv(client_socket, &op_code, sizeof(uint32_t), MSG_WAITALL) == -1)
	{
		return 0;
	}
	else
	{
		uint32_t streamSize;
		recv(client_socket, &streamSize, sizeof(uint32_t), MSG_WAITALL);
		void* stream = malloc(streamSize);
		recv(client_socket, stream, streamSize, MSG_WAITALL);

		t_package* recievedPackage = (t_package*)malloc(sizeof(t_package));
		recievedPackage->operationCode = op_code;
		recievedPackage->buffer->bufferSize = streamSize;
		recievedPackage->buffer->stream = stream;
		return recievedPackage;
	}
}

deli_message* GetMessage(int client_socket)
{
	t_package* package = GetPackage(client_socket);
	deli_message* message;
	if(package->operationCode == MESSAGE)
	{
		t_message* recievedMessage = DeserializeMessage(package->buffer->stream);
		message = (deli_message*)malloc(sizeof(recievedMessage));
		message->id = recievedMessage->id;
		message->correlationId = recievedMessage->correlationId;
		message->messageType = recievedMessage->messageType;
		message->messageContent = DeserializeMessageContent(recievedMessage->messageType, recievedMessage->messageBuffer->stream);
	}
	else
	{
		message = 0;
	}
	free(package->buffer->stream);
	free(package);
	return message;
}


uint32_t GetAcknowledge(int client_socket)
{
	t_package* package = GetPackage(client_socket);
	uint32_t acknowledge;
	if(package->operationCode == ACKNOWLEDGE)
	{
		memcpy(&(acknowledge), package->buffer->stream, package->buffer->bufferSize);
	}
	else
	{
		acknowledge = 0;
	}
	free(package->buffer->stream);
	free(package);
	return acknowledge;
}

message_type GetSubscription(int client_socket)
{
	t_package* package = GetPackage(client_socket);
	message_type type;
	if(package->operationCode == ACKNOWLEDGE)
	{
		memcpy(&(type), package->buffer->stream, package->buffer->bufferSize);
	}
	else
	{
		type = 0;
	}
	free(package->buffer->stream);
	free(package);
	return type;
}


//TODO Implement this
/*
void Send_CATCH(catch_pokemon catch, int socket_cliente);
void Send_CAUGHT(caught_pokemon caught, int socket_cliente);
void Send_APPEARED(appeared_pokemon appeared, int socket_cliente);
void Send_LOCALIZED(localized_pokemon localized, int socket_cliente);
void Send_GET(get_pokemon get, int socket_cliente);
*/
