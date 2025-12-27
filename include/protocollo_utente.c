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
void invia_messaggio(int32_t socket_fd, enum Comandi_utente_lavagna comando, uint16_t porta_utente, uint16_t id_card, enum Colonne colonna, char * testo) {
    struct Messaggio_utente_lavagna msg;
    memset(&msg, 0, sizeof(msg));

    msg.comando_utente = htons((uint16_t)comando);
    msg.porta_utente = htons(porta_utente);
    msg.id_card = htons(id_card);
    msg.colonna = htons((uint16_t)colonna);
    if (testo != NULL) snprintf(msg.testo, LUNGHEZZA_TESTO, "%s", testo);

    if (send(socket_fd, &msg, sizeof(msg), 0) < 0) {
        perror("Errore invio messaggio Utente -> Lavagna");
    }
}

// Riceve un messaggio generico lavagna -> utente
int32_t ricevi_messaggio(int32_t socket_fd, struct Messaggio_lavagna_utente * msg) {
    memset(msg, 0, sizeof(struct Messaggio_lavagna_utente));

    int32_t bytes_letti = recv(socket_fd, msg, sizeof(struct Messaggio_lavagna_utente), 0);
    if (bytes_letti == 0) {
        return 0; // Connessione chiusa dall'altra parte
    }
    if (bytes_letti < 0 || bytes_letti != sizeof(struct Messaggio_lavagna_utente)) {
        perror("Errore nella ricezione messaggio Lavagna -> Utente");
        return -1;
    }

    msg->comando_lavagna = ntohs(msg->comando_lavagna);
    msg->id_card = ntohs(msg->id_card);
    msg->num_utenti = ntohs(msg->num_utenti);
    for (uint16_t i = 0; i < MAX_UTENTI; i++) {
        msg->lista_porte[i] = ntohs(msg->lista_porte[i]);
    }

    return bytes_letti;
}