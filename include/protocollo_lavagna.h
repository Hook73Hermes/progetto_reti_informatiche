#ifndef PROTOCOLLO_LAVAGNA_H
#define PROTOCOLLO_LAVAGNA_H

#include "gestione_lavagna.h"
#include "common.h"

void invia_messaggio(uint32_t socket_fd, enum Comandi_lavagna_utente comando, struct Card * card, uint16_t *porte, uint16_t num_utenti);
int32_t ricevi_messaggio(uint32_t socket_fd, struct Messaggio_utente_lavagna * msg);

#endif