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

#define PORTA_LAVAGNA 5678                      // Porta del server-lavagna
#define LUNGHEZZA_TESTO 20                      // Lunghezza massima della descrizione delle attività
#define MAX_UTENTI 100                          // Numero massimo si
#define NUM_COLONNE 3
#define NUM_CARD_INIZIALI 10
#define MAX_CARD_DISPLAY 100 
#define LARGHEZZA_DISPLAY_COLONNA 40
#define DURATA_MINIMA_LAVORO 3
#define DURATA_MASSIMA_LAVORO 10
#define TEMPO_PRIMA_DEL_PING 90.0
#define TEMPO_DI_ATTESA_PONG 30.0

// Colonne della lavagna
enum Colonne {
    TODO,
    DOING,
    DONE
};

// Codici comando per il protocollo Lavagna -> Utente
enum Comandi_lavagna_utente {
    CMD_HANDLE_CARD,            // "Push" di una card da eseguire 
    CMD_USER_LIST,              // Risposta a REQUEST_USER_LIST
    CMD_PING,                   // Heartbeat
};

// Struttura del messaggio Lavagna -> Utente
struct Messaggio_lavagna_utente {
    // Header (presente in ogni messaggio)
    uint16_t comando_lavagna;   // Valore da enum Comandi_Lavagna

    // Payload (usato da HANDLE_CARD) ---
    uint16_t id_card;           
    char testo[LUNGHEZZA_TESTO]; // Descrizione attività

    // Payload (usato da HANDLE_CARD e USER_LIST)
    uint16_t lista_porte[MAX_UTENTI]; 
    uint16_t num_utenti;        

} __attribute__((packed));      // niente padding

// Codici comando per il protocollo Utente -> Lavagna
enum Comandi_utente_lavagna {
    CMD_HELLO,                  // Registrazione
    CMD_QUIT,                   // Disconnessione
    CMD_CREATE_CARD,            // Creazione nuova card 
    CMD_ACK_CARD,               // Conferma ricezione/assegnazione 
    CMD_CARD_DONE,              // Notifica completamento attività 
    CMD_REQUEST_USER_LIST,      // Richiesta lista utenti per review
    CMD_PONG_LAVAGNA            // Risposta al ping della lavagna
};

// Struttura del messaggio Utente -> Lavagna
struct Messaggio_utente_lavagna {
    // Header (presente in ogni messaggio)
    uint16_t comando_utente;    // Uno dei valori enum Comandi_utente
    uint16_t porta_utente;

    // Payload (usato da ACK_CARD, CARD_DONE, e CREATE_CARD)
    uint16_t id_card;
    
    // Payload (usato da CREATE_CARD)
    uint16_t colonna;           // Uno dei valori enum Colonne
    char testo[LUNGHEZZA_TESTO]; 
} __attribute__((packed));      // niente padding

#endif