#ifndef DELIBIRD_POKEIO_H_
#define DELIBIRD_POKEIO_H_

#include <sys/socket.h>
#include <stdlib.h>
#include "messages.h"
#include "serialization.h"
#include <string.h>

int SendMessageAcknowledge(int messageId, int client_socket);
int SendSubscriptionRequest(message_type queueType, int client_socket);

int SendMessage(deli_message message, int client_socket);

int Send_NEW(new_pokemon new, int client_socket);
int Send_LOCALIZED(localized_pokemon localized, int client_socket);

//TODO Implement the rest
/*
void Send_CATCH(catch_pokemon catch, int socket_cliente);
void Send_CAUGHT(caught_pokemon caught, int socket_cliente);
void Send_APPEARED(appeared_pokemon appeared, int socket_cliente);
void Send_LOCALIZED(localized_pokemon localized, int socket_cliente);
void Send_GET(get_pokemon get, int socket_cliente);
*/

//REMOVED
/*message_type GetSubscription(int client_socket);
uint32_t GetAcknowledge(int client_socket);
deli_message* GetMessage(int client_socket);*/

int RecievePackage(int client_socket, op_code* operationCode, void** content);

#endif
