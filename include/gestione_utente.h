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

#define DIM_BUFFER 1024                         // Dimensione di buffer di utilità per le operaizioni di I/O
// Variabili per simulare il lavoro da parte dei client: il lavoro durerà unn numero di secondi casuale
#define DURATA_MINIMA_LAVORO 3                  // Durata minima dei lavori 
#define DURATA_MASSIMA_LAVORO 10                // Durata massima dei lavori

// Setup Rete
int32_t setup_connessione_lavagna(); // Restituisce il file descriptor del socket verso la lavagna
int32_t setup_server_p2p(uint16_t porta); // Restituisce il file descriptor del socket di ascolto verso i peer

// Gestione Input
void gestisci_input_tastiera();
int32_t gestisci_messaggio_lavagna(); // Restituisce 0 se la lavagna si è disconnessa, 1 se è andato tutto bene
void gestisci_connessione_p2p_entrante();

// Logica task e P2P
void esegui_task(struct Messaggio_lavagna_utente * msg);
void esegui_review(struct Messaggio_lavagna_utente * msg);
uint16_t richiedi_review_ai_peer(uint16_t * lista_porte, uint16_t num_utenti); // Restituisce il numero di peer che hanno approvato il lavoro

#endif