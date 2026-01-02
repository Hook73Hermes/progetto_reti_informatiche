#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "protocollo_lavagna.h"

// Invia un messaggio generico lavagna -> utente
// Per lasciare dei dati di un pacchetto a 0, basta passare NULL per il rispettivo parametro
// I campi interi vengono portati in endianness della rete
// I campi vengono copiati in una struttura messaggio e poi serializzati e spediti
void invia_messaggio(int socket_fd, enum Comandi_lavagna_utente comando, struct Card * card, int * utenti_connessi, int num_utenti) {
    struct Messaggio_lavagna_utente msg;
    memset(&msg, 0, sizeof(msg));

    msg.comando_lavagna = htons((uint16_t)comando);

    if (card != NULL) {
        msg.id_card = htons((uint16_t)card->id);
        snprintf(msg.testo, LUNGHEZZA_TESTO, "%s", card->testo);
    }

    if (utenti_connessi != NULL && num_utenti > 0) {
        msg.num_utenti = htons((uint16_t)num_utenti);
        for (int i = 0; i < MAX_UTENTI; i++) {
            msg.lista_porte[i] = htons((uint16_t)utenti_connessi[i]);
        }
    }

    // Serializzazione del messaggio
    char buffer[DIM_MSG_LAVAGNA_UTENTE];
    serializza_messaggio(buffer, msg);

    // Invio del messaggio utilizzando il socket
    if (send(socket_fd, buffer, DIM_MSG_LAVAGNA_UTENTE, 0) < 0) {
        perror("Errore nell'invio messaggio Lavagna -> Utente");
    }
}

// Riceve un messaggio generico utente -> lavagna
// Restituisce il numero di bytes letti, 0 se il socket è stato chiuso dall'altra parte e -1 in caso di errori
// Il messaggio ricevuto e deserializzato in una struttura messaggio
// I campi interi vengono portati in endianness dell'architettura
int ricevi_messaggio(int socket_fd, struct Messaggio_utente_lavagna * msg) {
    char buffer[DIM_MSG_UTENTE_LAVAGNA];

    int bytes_letti = recv(socket_fd, buffer, DIM_MSG_UTENTE_LAVAGNA, 0);
    if (bytes_letti == 0) {
        // Connessione chiusa dall'altra parte
        return 0; 
    }
    if (bytes_letti < 0 || bytes_letti != DIM_MSG_UTENTE_LAVAGNA) {
        // Errore durante la ricezione
        perror("Errore ricezione messaggio Utente -> Lavagna");
        return -1;
    }
    
    // Deserializzazione del messaggio
    memset(msg, 0, sizeof(struct Messaggio_utente_lavagna));
    deserializza_messaggio(buffer, msg);

    msg->comando_utente = ntohs(msg->comando_utente);
    msg->porta_utente = ntohs(msg->porta_utente);
    msg->id_card = ntohs(msg->id_card);
    msg->colonna = ntohs(msg->colonna);

    return bytes_letti;
}

// Funzione che serializza un generico messaggio lavagna -> utente
// Trasferisce tutti i campi da una struttura messaggio a un buffer di char
void serializza_messaggio(char * buffer, struct Messaggio_lavagna_utente msg) {
    memcpy(buffer, &(msg.comando_lavagna), 2);
    memcpy(buffer + 2, &(msg.id_card), 2);
    memcpy(buffer + 4, &(msg.testo), LUNGHEZZA_TESTO);
    memcpy(buffer + 4 + LUNGHEZZA_TESTO, &(msg.lista_porte), 2 * MAX_UTENTI);
    memcpy(buffer + 4 + LUNGHEZZA_TESTO + 2 * MAX_UTENTI, &(msg.num_utenti), 2);
}

// Funzione che deserializza un generico messaggio utente -> lavagna
// Trasferisce tutti i campi da un buffer di char a una struttura messaggio
void deserializza_messaggio(char * buffer, struct Messaggio_utente_lavagna * msg) {
    memcpy(&(msg->comando_utente), buffer, 2);
    memcpy(&(msg->porta_utente), buffer + 2, 2);
    memcpy(&(msg->id_card), buffer + 4, 2);
    memcpy(&(msg->colonna), buffer + 6, 2);
    memcpy(&(msg->testo), buffer + 8, LUNGHEZZA_TESTO);
}
