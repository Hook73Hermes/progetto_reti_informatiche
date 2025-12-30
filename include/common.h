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
#define LUNGHEZZA_TESTO 128                     // Lunghezza massima della descrizione delle attività
#define MAX_UTENTI 100                          // Numero massimo utenti connessi in contemporanea
#define NUM_COLONNE 3                           // Numeri di stati in cui possono trovarsi le card       
#define NUM_CARD_INIZIALI 10                    // Numero di card create all'inizio dalla lavagna
#define MAX_CARD_DISPLAY 100                    // Numero massimo di card per colonna che vengono mostrate a video
#define LARGHEZZA_DISPLAY_COLONNA 40            // Larghezza della colonna quando viene mostrata a video
#define TEMPO_PRIMA_DEL_PING 90.0               // Tempo di lavoro prima che lavagna invii ping
#define TEMPO_DI_ATTESA_PONG 30.0               // Attesa massima del pong dopo il ping prima che l'utente venga disconnesso 

// Colonne della lavagna: una per ogni stato dei lavori
enum Colonne {
    TODO,
    DOING,
    DONE
};

// Codici comando per il protocollo Lavagna -> Utente
enum Comandi_lavagna_utente {
    CMD_HANDLE_CARD,            // Invio di una card da eseguire 
    CMD_USER_LIST,              // Risposta a REQUEST_USER_LIST
    CMD_PING                    // Heartbeat
};

// Struttura del messaggio Lavagna -> Utente
struct Messaggio_lavagna_utente {
    // --- Header --- (presente in ogni messaggio)
    uint16_t comando_lavagna; // Valore da enum Comandi_Lavagna

    // --- Payload --- (alcuni campi possono essere vuoti)
    uint16_t id_card; // Id della card
    char testo[LUNGHEZZA_TESTO]; // Descrizione attività
    uint16_t lista_porte[MAX_UTENTI]; // Lista delle porte degli utenti attivi
    uint16_t num_utenti; // Numero di utenti attivi

} __attribute__((packed)); // niente padding

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
    // --- Header --- (presente in ogni messaggio)
    uint16_t comando_utente; // Valore da enum Comandi_utente
    uint16_t porta_utente; // Numero di porta dell'utente

    // --- Payload --- (alcuni campi possono essere vuoti)
    uint16_t id_card; // Id della card
    uint16_t colonna; // Valore da enum Colonne
    char testo[LUNGHEZZA_TESTO]; // Descrizione attività
} __attribute__((packed)); // niente padding

#endif