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

void invia_messaggio(int socket_fd, enum Comandi_lavagna_utente comando, struct Card * card, int * porte, int num_utenti);
int ricevi_messaggio(int socket_fd, struct Messaggio_utente_lavagna * msg); // Restituisce 0 per connessione chiusa, -1 per errori

#endif