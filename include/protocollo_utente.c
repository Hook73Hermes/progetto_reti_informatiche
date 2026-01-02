#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "protocollo_utente.h"

// Invia un messaggio generico utente -> lavagna
// Per lasciare dei dati di un pacchetto a 0, basta passare NULL per il rispettivo parametro
// I campi interi vengono portati in endianness della rete
// I campi vengono copiati in una struttura messaggio e poi serializzati e spediti
void invia_messaggio(int socket_fd, enum Comandi_utente_lavagna comando, int porta_utente, int id_card, enum Colonne colonna, char * testo) {
    struct Messaggio_utente_lavagna msg;
    memset(&msg, 0, sizeof(msg));

    msg.comando_utente = htons((uint16_t)comando);
    msg.porta_utente = htons((uint16_t)porta_utente);
    msg.id_card = htons((uint16_t)id_card);
    msg.colonna = htons((uint16_t)colonna);
    if (testo != NULL) snprintf(msg.testo, LUNGHEZZA_TESTO, "%s", testo);

    // Serializzazione del messaggio
    char buffer[DIM_MSG_UTENTE_LAVAGNA];
    serializza_messaggio(buffer, msg);

    // Invio del messaggio utilizzando il socket
    if (send(socket_fd, buffer, DIM_MSG_UTENTE_LAVAGNA, 0) < 0) {
        perror("Errore invio messaggio Utente -> Lavagna");
    }
}

// Riceve un messaggio generico lavagna -> utente
// Restituisce il numero di bytes letti, 0 se il socket è stato chiuso dall'altra parte e -1 in caso di errori
// Il messaggio ricevuto e deserializzato in una struttura messaggio
// I campi interi vengono portati in endianness dell'architettura
int ricevi_messaggio(int socket_fd, struct Messaggio_lavagna_utente * msg) {
    char buffer[DIM_MSG_LAVAGNA_UTENTE];

    int bytes_letti = recv(socket_fd, buffer, DIM_MSG_LAVAGNA_UTENTE, 0);
    if (bytes_letti == 0) {
        // Connessione chiusa dall'altra parte
        return 0; 
    }
    if (bytes_letti < 0 || bytes_letti != DIM_MSG_LAVAGNA_UTENTE) {
        // Errore durante la ricezione
        perror("Errore nella ricezione messaggio Lavagna -> Utente");
        return -1;
    }

    // Deserializzazione del messaggio
    memset(msg, 0, sizeof(struct Messaggio_lavagna_utente));
    deserializza_messaggio(buffer, msg);

    msg->comando_lavagna = ntohs(msg->comando_lavagna);
    msg->id_card = ntohs(msg->id_card);
    msg->num_utenti = ntohs(msg->num_utenti);
    for (int i = 0; i < MAX_UTENTI; i++) {
        msg->lista_porte[i] = ntohs(msg->lista_porte[i]);
    }

    return bytes_letti;
}

// Funzione che serializza un generico messaggio utente ->lavagna
// Trasferisce tutti i campi da una struttura a un buffer di char
void serializza_messaggio(char * buffer, struct Messaggio_utente_lavagna msg) {
    memcpy(buffer, &(msg.comando_utente), 2);
    memcpy(buffer + 2, &(msg.porta_utente), 2);
    memcpy(buffer + 4, &(msg.id_card), 2);
    memcpy(buffer + 6, &(msg.colonna), 2);
    memcpy(buffer + 8, &(msg.testo), LUNGHEZZA_TESTO);
}

// Funzione che deserializza un generico messaggio lavagna -> utente 
// Trasferisce tutti i campi da un buffer di char a una struttura messaggio
void deserializza_messaggio(char * buffer, struct Messaggio_lavagna_utente * msg) {
    memcpy(&(msg->comando_lavagna), buffer, 2);
    memcpy(&(msg->id_card), buffer + 2, 2);
    memcpy(&(msg->testo), buffer + 4, LUNGHEZZA_TESTO);
    memcpy(&(msg->lista_porte), buffer + 4 + LUNGHEZZA_TESTO, 2 * MAX_UTENTI);
    memcpy(&(msg->num_utenti), buffer + 4 + LUNGHEZZA_TESTO + 2 * MAX_UTENTI, 2);
}