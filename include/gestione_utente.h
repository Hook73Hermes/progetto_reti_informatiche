#ifndef GESTIONE_UTENTE_H
#define GESTIONE_UTENTE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "common.h"

// Setup Rete
int32_t setup_connessione_lavagna();
int32_t setup_server_p2p(uint16_t porta);

// Gestione Input
void gestisci_input_tastiera();
int32_t gestisci_messaggio_lavagna(); // Ritorna 0 se disconnesso, 1 se ok
void gestisci_connessione_p2p_entrante();

// Logica Task e P2P
void esegui_task(struct Messaggio_lavagna_utente * msg);
void esegui_review(struct Messaggio_lavagna_utente * msg);
uint16_t richiedi_review_ai_peer(uint16_t * lista_porte, uint16_t num_utenti);

#endif