#ifndef GESTIONE_LAVAGNA_H
#define GESTIONE_LAVAGNA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "common.h"

// Struttura per gli utenti
struct Utente {
    int32_t socket_utente;                  // File descriptor del socket
    uint16_t porta_utente;                  // Numero di porta dell'utente
    uint16_t attivo;                        // Un utente è attivo solo dopo l'HELLO
    time_t tempo_invio_ping;                // Istante di invio del ping (server in attesa di pong)
    struct Utente * successivo;             // Puntatore per linked list
};

// Struttura per la Card
struct Card {                               
    uint16_t id;                            // Id della card
    enum Colonne colonna;                   // Stato della card
    char testo[LUNGHEZZA_TESTO];            // Descrizione attività
    struct Utente * utente;                 // Utente che ha in carico la card
    time_t ultima_modifica;                 // Istante di ultima modifica
    struct Card * successiva;               // Puntatore per linked list
};

// Display lavagna
void show_lavagna();

// Gestione delle card
struct Card * get_card_todo(); // Restituisce la prima card in todo
void sposta_card(uint16_t id, enum Colonne nuova_colonna, struct Utente * nuovo_utente);
void assegna_card(struct Utente * utente);
uint16_t get_prossimo_card_id_libero(); // Restituisce il primo id libero
void crea_card(enum Colonne colonna, char * testo);

// Gestione degli utenti
void resetta_timer_lavoro_utente(struct Utente * utente);
void interrompi_lavoro_utente(struct Utente * utente);
void get_porte_attive(uint16_t * porte_utenti, uint16_t * num_utenti, struct Utente * utente_richiedente);
uint16_t is_utente_occupato(struct Utente * utente); // 1 se occupato, 0 se libero
struct Utente * get_utente_libero_min_porta();
void invia_lista_utenti(struct Utente * utente);
uint16_t conta_utenti_connessi(); // Restituisce il numero di utenti connessi

// Operazioni sugli utenti
void disattiva_utente(struct Utente * utente);
void attiva_utente(struct Utente * utente, uint16_t porta_utente);
struct Utente * distruggi_utente(struct Utente * utente, struct Utente * precedente); // Restituisce il successivo nella linked list
void crea_utente(int32_t socket_utente);

#endif