#include <messages.h>
#include <serialization.h>

void SendMessageAcknowledge(int messageId, int client_socket);
void SendSubscriptionRequest(message_type queueType, int client_socket);

void Send_NEW(new_pokemon new, int client_socket);

//TODO Implement the rest
/*
void Send_CATCH(catch_pokemon catch, int socket_cliente);
void Send_CAUGHT(caught_pokemon caught, int socket_cliente);
void Send_APPEARED(appeared_pokemon appeared, int socket_cliente);
void Send_LOCALIZED(localized_pokemon localized, int socket_cliente);
void Send_GET(get_pokemon get, int socket_cliente);
*/

message_type GetSubscription(int client_socket);
uint32_t GetAcknowledge(int client_socket);
deli_message* GetMessage(int client_socket);
