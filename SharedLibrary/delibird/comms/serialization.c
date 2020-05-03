#include <serialization.h>

// ---- Serialization ----------------

void* SerializePackage(t_package* package)
{
	//Size of operation_code + stream_size + size of the stream
	void* serializedPackage = malloc(sizeof(uint32_t) + sizeof(uint32_t) + package->buffer->bufferSize);

	int offset = 0;
	memcpy(serializedPackage + offset, &(package->operationCode), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedPackage + offset, &(package->buffer->bufferSize), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedPackage + offset, package->buffer->stream, package->buffer->bufferSize);

	return serializedPackage;
}


t_buffer* SerializeMessage(t_message* message)
{
	//Size of id, corelation id, message type, message size, and size of message stream
	t_buffer* newBuffer = (t_buffer*)malloc(sizeof(t_buffer));
	newBuffer->bufferSize = sizeof(uint32_t) * 4 + message->messageBuffer->bufferSize;

	void serializedMessage = malloc(newBuffer->bufferSize);

	int offset = 0;
	memcpy(serializedMessage + offset, &(message->messageType), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedMessage + offset, &(message->correlationId), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedMessage + offset, &(message->id), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedMessage + offset, &(message->messageBuffer->bufferSize), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(serializedMessage + offset, message->messageBuffer->stream, message->messageBuffer->bufferSize);

	newBuffer->stream = serializedMessage;

	return newBuffer;
}

/*
void* SerializeMessageContent(message_type type, void* content)
{
	void* serializedContent;
	switch(type)
	{
		case NEW_POKEMON:
			serializedContent = SerializeNewPokemon((new_pokemon)content);
			break;
		case LOCALIZED_POKEMON:
			serializedContent = SerializeLocalizedPokemon((localized_pokemon)content);
			break;
		case GET_POKEMON:
			serializedContent = SerializeGetPokemon((get_pokemon)content);
			break;
		case APPEARED_POKEMON:
			serializedContent = SerializeAppearedPokemon((appeared_pokemon)content);
			break;
		case CATCH_POKEMON:
			serializedContent = SerializeCatchPokemon((catch_pokemon)content);
			break;
		case CAUGHT_POKEMON:
			serializedContent = SerializeCaughtPokemon((caught_pokemon)content);
			break;
		default:
	}
}
*/

t_buffer* SerializeNewPokemon(new_pokemon newPokemon)
{
	//size of name, name, cordinate in x, cordinate in y, ammoun of pokemon
	uint32_t sizeOfPokemonName = strlen(newPokemon->pokemonName) + 1;

	t_buffer* newBuffer = (t_buffer*)malloc(sizeof(t_buffer));
	newBuffer->bufferSize = sizeof(uint32_t) * 4 + sizeOfPokemonName;

	void* newPokemonSerialized = malloc(newBuffer->bufferSize);

	int offset = 0;
	memcpy(newPokemonSerialized + offset, &(sizeOfPokemonName), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(newPokemonSerialized + offset, newPokemon->pokemonName, sizeOfPokemonName);
	offset += sizeOfPokemonName;
	memcpy(newPokemonSerialized + offset, &(newPokemon->horizontalCoordinate), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(newPokemonSerialized + offset, &(newPokemon->verticalCoordinate), sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(newPokemonSerialized + offset, &(newPokemon->ammount),  sizeof(uint32_t));

	newBuffer->stream = newPokemonSerialized;

	return newBuffer;
}

// ---- De-serialization ----------------

t_package* DeserializePackage(void* serializedPackage)
{
	t_package* package = (t_package*)malloc(sizeof(t_package));
	package->buffer = (t_buffer*)malloc(sizeof(t_buffer));

	memcpy(&(package->operationCode), serializedPackage, sizeof(uint32_t));
	serializedPackage += sizeof(uint32_t);
	memcpy(&(package->buffer->bufferSize), serializedPackage, sizeof(uint32_t));
	serializedPackage += sizeof(uint32_t);
	memcpy(&(package->buffer->stream), serializedPackage, package->buffer->bufferSize);

	return package;
}

t_message* DeserializeMessage(void* serializedMessage)
{
	//Size of id, correlation id, message type, message size, and size of message stream
	t_message* message = (t_message*)malloc(sizeof(t_message));
	message->messageBuffer = (t_buffer*)malloc(sizeof(t_buffer));

	memcpy(&(message->messageType), serializedMessage, sizeof(uint32_t));
	serializedMessage += sizeof(uint32_t);
	memcpy(&(message->correlationId), serializedMessage, sizeof(uint32_t));
	serializedMessage += sizeof(uint32_t);
	memcpy(&(message->id), serializedMessage, sizeof(uint32_t));
	serializedMessage += sizeof(uint32_t);
	memcpy(&(message->messageBuffer->bufferSize), serializedMessage, sizeof(uint32_t));
	serializedMessage += sizeof(uint32_t);
	memcpy(message->messageBuffer->stream, serializedMessage, message->messageBuffer->bufferSize);

	return message;
}


void* DeerializeMessageContent(message_type type, void* serializedContent)
{
	void* content;
	switch(type)
	{
		case NEW_POKEMON:
			serializedContent = DeserializeNewPokemon((new_pokemon)content);
			break;
		case LOCALIZED_POKEMON:
			serializedContent = DeserializeLocalizedPokemon((localized_pokemon)content);
			break;
		case GET_POKEMON:
			serializedContent = DeserializeGetPokemon((get_pokemon)content);
			break;
		case APPEARED_POKEMON:
			serializedContent = DeserializeAppearedPokemon((appeared_pokemon)content);
			break;
		case CATCH_POKEMON:
			serializedContent = DeserializeCatchPokemon((catch_pokemon)content);
			break;
		case CAUGHT_POKEMON:
			serializedContent = DeserializeCaughtPokemon((caught_pokemon)content);
			break;
		default:
			return 0;
	}
}


new_pokemon* DeserializeNewPokemon(void* serializedNewPokemon)
{
	//size of name, name, cordinate in x, cordinate in y, ammoun of pokemon
	//void* newPokemonSerialized = malloc(sizeof(uint32_t) * 4 + sizeOfPokemonName);
	new_pokemon* newPokemon = (new_pokemon*)malloc(sizeof(new_pokemon));

	uint32_t sizeOfPokemonName;
	memcpy(&(sizeOfPokemonName),serializedNewPokemon, sizeof(uint32_t));
	serializedNewPokemon += sizeof(uint32_t);
	memcpy(newPokemon->pokemonName, serializedNewPokemon, sizeOfPokemonName);
	serializedNewPokemon += sizeOfPokemonName;
	memcpy(&(newPokemon->horizontalCoordinate), serializedNewPokemon, sizeof(uint32_t));
	serializedNewPokemon += sizeof(uint32_t);
	memcpy(&(newPokemon->verticalCoordinate), serializedNewPokemon, sizeof(uint32_t));
	serializedNewPokemon += sizeof(uint32_t);
	memcpy(&(newPokemon->ammount), serializedNewPokemon,  sizeof(uint32_t));

	return newPokemon;
}


