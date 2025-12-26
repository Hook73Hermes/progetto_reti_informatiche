#ifndef INCLUDE_LAVAGNA_H
#define INCLUDE_LAVAGNA_H

#include "common.h"

// Struttura per gli utenti
struct Utente {
    uint32_t socket_utente;
    uint16_t porta_utente;
    uint16_t attivo;
    struct Utente * successivo;
};

// Struttura per la Card
struct Card {
    uint16_t id;
    enum Colonne colonna;
    char testo[LUNGHEZZA_TESTO];
    struct Utente utente;
    time_t ultima_modifica;
    struct Card * successiva;
};

#endif