#ifndef GESTIONE_LAVAGNA_H
#define GESTIONE_LAVAGNA_H

#include "common.h"

// Struttura per gli utenti
struct Utente {
    uint32_t socket_utente;
    uint16_t porta_utente;
    uint16_t attivo;
    time_t tempo_invio_ping;
    struct Utente * successivo;
};

// Struttura per la Card
struct Card {
    uint16_t id;
    enum Colonne colonna;
    char testo[LUNGHEZZA_TESTO];
    struct Utente * utente;
    time_t ultima_modifica;
    struct Card * successiva;
};

void show_lavagna();

struct Card * get_card_todo();
void sposta_card(uint16_t id, enum Colonne nuova_colonna, struct Utente * nuovo_utente);
void assegna_card(struct Utente * utente);
uint16_t get_prossimo_card_id_libero();
void crea_card(enum Colonne colonna, char * testo);

void resetta_timer_lavoro_utente(struct Utente * utente);
void interrompi_lavoro_utente(struct Utente * utente);
void get_porte_attive(uint16_t * porte_utenti, uint16_t * num_utenti, struct Utente * utente_richiedente);
uint16_t is_utente_occupato(struct Utente * utente);
struct Utente * get_utente_libero_min_porta();
void invia_lista_utenti(struct Utente * utente);

void disattiva_utente(struct Utente * utente);
void attiva_utente(struct Utente * utente, uint16_t porta_utente);
struct Utente * distruggi_utente(struct Utente * utente, struct Utente * precedente);
void crea_utente(uint32_t socket_utente);

#endif