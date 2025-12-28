#ifndef PROTOCOLLO_LAVAGNA_H
#define PROTOCOLLO_LAVAGNA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "gestione_lavagna.h"
#include "common.h"

void invia_messaggio(int32_t socket_fd, enum Comandi_lavagna_utente comando, struct Card * card, uint16_t * porte, uint16_t num_utenti);
int32_t ricevi_messaggio(int32_t socket_fd, struct Messaggio_utente_lavagna * msg); // Restituisce 0 per connessione chiusa, -1 per errori

#endif