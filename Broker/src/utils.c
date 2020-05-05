#include"utils.h"

int iniciar_servidor(char* ip, char* puerto)
{
	int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    for (p=servinfo; p != NULL; p = p->ai_next)
    {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

	listen(socket_servidor, SOMAXCONN);

    freeaddrinfo(servinfo);

    return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	struct sockaddr_in dir_cliente;

	int tam_direccion = sizeof(struct sockaddr_in);

	int respuesta = -1;

	while (respuesta == -1){
		respuesta = accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);
		printf("Valor de respuesta %d \n", respuesta);
	}

	return respuesta;

}

//void serve_client(int* socket, sem_t* mutex_id, uint32_t* ID)
void serve_client(void* variables)
{
//	int cod_op;

	char* msg = "CONECTADO \n";
	puts("antes de enviar el mensaje");
	send((*((t_arg_get_id*)variables)->cliente), msg, strlen(msg), 0);
//	sem_wait(((t_arg_get_id*)variables)->mutex);
	printf("ID del mensaje %d \n", (*((t_arg_get_id*)variables)->id));
	(*((t_arg_get_id*)variables)->id)++;
	sem_post(((t_arg_get_id*)variables)->mutex);
	pthread_exit(NULL);
//	if(recv(*socket, &cod_op, sizeof(int), MSG_WAITALL) == -1)
//		cod_op = -1;
//	process_request(cod_op, *socket);
}

void process_request(int cod_op, int cliente_fd) {
	int size;
	char* procesando = "PROCESANDO MENSAJE \n";
	send(cliente_fd,procesando,strlen(procesando),0);
	void* msg;
		switch (cod_op) {
		case MENSAJE:
//			puts("Llegó");
			msg = recibir_mensaje(cliente_fd, &size);
//			printf("Longitud del mensaje %d \n", size);
//			puts("Recibido");
			puts(msg);
			devolver_mensaje(msg, size, cliente_fd);
//			puts("Enviado");
			free(msg);
			break;
		case 0:
			pthread_exit(NULL);
		case -1:
			pthread_exit(NULL);
		}
}

void* recibir_mensaje(int socket_cliente, int* size)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void devolver_mensaje(void* payload, int size, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = size;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, payload, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

//	printf("Tamaño del stream %d \n",paquete->buffer->size);
//	printf("Tamaño del mensaje a enviar %d \n", bytes);
	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}
