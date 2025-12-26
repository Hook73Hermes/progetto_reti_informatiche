#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>

#include "include/common.h"
#include "include/gestione_utente.h"
#include "include/protocollo_utente.h"

// Variabili globali dell'utente
uint16_t mia_porta_p2p;
int32_t socket_lavagna;
int32_t socket_p2p_listen;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Seguire il seguente formato specificando la porta: '%s <porta>'\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fd_set descrittori_lettura;                             // Set dei descrittori su cui fare polling
    int32_t max_socket;                                     // socket con id massimo 

    mia_porta_p2p = (uint16_t)atoi(argv[1]);                // Porta ricevuta da riga di comando
    socket_p2p_listen = setup_server_p2p(mia_porta_p2p);    // Socket verso i peer (server)
    socket_lavagna = setup_connessione_lavagna();           // Socket verso la lavagna (client)

    // Handshake con la lavagna
    invia_messaggio(socket_lavagna, CMD_HELLO, mia_porta_p2p, 0, 0, NULL);

    while (1) {
        FD_ZERO(&descrittori_lettura);
        FD_SET(STDIN_FILENO, &descrittori_lettura);      // Input Tastiera
        FD_SET(socket_lavagna, &descrittori_lettura);    // Messaggi dalla Lavagna
        FD_SET(socket_p2p_listen, &descrittori_lettura); // Richieste di Review da altri peer

        // Calcolo max_socket per la select
        max_socket = (socket_lavagna > socket_p2p_listen) ? socket_lavagna : socket_p2p_listen;
        max_socket = (STDIN_FILENO > max_socket) ? STDIN_FILENO : max_socket;

        // Select: attende che succeda qualcosa su uno dei canali
        if (select(max_socket + 1, &descrittori_lettura, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue; // Errore non fatale
            perror("Errore select");
            exit(EXIT_FAILURE);
        }

        // Input da Tastiera (Utente vuole creare card o uscire)
        if (FD_ISSET(STDIN_FILENO, &descrittori_lettura)) {
            gestisci_input_tastiera();
        }

        // Messaggio dalla Lavagna (Task assegnato, Ping, ecc.)
        if (FD_ISSET(socket_lavagna, &descrittori_lettura)) {
            if (gestisci_messaggio_lavagna() == 0) {
                printf("Lavagna disconnessa. Chiusura.\n");
                break;
            }
        }

        // Connessione P2P in arrivo (Un altro utente vuole una review)
        if (FD_ISSET(socket_p2p_listen, &descrittori_lettura)) {
            gestisci_connessione_p2p_entrante();
        }
    }

    close(socket_lavagna);
    close(socket_p2p_listen);
    exit(EXIT_SUCCESS);
}