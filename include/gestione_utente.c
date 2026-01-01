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

// Variabili globali dell'utente
extern int mia_porta_p2p;                           // Numero di porta per il review P2P
extern int socket_p2p_listen;                       // File descriptor del socket in ascolto verso i peer
extern int socket_lavagna;                          // File descriptor del socket verso la lavagna
int id_card_in_lavorazione;                         // Card attualemente in lavorazione da parte dell'utente

// Stabilisce la connessione TCP con il server Lavagna: restituisce il file descriptor del socket
int setup_connessione_lavagna() {
    int sockfd;
    struct sockaddr_in address_lavagna;

    // Creazione del file descriptor del socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket fallito");
        exit(EXIT_FAILURE);
    }

    //  Assegnamento valori all'indirizzo del server lavagna
    address_lavagna.sin_family = AF_INET;
    address_lavagna.sin_port = htons(PORTA_LAVAGNA);
    if (inet_pton(AF_INET, "127.0.0.1", &address_lavagna.sin_addr) <= 0) {
        perror("Indirizzo non valido");
        exit(EXIT_FAILURE);
    }

    // Connessione al server lavagna
    if (connect(sockfd, (struct sockaddr *)&address_lavagna, sizeof(address_lavagna)) < 0) {
        perror("Connessione alla Lavagna fallita");
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Prepara il socket in ascolto per le connessioni P2P: restituisce il file descriptor del socket di ascolto
int setup_server_p2p(int porta) {
    int sockfd;
    struct sockaddr_in address;
    int opt = 1;

    // Creazione del file desriptor del socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("Socket P2P fallito");
        exit(EXIT_FAILURE);
    }

    // SO_REUSEADDR permette di riutilizzare immediatamente la porta dopo la chiusura
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt P2P fallito");
        exit(EXIT_FAILURE);
    }

    // Indirizzo per la review P2P
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(porta);

    // Bind del socket all'indirizzo
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind P2P fallito");
        exit(EXIT_FAILURE);
    }

    // Socket passivo: ascolta in attesa di connessioni
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
    
    int bytes_letti = read(STDIN_FILENO, buffer, DIM_BUFFER - 1);
    if (bytes_letti > 0) {
        buffer[bytes_letti - 1] = '\0';

        if (strncmp(buffer, "create ", 7) == 0) { 
            // Comando: create <testo>
            char *testo = buffer + 7;
            if (strlen(testo) > 0) {
                // Invio CMD_CREATE_CARD alla lavagna specificando la descrizione della nuova task
                invia_messaggio(socket_lavagna, CMD_CREATE_CARD, mia_porta_p2p, 0, TODO, testo);
            } else {
                // L'utente non ha rispettato il formato corretto
                printf("Errore: specifica il testo della card.\n");
            }
        }
        else if (strncmp(buffer, "quit", 4) == 0) { 
            // Comando: quit
            // Invia CMD_QUIT alla lavagna e conclude l'esecuzione del programma utente
            invia_messaggio(socket_lavagna, CMD_QUIT, mia_porta_p2p, 0, 0, NULL);
            close(socket_lavagna);
            close(socket_p2p_listen);
            exit(EXIT_SUCCESS);
        }
        else { 
            // Comando non riconosciuto
            printf("Comando non riconosciuto. Usa: 'create <testo>' o 'quit'\n");
        }
    }
}

