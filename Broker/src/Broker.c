/*
 ============================================================================
 Name        : Broker.c
 Author      : Mauro
 Version     :
 Copyright   : Your copyright notice
 Description : Broker process
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/temporal.h>

int main(void) {
	puts("Broker (Memoria)"); /* prints Broker (Memoria) */
	char* tiempo = temporal_get_string_time();
	puts(tiempo);
	free(tiempo);
	return EXIT_SUCCESS;
}
