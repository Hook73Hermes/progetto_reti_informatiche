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
uint16_t mia_porta_p2p;                                     // Numero di porta per il review P2P
int32_t socket_p2p_listen;                                  // File descriptor del socket in ascolto verso i peer
int32_t socket_lavagna;                                     // File descriptor del socket verso la lavagna

int main(int argc, char *argv[]) {
    if (argc != 2) {
        // L'utente non ha rispettato il formato corretto
        fprintf(stderr, "Seguire il seguente formato specificando la porta: '%s <porta>'\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Verifica che la porta passata dall'utente sia un intero di valore ammissibile
    char *endptr;
    int64_t porta_long = strtol(argv[1], &endptr, 10);

    // *endptr != '\0' significa che ci sono caratteri non numerici
    // La porta deve essere maggiore di PORTA_LAVAGNA e deve poter essere rappresentata su 16 bit
    if (*endptr != '\0' || porta_long <= PORTA_LAVAGNA || porta_long >= (1 << 16)) {
        fprintf(stderr, "Errore: La porta deve essere un intero maggiore di %d e minore di 65536\n", PORTA_LAVAGNA);
        exit(EXIT_FAILURE);
    }

    // Assegnamento valori alle variabili globali dell'utente
    mia_porta_p2p = (uint16_t)porta_long;                   
    socket_p2p_listen = setup_server_p2p(mia_porta_p2p);
    socket_lavagna = setup_connessione_lavagna();

    fd_set descrittori_lettura;                             // Set dei descrittori su cui fare polling
    int32_t max_socket;                                     // Socket con id massimo 

    // Handshake con la lavagna
    invia_messaggio(socket_lavagna, CMD_HELLO, mia_porta_p2p, 0, 0, NULL);

    // Ciclo infinito per gestire I/O multiplexing
    while (1) {
        FD_ZERO(&descrittori_lettura); // Svuotamento del set dei descrittori per la select()
        FD_SET(STDIN_FILENO, &descrittori_lettura); // Input Tastiera
        FD_SET(socket_lavagna, &descrittori_lettura); // Messaggi dalla Lavagna
        FD_SET(socket_p2p_listen, &descrittori_lettura); // Richieste di Review da altri peer

        // Calcolo max_socket per la select()
        max_socket = (socket_lavagna > socket_p2p_listen) ? socket_lavagna : socket_p2p_listen;
        max_socket = (STDIN_FILENO > max_socket) ? STDIN_FILENO : max_socket;

        // Select: attende indefinitamente che succeda qualcosa su uno dei canali
        if (select(max_socket + 1, &descrittori_lettura, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                // Errore non fatale
                continue;
            }
            // Errore fatale 
            perror("Errore select");
            exit(EXIT_FAILURE);
        }

        // Input da tastiera: utente vuole creare card o uscire
        if (FD_ISSET(STDIN_FILENO, &descrittori_lettura)) {
            gestisci_input_tastiera();
        }

        // Messaggio dalla lavagna: task assegnato, ping, ecc...
        if (FD_ISSET(socket_lavagna, &descrittori_lettura)) {
            if (gestisci_messaggio_lavagna() <= 0) {
                printf("Lavagna disconnessa. Chiusura.\n");
                break;
            }
        }

        // Connessione P2P in arrivo: un altro utente vuole una review
        if (FD_ISSET(socket_p2p_listen, &descrittori_lettura)) {
            gestisci_connessione_p2p_entrante();
        }
    }

    close(socket_lavagna);
    close(socket_p2p_listen);
    exit(EXIT_SUCCESS);
}