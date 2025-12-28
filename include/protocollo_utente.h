#ifndef PROTOCOLLO_UTENTE_H
#define PROTOCOLLO_UTENTE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "gestione_utente.h"
#include "common.h"

void invia_messaggio(int32_t socket_fd, enum Comandi_utente_lavagna comando, uint16_t porta_utente, uint16_t id_card, enum Colonne colonna, char * testo);
int32_t ricevi_messaggio(int32_t socket_fd, struct Messaggio_lavagna_utente * msg); // Restituisce 0 per connessione chiusa, -1 per errori

#endif