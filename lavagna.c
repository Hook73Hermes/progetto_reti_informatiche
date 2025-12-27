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
#include "include/gestione_lavagna.h"
#include "include/protocollo_lavagna.h"

// Stato globale della lavagna
struct Card * lavagna;
struct Utente * lista_utenti;

int32_t main() {
    int32_t socket_lavagna;                         // File descriptor del socket per le richieste di connessione TCP
    struct sockaddr_in indirizzo_lavagna;           // Indirizzo del server lavagna
    uint32_t addrlen = sizeof(indirizzo_lavagna);   // Lunghezza della struttura in byte

    fd_set descrittori_lettura;                     // Set dei descrittori su cui fare polling
    int32_t max_socket;                             // Socket con ID massimo
    struct timeval timeout;                         // Parametro per la select()

    // Inizializzazione card
    for (uint16_t i = 1; i <= NUM_CARD_INIZIALI; i++) {
        char testo[LUNGHEZZA_TESTO];
        sprintf(testo, "Task Iniziale %d", i);
        crea_card(TODO, testo);
    }
    show_lavagna();

    // Indirizzo del server lavagna
    indirizzo_lavagna.sin_family = AF_INET;
    indirizzo_lavagna.sin_addr.s_addr = INADDR_ANY;
    indirizzo_lavagna.sin_port = htons(PORTA_LAVAGNA);

    // Creazione del file descriptor del socket 
    if ((socket_lavagna = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket fallito: ");
        exit(EXIT_FAILURE);
    }

    // SO_REUSEADDR permette di riutilizzare immediatamente la porta 5678 dopo la chiusura del server
    int32_t opt = 1;
    if (setsockopt(socket_lavagna, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt fallito: ");
        exit(EXIT_FAILURE);
    }

    // Bind del socket all'indirizzo
    if (bind(socket_lavagna, (struct sockaddr *)&indirizzo_lavagna, sizeof(indirizzo_lavagna)) < 0) {
        perror("Bind fallito: ");
        exit(EXIT_FAILURE);
    }

    // Socket passivo 
    if (listen(socket_lavagna, 5) < 0) {
        perror("Listen fallita: ");
        exit(EXIT_FAILURE);
    }
    printf(">> Server attivo, in attesa di utenti...\n");

    while (1) {
        FD_ZERO(&descrittori_lettura);
        FD_SET(socket_lavagna, &descrittori_lettura);
        max_socket = socket_lavagna;

        // Aggiunge i socket degli utenti già connessi al set e calcola il massimo
        for (struct Utente * utente = lista_utenti; utente; utente = utente->successivo) {
            int32_t socket_utente = utente->socket_utente;
            if (socket_utente > 0) {
                FD_SET(socket_utente, &descrittori_lettura);
                max_socket = (socket_utente > max_socket) ? socket_utente : max_socket;
            }
        }

        // Configura timeout
        timeout.tv_sec = 5; 
        timeout.tv_usec = 0;

        // Polling dei socket
        uint16_t num_descrittori_pronti = select(max_socket + 1, &descrittori_lettura, NULL, NULL, &timeout);
        if (num_descrittori_pronti == (uint16_t)-1) {
            if (errno == EINTR) continue; // Errore non fatale: una chiamata di sistema è stata interrotta
            perror("Select fallita con errore fatale: ");
            exit(EXIT_FAILURE);
        }

        // Controlla se bisogna spedire dei ping oppure se bisogna disconnettere degli utenti
        time_t now = time(NULL);
        struct Card * card = lavagna;
        while (card != NULL) {
            if (card->colonna == DOING && card->utente != NULL) {
                if (difftime(now, card->ultima_modifica) > TEMPO_PRIMA_DEL_PING) { // Card in doing da oltre 90 secondi
                    struct Utente * utente_sospetto = card->utente;
                    if (utente_sospetto->tempo_invio_ping == 0) { // Invia il ping se non fosse già stato inviato
                        printf(">> Invio ping all'utente %d. In attesa di risposta...\n", utente_sospetto->porta_utente);
                        invia_messaggio(utente_sospetto->socket_utente, CMD_PING, NULL, NULL, 0);
                        utente_sospetto->tempo_invio_ping = now; 
                    }
                    else if (difftime(now, utente_sospetto->tempo_invio_ping) > TEMPO_DI_ATTESA_PONG) { // Distruzione dell'utente
                        struct Utente * utente = lista_utenti;
                        struct Utente * precedente = NULL;
                        while(utente != NULL && utente != utente_sospetto) {
                            precedente = utente;
                            utente = utente->successivo;
                        }
                        printf(">> L'utente %d non risponde. Disconnesso\n", utente_sospetto->porta_utente);
                        FD_CLR(utente_sospetto->socket_utente, &descrittori_lettura);
                        distruggi_utente(utente_sospetto, precedente);
                        assegna_card(NULL);
                        show_lavagna();
                    }
                }
            }
            card = card->successiva;
        }

        // Utente chiede di stabilire una connessione TCP
        if (FD_ISSET(socket_lavagna, &descrittori_lettura)) { // Crea utente o lo inserisce nella lista
            int32_t socket_utente = accept(socket_lavagna, (struct sockaddr *)&indirizzo_lavagna, &addrlen);
            if (socket_utente > 0) crea_utente(socket_utente);
        }

        // Gestione messaggi dagli utenti
        struct Utente * utente = lista_utenti;
        struct Utente * precedente = NULL; // Utile per rimozione (QUIT)

        while (utente != NULL) {
            if (FD_ISSET(utente->socket_utente, &descrittori_lettura)) {
                struct Messaggio_utente_lavagna msg;
                int32_t bytes_letti = ricevi_messaggio(utente->socket_utente, &msg);

                if (bytes_letti <= 0) { // Utente disconnesso
                    printf(">> Utente %d disconnesso\n", utente->porta_utente);
                    utente = distruggi_utente(utente, precedente);
                    assegna_card(NULL); 
                    show_lavagna();
                    continue;
                } 
                else { // Gestione comandi
                    switch (msg.comando_utente) {
                        case CMD_HELLO: // Utente inizia a lavorare
                            printf(">> Utente %d connesso\n", msg.porta_utente);
                            attiva_utente(utente, msg.porta_utente);
                            assegna_card(utente);
                            break;

                        case CMD_ACK_CARD: // Utente conferma presa in carico
                            printf(">> Lavoro %d preso in carico dall'utente %d\n", msg.id_card, utente->porta_utente);
                            sposta_card(msg.id_card, DOING, utente);
                            show_lavagna();
                            break;

                        case CMD_REQUEST_USER_LIST: // Utente chiede lista per fare review
                            invia_lista_utenti(utente);
                            break;

                        case CMD_CARD_DONE: // Utente ha finito, sposta card in DONE e assegna nuova card
                            printf(">> Lavoro %d completato dall'utente %d\n", msg.id_card, utente->porta_utente);
                            sposta_card(msg.id_card, DONE, NULL);
                            assegna_card(utente);
                            show_lavagna();
                            break;
                            
                        case CMD_CREATE_CARD: // Utente chiede la creazione di una nuova card
                            crea_card((enum Colonne)msg.colonna, msg.testo);
                            assegna_card(NULL); // Assegno all'utente di porta minore
                            show_lavagna();
                            break;

                        case CMD_PONG_LAVAGNA:
                            printf(">> L'utente %d ha risposto al ping\n", utente->porta_utente);
                            resetta_timer_lavoro_utente(utente);
                            break;

                        case CMD_QUIT: // Utente finisce di lavorare
                            printf(">> Utente %d disconnesso\n", utente->porta_utente);
                            distruggi_utente(utente, precedente);
                            break;
                    }
                }
            }
            precedente = utente;
            utente = utente->successivo;
        }
    }

    exit(EXIT_SUCCESS);
}