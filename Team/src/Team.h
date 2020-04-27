/*
 * Team.h
 *
 *  Created on: 19 abr. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/log.h>
#include "utils.h"
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <semaphore.h>

t_log* iniciar_logger(void);
t_config* leer_config(void);
t_log* startLogger(char*);
void deleteLogger(t_log*);
void removeLogger(char*);

#endif /* TEAM_H_ */
