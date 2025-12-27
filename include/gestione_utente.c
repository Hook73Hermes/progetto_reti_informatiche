#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "common.h"
#include "gestione_utente.h"
#include "protocollo_utente.h"

#define DIM_BUFFER 1024

// Variabili globali dell'utente
extern uint16_t mia_porta_p2p;
extern int32_t socket_lavagna;
extern int32_t socket_p2p_listen;
uint16_t id_card_in_lavorazione;

// Stabilisce la connessione TCP con il server Lavagna
int32_t setup_connessione_lavagna() {
    int32_t sockfd;
    struct sockaddr_in address_lavagna;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket fallito");
        exit(EXIT_FAILURE);
    }

    address_lavagna.sin_family = AF_INET;
    address_lavagna.sin_port = htons(PORTA_LAVAGNA);
    if (inet_pton(AF_INET, "127.0.0.1", &address_lavagna.sin_addr) <= 0) {
        perror("Indirizzo non valido");
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&address_lavagna, sizeof(address_lavagna)) < 0) {
        perror("Connessione alla Lavagna fallita");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Prepara il socket in ascolto per le connessioni P2P
int32_t setup_server_p2p(uint16_t porta) {
    int32_t sockfd;
    struct sockaddr_in address;
    int32_t opt = 1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("Socket P2P fallito");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt P2P fallito");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(porta);

    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind P2P fallito");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0) {
        perror("Listen P2P fallita");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Gestisce i comandi digitati dall'utente
void gestisci_input_tastiera() {
    char buffer[DIM_BUFFER];
    memset(buffer, 0, sizeof(buffer));
    
    int32_t bytes_letti = read(STDIN_FILENO, buffer, DIM_BUFFER - 1);
    if (bytes_letti > 0) {
        buffer[bytes_letti - 1] = '\0';

        if (strncmp(buffer, "create ", 7) == 0) { // Comando: create <testo>
            char *testo = buffer + 7;
            if (strlen(testo) > 0) {
                invia_messaggio(socket_lavagna, CMD_CREATE_CARD, mia_porta_p2p, 0, TODO, testo);
            } else {
                printf("Errore: specifica il testo della card.\n");
            }
        }
        else if (strncmp(buffer, "quit", 4) == 0) { // Comando: quit
            invia_messaggio(socket_lavagna, CMD_QUIT, mia_porta_p2p, 0, 0, NULL);
            close(socket_lavagna);
            close(socket_p2p_listen);
            exit(EXIT_SUCCESS);
        }
        else { // Comando non riconosciuto
            printf("Comando non riconosciuto. Usa: 'create <testo>' o 'quit'\n");
        }
    }
}

// Gestisce i messaggi in arrivo dalla Lavagna
int32_t gestisci_messaggio_lavagna() {
    struct Messaggio_lavagna_utente msg;
    int32_t bytes_letti = ricevi_messaggio(socket_lavagna, &msg);
    
    if (bytes_letti <= 0 || bytes_letti != sizeof(struct Messaggio_lavagna_utente)) return 0; // Disconnessione

    switch (msg.comando_lavagna) {
        case CMD_PING:
            invia_messaggio(socket_lavagna, CMD_PONG_LAVAGNA, mia_porta_p2p, 0, 0, NULL);
            break;

        case CMD_HANDLE_CARD:
            esegui_task(&msg);
            break;

        case CMD_USER_LIST:
            esegui_review(&msg);
            break;
    }
    return 1;
}

// Gestione del task fino alla review P2P
void esegui_task(struct Messaggio_lavagna_utente * msg) {
    // Conferma di aver preso il lavoro
    id_card_in_lavorazione = msg->id_card;
    invia_messaggio(socket_lavagna, CMD_ACK_CARD, mia_porta_p2p, id_card_in_lavorazione, 0, NULL);

    // Simulazione lavoro (blocco compreso tra 3 e 10 secondi) e richiesta dei peer a lavoro completato
    srand(time(NULL));
    uint16_t tempo_di_lavoro = (rand() % (DURATA_MASSIMA_LAVORO - DURATA_MINIMA_LAVORO)) + DURATA_MINIMA_LAVORO;
    sleep(tempo_di_lavoro); 
    invia_messaggio(socket_lavagna, CMD_REQUEST_USER_LIST, mia_porta_p2p, id_card_in_lavorazione, 0, NULL);
}

// Review P2P
void esegui_review(struct Messaggio_lavagna_utente * msg) {
    if (msg->num_utenti == 0) { // Ogni secondo verifica se sono arrivati altri utenti
        sleep(1);
        invia_messaggio(socket_lavagna, CMD_REQUEST_USER_LIST, mia_porta_p2p, id_card_in_lavorazione, 0, NULL);
        return;
    }
    // Richiesta di approvazione P2P
    uint16_t lista_porte_allineata[MAX_UTENTI]; // per via di ((packed)) non è detto che la lista in msg sia allineata
    memcpy(lista_porte_allineata, msg->lista_porte, sizeof(lista_porte_allineata));
    richiedi_review_ai_peer(lista_porte_allineata, msg->num_utenti);
    // Lavoro finito
    invia_messaggio(socket_lavagna, CMD_CARD_DONE, mia_porta_p2p, id_card_in_lavorazione, 0, NULL);
    id_card_in_lavorazione = 0;
}

// Contatta gli altri peer per chiedere una review
// Ritorna il numero di review positive ricevute
uint16_t richiedi_review_ai_peer(uint16_t * lista_porte, uint16_t num_utenti) {
    uint16_t approvazioni = 0;
    for (uint16_t i = 0; i < num_utenti; i++) {
        uint16_t porta_target = lista_porte[i];
        
        // Setup socket temporaneo verso il peer
        int32_t socket_peer = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_peer <= 0) continue;

        struct sockaddr_in peer_addr;
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = htons(porta_target);
        if (inet_pton(AF_INET, "127.0.0.1", &peer_addr.sin_addr) <= 0) {
            close(socket_peer);
            continue;
        }

        // Protocollo P2P Semplificato: 1) Invio "REV", 2) Ricevo "OK" oppure "NO"
        // Bisogno usare di nuovo connect() per evitare che due utenti si blocchino aspettando review a vicenda
        if (connect(socket_peer, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == 0) {
            char req[] = "REV";
            if (send(socket_peer, req, sizeof(req), 0) > 0) {
                int ricevuto = 0;
                while (!ricevuto) {
                    fd_set descrittori_lettura;
                    FD_ZERO(&descrittori_lettura);
                    FD_SET(socket_peer, &descrittori_lettura); // Canale dove aspetto "OK"
                    FD_SET(socket_p2p_listen, &descrittori_lettura); // Canale dove arrivano le richieste altrui
                    int32_t max_socket = (socket_peer > socket_p2p_listen) ? socket_peer : socket_p2p_listen;
                    
                    struct timeval timeout = {2, 0}; // Timeout nel caso il peer morisse
                    int32_t attivita = select(max_socket + 1, &descrittori_lettura, NULL, NULL, &timeout);

                    if (attivita > 0) {
                        // Qualcuno (magari chi sto chiamando) vuole una review da me
                        if (FD_ISSET(socket_p2p_listen, &descrittori_lettura)) // Gestisco la richiesta entrante per SBLOCCARE l'altro utente
                            gestisci_connessione_p2p_entrante();
                        // È arrivata la risposta che aspettavo
                        if (FD_ISSET(socket_peer, &descrittori_lettura)) {
                            char buffer[DIM_BUFFER];
                            int32_t bytes = recv(socket_peer, buffer, sizeof(buffer), 0);
                            if (bytes > 0) {
                                buffer[bytes] = '\0';
                                if (strcmp(buffer, "OK") == 0) approvazioni++;
                            }
                            ricevuto = 1; // Esco dal while interno
                        }
                    } 
                    else ricevuto = 1; // Timeout scaduto, passo al prossimo peer
                }
            }
        } 
        close(socket_peer);
    }
    return approvazioni;
}

// Gestisce una richiesta di connessione da parte di un altro peer (Server P2P)
void gestisci_connessione_p2p_entrante() {
    int32_t new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    if ((new_socket = accept(socket_p2p_listen, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("Accept P2P error");
        return;
    }

    // Gestione veloce della richiesta
    char buffer[DIM_BUFFER] = {0};
    int32_t bytes_letti = recv(new_socket, buffer, DIM_BUFFER - 1, 0);
    if (bytes_letti > 0) {
        buffer[bytes_letti] = '\0';
        if (strcmp(buffer, "REV") == 0) {
            char response[] = "OK";
            send(new_socket, response, strlen(response), 0);
        }
    }
    close(new_socket);
}