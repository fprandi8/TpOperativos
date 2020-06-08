#ifndef DELIBIRD_SERIALIZATION_H_
#define DELIBIRD_SERIALIZATION_H_

#include <string.h>
#include <stdlib.h>
#include "messages.h"

void* SerializePackage(t_package*);

t_buffer* SerializeMessage(t_message*);

//--Will keep this for now, might delete later
//t_buffer* SerializeMessageContent(message_type, void*);

t_message* ConvertDeliMessageToMessage(deli_message* deliMessage);

void* SerializeDeliMessage(deli_message*);

t_buffer* SerializeNewPokemon(new_pokemon*);
t_buffer* SerializeLocalizedPokemon(localized_pokemon*);
t_buffer* SerializeGetPokemon(get_pokemon*);

//TODO Implement this
/*
t_buffer *SerializeGetPokemon(get_pokemon);
t_buffer *SerializeAppearedPokemon(appeared_pokemon);
t_buffer *SerializeCatchPokemon(catch_pokemon);
t_buffer *SerializeCaughtPokemon(caught_pokemon);
*/
t_package* DeserializePackage(void*);

t_message* DeserializeMessage(void*);

void* DeserializeMessageContent(message_type, void*);

new_pokemon* DeserializeNewPokemon(void*);
localized_pokemon* DeserializeLocalizedPokemon(void*);
get_pokemon* DeserializeGetPokemon(void*);

//TODO Implement this
/*
get_pokemon *DeserializeGetPokemon(void*);
appeared_pokemon *DeserializeAppearedPokemon(void*);
catch_pokemon *DeserializeCatchPokemon(void*);
caught_pokemon *DeserializeCaughtPokemon(void*);
*/

#endif
