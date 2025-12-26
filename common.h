#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORTA_LAVAGNA 5678
#define LUNGHEZZA_TESTO 32
#define MAX_UTENTI 100
#define NUM_COLONNE 3

// Colonne della lavagna
enum Colonne {
    TODO,
    DOING,
    DONE
};

// Codici comando per il protocollo Utente -> Lavagna
enum Comandi_utente {
    CMD_HELLO,                  // Registrazione
    CMD_QUIT,                   // Disconnessione
    CMD_CREATE_CARD,            // Creazione nuova card 
    CMD_ACK_CARD,               // Conferma ricezione/assegnazione 
    CMD_CARD_DONE,              // Notifica completamento attività 
    CMD_REQUEST_USER_LIST,      // Richiesta lista utenti per review
    CMD_PONG_LAVAGNA            // Risposta al ping della lavagna
};

// Struttura del messaggio Utente -> Lavagna
struct Messaggio_utente {
    // Header (presente in ogni messaggio)
    uint16_t comando_utente;       // Uno dei valori enum Comandi_utente
    uint16_t porta_utente;

    // Payload (usato per ACK_CARD, CARD_DONE, e CREATE_CARD)
    uint16_t id_card;
    
    // Payload (usato per CREATE_CARD)
    enum Colonne colonna;     
    char testo[LUNGHEZZA_TESTO]; 
};

#endif