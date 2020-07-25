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

        uint32_t flag=1;
        setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEPORT,&(flag),sizeof(flag));

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

	return accept(socket_servidor, (void*) &dir_cliente, &tam_direccion);

}


//void process_request(int cod_op, int cliente_fd) {
//	int size;
//	char* procesando = "PROCESANDO MENSAJE \n";
//	send(cliente_fd,procesando,strlen(procesando),0);
//	void* msg;
//		switch (cod_op) {
//		case MENSAJE:
////			puts("Llegó");
//			msg = recibir_mensaje(cliente_fd, &size);
////			printf("Longitud del mensaje %d \n", size);
////			puts("Recibido");
//			puts(msg);
//			devolver_mensaje(msg, size, cliente_fd);
////			puts("Enviado");
//			free(msg);
//			break;
//		case 0:
//			pthread_exit(NULL);
//		case -1:
//			pthread_exit(NULL);
//		}
//}
