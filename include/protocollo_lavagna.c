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
// I campi vengono semplicemente copiati in una struttura messaggio
// I campi interi vengono portati in endianness della rete
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

    // Invio del messaggio utilizzando il socket
    if (send(socket_fd, &msg, sizeof(msg), 0) < 0) {
        perror("Errore nell'invio messaggio Lavagna -> Utente");
    }
}

// Riceve un messaggio generico utente -> lavagna
// Restituisce il numero di bytes letti, 0 se il socket è stato chiuso dall'altra parte e -1 in caso di errori
// I campi del messaggio in ingresso vengono semplicemente copiati in una struttura messaggio passata tramite puntatore
// I campi interi vengono portati in endianness dell'architettura
int ricevi_messaggio(int socket_fd, struct Messaggio_utente_lavagna * msg) {
    memset(msg, 0, sizeof(struct Messaggio_utente_lavagna));

    int bytes_letti = recv(socket_fd, msg, sizeof(struct Messaggio_utente_lavagna), 0);
    if (bytes_letti == 0) {
        // Connessione chiusa dall'altra parte
        return 0; 
    }
    if (bytes_letti < 0 || bytes_letti != sizeof(struct Messaggio_utente_lavagna)) {
        // Errore durante la ricezione
        perror("Errore ricezione messaggio Utente -> Lavagna");
        return -1;
    }

    msg->comando_utente = ntohs(msg->comando_utente);
    msg->porta_utente = ntohs(msg->porta_utente);
    msg->id_card = ntohs(msg->id_card);
    msg->colonna = ntohs(msg->colonna);

    return bytes_letti;
}
