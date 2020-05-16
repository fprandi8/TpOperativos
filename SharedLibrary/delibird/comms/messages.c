#include "messages.h"

void Free_t_message(t_message* message)
{
	free(message->messageBuffer->stream);
	free(message->messageBuffer);
	free(message);
}

void Free_t_package(t_package* package)
{
	free(package->buffer->stream);
	free(package->buffer);
	free(package);
}

void Free_deli_message(deli_message* message)
{
	free(message->messageContent);
	free(message);
}

void Free_new_pokemon(new_pokemon* new)
{
	free(new->pokemonName);
	free(new);
}

void Free_localized_pokemon(localized_pokemon* localized)
{
	free(localized->coordinates);
	free(localized->pokemonName);
	free(localized);
}

void Free_get_pokemon(get_pokemon* get)
{
	free(get->pokemonName);
	free(get);
}

void Free_appeared_pokemon(appeared_pokemon* appeared)
{
	free(appeared->pokemonName);
	free(appeared);
}

void Free_catch_pokemon(catch_pokemon* catch)
{
	free(catch->pokemonName);
	free(catch);
}


void Free_caught_pokemon(caught_pokemon* caught)
{
	free(caught);
}






