#ifndef PROTOCOLLO_LAVAGNA_H
#define PROTOCOLLO_LAVAGNA_H

#include "include_lavagna.h"

void invia_messaggio(uint32_t socket_fd, enum Comandi_utente_lavagna comando, uint16_t porta_utente, uint16_t id_card, enum Colonne colonna, char * testo);
int32_t ricevi_messaggio(uint32_t socket_fd, struct Messaggio_lavagna_utente * msg);

#endif