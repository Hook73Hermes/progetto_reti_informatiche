#include "common.h"

// Struttura per gli utenti
struct Utente {
    uint32_t socket_utente;
    uint16_t porta_utente;
    uint16_t attivo;
    struct Utente * successivo;
};

// Struttura per la Card
struct Card {
    uint16_t id;
    enum Colonne colonna;
    char testo[LUNGHEZZA_TESTO];
    struct Utente utente;
    time_t ultima_modifica;
    struct Card * successiva;
};

// Stato globale della lavagna
struct Card * lavagna;
struct Utente * lista_utenti;

// Utenti connessi identificati dal numero di porta
uint16_t utenti_connessi[MAX_UTENTI];       
uint16_t num_utenti = 0;

// Stampa la lavagna a video
void show_lavagna() {
    // Puntatori alla prima card di ogni colonna
    struct Card * card_inizio_colonna[NUM_COLONNE] = {0};
    for(struct Card * card = lavagna; card; card = card->successiva) {
        if (!card_inizio_colonna[card->colonna]) {
            card_inizio_colonna[card->colonna] = card;
        }
    }

    // Stampa a video la lavagna
    printf("\n--- STATO LAVAGNA ---\n\n");
    while (1) {
        uint16_t colonne_modificate = 0;   
        for (uint16_t num_colonna = 0; num_colonna < NUM_COLONNE; num_colonna++) {
            printf("| ");
            if (card_inizio_colonna[num_colonna]) {
                char testo[LUNGHEZZA_TESTO];
                strcpy(testo, card_inizio_colonna[num_colonna]->testo);
                for (u_int16_t i = strlen(testo); i < LUNGHEZZA_TESTO - 1; i++) {
                    testo[i] = ' ';
                }
                testo[LUNGHEZZA_TESTO - 1] = '\0';
                printf("%s", testo);
                card_inizio_colonna[num_colonna] = card_inizio_colonna[num_colonna]->successiva;
                colonne_modificate++;
            }
        }
        printf("|\n");
        // Esce quando non viene appesa nessuna card alla lavagna
        if (colonne_modificate == 0)
            break;
    }
    printf("\n---------------------\n\n");
}

int32_t main() {
    uint32_t socket_lavagna;                        // File descriptor del socket per le richieste di connessione TCP
    struct sockaddr_in indirizzo_lavagna;           // Indirizzo del server lavagna
    uint32_t addrlen = sizeof(indirizzo_lavagna);   // Lunghezza della struttura in byte

    fd_set descrittori_lettura;                     // Set dei descrittori su cui fare polling
    uint32_t max_socket;                                // Socket con ID massimo
    struct timeval timeout;                         // Parametro per la accept()

    // Inizializzazione Card (almeno 10)
    // TODO
    // Inserire logica di popolamento iniziale in "To Do" ...

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
    if (setsockopt(socket_lavagna, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt fallito: ");
        close(socket_lavagna);
        exit(EXIT_FAILURE);
    }

    // Bind del socket all'indirizzo
    if (bind(socket_lavagna, (struct sockaddr *)&indirizzo_lavagna, sizeof(indirizzo_lavagna)) < 0) {
        perror("Bind fallito: ");
        close(socket_lavagna);
        exit(EXIT_FAILURE);
    }

    // Socket passivo 
    if (listen(socket_lavagna, 5) < 0) {
        perror("Listen fallita: ");
        close(socket_lavagna);
        exit(EXIT_FAILURE);
    }

    show_lavagna();

    while (1) {
        FD_ZERO(&descrittori_lettura);
        FD_SET(socket_lavagna, &descrittori_lettura);
        max_socket = socket_lavagna;

        // Aggiunge i socket degli utenti già connessi al set e calcola il massimo
        for (struct Utente * utente = lista_utenti; utente; utente = utente->successivo) {
            uint32_t socket_utente = utente->socket_utente;
            if (socket_utente > 0) {
                FD_SET(socket_utente, &descrittori_lettura);
                max_socket = (socket_utente > max_socket) ? socket_utente : max_socket;
            }
        }

        // Configura timeout (es. 1.30 min per PING_USER) 
        timeout.tv_sec = 90; 
        timeout.tv_usec = 0;

        uint32_t num_descrittori_pronti = select(max_socket + 1, &descrittori_lettura, NULL, NULL, &timeout);
        if (num_descrittori_pronti == (uint32_t)-1) {
            if (errno == EINTR) {
                // Errore non fatale: una chiamata di sistema è stata interrotta
                continue;
            } 
            else {
                // Errore fatale (es. memoria esaurita o socket non valido)
                perror("Select fallita con errore fatale: ");
                exit(EXIT_FAILURE); 
            }
        }

        // Se select restituisce 0, è passato il tempo limite 
        if (num_descrittori_pronti == 0) {
            // TODO
            // Logica PING_USER: scorri la lista 'lavagna' e controlla i tempi [cite: 46, 81]
            printf("Timeout raggiunto: avvio controllo PING_USER...\n");
            continue;
        }

        // Utente chiede di stabilire una connessione TCP
        if (FD_ISSET(socket_lavagna, &descrittori_lettura)) {
            uint32_t socket_utente = accept(socket_lavagna, (struct sockaddr *)&indirizzo_lavagna, &addrlen);
            if (socket_utente == (uint32_t)-1) {
                perror("Accept fallita: ");
                continue;
            }
            
            // Salva il nuovo utente nella lista per monitorarlo con la select()
            struct Utente * utente = (struct Utente *)malloc(sizeof(struct Utente));
            memset((void *)utente, 0, sizeof(struct Utente));
            utente->socket_utente = socket_utente;

            if (!lista_utenti) {
                lista_utenti = utente;
            }
            else {
                utente->successivo = lista_utenti;
                lista_utenti = utente;
            }
        }

        // Gestione messaggi dagli utenti (ACK, QUIT, CARD_DONE)
        for (int32_t i = 0; i < MAX_UTENTI; i++) {
            /*int32_t socket_utente = socket_utenti[i];
            if (FD_ISSET(socket_utente, &descrittori_lettura)) {
                char buffer[1024] = {0};
                int bytes_letti = read(socket_utente, buffer, 1024); 

                if (bytes_letti <= 0) {
                    // L'utente si è disconnesso bruscamente [cite: 42, 81]
                    // TODO
                    close(socket_utente);
                    socket_utenti[i] = 0;
                } else {
                    // Processa comando (es. HELLO, QUIT, CARD_DONE) [cite: 40, 41, 62]
                    // Se HELLO: registra porta e invia HANDLE_CARD 
                    show_lavagna();
                }
            }*/
        }
    }

    exit(EXIT_SUCCESS);
}