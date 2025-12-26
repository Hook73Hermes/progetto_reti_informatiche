#ifndef PROTOCOLLO_UTENTE_H
#define PROTOCOLLO_UTENTE_H

#include "gestione_utente.h"
#include "common.h"

void invia_messaggio(uint32_t socket_fd, enum Comandi_utente_lavagna comando, uint16_t porta_utente, uint16_t id_card, enum Colonne colonna, char * testo);
int32_t ricevi_messaggio(uint32_t socket_fd, struct Messaggio_lavagna_utente * msg);

#endif