// Gestisce i messaggi in arrivo dalla Lavagna
int gestisci_messaggio_lavagna() {
    struct Messaggio_lavagna_utente msg;
    int bytes_letti = ricevi_messaggio(socket_lavagna, &msg);
    
    if (bytes_letti <= 0 || bytes_letti != sizeof(struct Messaggio_lavagna_utente)) {
        // Errore in lettura oppure lavagna ha chiuso il socket 
        if (bytes_letti > 0) {
            // Errore in lettura, restituisce -1
            bytes_letti = -1;
        }
        return bytes_letti;
    } 

    switch (msg.comando_lavagna) {
        case CMD_PING: // Lavagna invia ping, risponde con pong
            invia_messaggio(socket_lavagna, CMD_PONG_LAVAGNA, mia_porta_p2p, 0, 0, NULL);
            break;

        case CMD_HANDLE_CARD: // Lavagna assegna una card su cui lavorare
            esegui_task(&msg);
            break;

        case CMD_USER_LIST: // Lavagna invia lista delle porte degli utenti attivi
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

    // Simulazione lavoro (blocco compreso tra DURATA_MINIMA_LAVORO e DURATA_MASSIMA_LAVORO secondi)
    srand(time(NULL));
    int tempo_di_lavoro = (rand() % (DURATA_MASSIMA_LAVORO - DURATA_MINIMA_LAVORO)) + DURATA_MINIMA_LAVORO;
    sleep(tempo_di_lavoro); 
    // Richiesta di review ai peer a lavoro completato
    invia_messaggio(socket_lavagna, CMD_REQUEST_USER_LIST, mia_porta_p2p, id_card_in_lavorazione, 0, NULL);
}

// Review P2P
void esegui_review(struct Messaggio_lavagna_utente * msg) {
    if (msg->num_utenti == 0) { 
        // Se non ci sono utenti aspetta un secondo
        sleep(1);
        // Invia CMD_REQUEST_USER_LIST sperando che si sia connesso qualcuno
        invia_messaggio(socket_lavagna, CMD_REQUEST_USER_LIST, mia_porta_p2p, id_card_in_lavorazione, 0, NULL);
        return;
    }

    // Richiesta di approvazione P2P
    // per via di ((packed)) non è detto che la lista in msg sia allineata: deve essere copiata su un buffer locale
    int lista_porte_allineata[MAX_UTENTI]; 
    for (int i = 0; i < MAX_UTENTI; i++) {
        lista_porte_allineata[i] = msg->lista_porte[i];
    }
    richiedi_review_ai_peer(lista_porte_allineata, msg->num_utenti);
    // Invia CMD_CARD_DONE alla lavagna
    invia_messaggio(socket_lavagna, CMD_CARD_DONE, mia_porta_p2p, id_card_in_lavorazione, 0, NULL);
}

// Contatta gli altri peer per chiedere una review
// Ritorna il numero di review positive ricevute
int richiedi_review_ai_peer(int * lista_porte, int num_utenti) {
    int approvazioni = 0;
    for (int i = 0; i < num_utenti; i++) {
        int porta_target = lista_porte[i];
        
        // Setup socket temporaneo verso il peer
        int socket_peer = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_peer <= 0) continue;

        // Indirizzo del peer a cui chiedere la review
        struct sockaddr_in peer_addr;
        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = htons(porta_target);
        if (inet_pton(AF_INET, "127.0.0.1", &peer_addr.sin_addr) <= 0) {
            close(socket_peer);
            continue;
        }

        // Protocollo P2P Semplificato: 1) Invio "REV", 2) Ricevo "OK" oppure "NO"
        // Bisogna usare di nuovo select() per evitare che due utenti si blocchino aspettando review a vicenda
        if (connect(socket_peer, (struct sockaddr *)&peer_addr, sizeof(peer_addr)) == 0) {
            // Connessione con il peer avvenuta
            char req[] = "REV";
            if (send(socket_peer, req, sizeof(req), 0) > 0) {
                // Invio della richiesta di review "REV" completato con successo
                int ricevuto = 0;
                while (!ricevuto) {
                    fd_set descrittori_lettura; // Set dei descrittori dei socket per la lettura
                    FD_ZERO(&descrittori_lettura); // Svuotamento del set
                    FD_SET(socket_peer, &descrittori_lettura); // Canale dove aspetto "OK"
                    FD_SET(socket_p2p_listen, &descrittori_lettura); // Canale dove arrivano le richieste altrui
                    int max_socket = (socket_peer > socket_p2p_listen) ? socket_peer : socket_p2p_listen;
                    
                    struct timeval timeout = {2, 0}; // Timeout nel caso il peer si disconnetta
                    int attivita = select(max_socket + 1, &descrittori_lettura, NULL, NULL, &timeout);

                    if (attivita > 0) {
                        // La select ha riscontrato dei file descriptor pronti
                        if (FD_ISSET(socket_p2p_listen, &descrittori_lettura)) {
                            // Gestisce la richiesta entrante per sbloccare l'altro utente
                            gestisci_connessione_p2p_entrante();
                        }
                        if (FD_ISSET(socket_peer, &descrittori_lettura)) {
                            // È arrivata la risposta che l'utente aspettava
                            char buffer[DIM_BUFFER];
                            int bytes = recv(socket_peer, buffer, sizeof(buffer), 0);
                            if (bytes > 0) {
                                buffer[bytes] = '\0';
                                if (strcmp(buffer, "OK") == 0) approvazioni++;
                            }
                            ricevuto = 1; // Esce dal while interno
                        }
                    } 
                    else ricevuto = 1; // Timeout scaduto, passa al prossimo peer
                }
            }
        } 
        close(socket_peer);
    }
    return approvazioni;
}

// Gestisce una richiesta di connessione da parte di un altro peer (Server P2P)
void gestisci_connessione_p2p_entrante() {
    int new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    // Accetta una connessione in ingresso e salva il file descriptor del socket
    if ((new_socket = accept(socket_p2p_listen, (struct sockaddr *)&address, &addrlen)) < 0) {
        perror("Accept P2P error");
        return;
    }

    // Gestione veloce della richiesta: viene sempre accettata
    char buffer[DIM_BUFFER] = {0};
    int bytes_letti = recv(new_socket, buffer, DIM_BUFFER - 1, 0);
    if (bytes_letti > 0) {
        buffer[bytes_letti] = '\0';
        if (strcmp(buffer, "REV") == 0) {
            char response[] = "OK";
            send(new_socket, response, strlen(response), 0);
        }
    }
    close(new_socket);
}