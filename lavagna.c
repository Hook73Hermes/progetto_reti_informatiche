#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORTA_LAVAGNA 5678
#define LUNGHEZZA_TESTO 32
#define MAX_UTENTI 100
#define NUM_COLONNE 3

// Colonne della lavagna
enum NomeColonne {
    TODO,
    DOING,
    DONE
};

// Struttura per la Card
struct Card {
    uint32_t id;
    enum NomeColonne colonna;
    char testo[LUNGHEZZA_TESTO];
    uint16_t porta_utente;
    time_t ultima_modifica;
    struct Card * successiva;
};

// Stato globale della lavagna
struct Card * lavagna;

// Utenti connessi identificati dal numero di porta
uint16_t utenti_connessi[MAX_UTENTI];       
uint32_t num_utenti = 0;

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
        int32_t colonne_modificate = 0;   
        for (int32_t num_colonna = 0; num_colonna < NUM_COLONNE; num_colonna++) {
            printf("| ");
            if (card_inizio_colonna[num_colonna]) {
                char testo[LUNGHEZZA_TESTO];
                strcpy(testo, card_inizio_colonna[num_colonna]->testo);
                for (int32_t i = strlen(testo); i < LUNGHEZZA_TESTO - 1; i++) {
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

// Assegna card all'utente
void handle_card_push(uint32_t socket_utente, int indice_card, int porta_dest) {
    // Invia comando HANDLE_CARD con i dati della card e lista utenti [cite: 58]
    // MOVE_CARD verrà eseguito alla ricezione dell'ACK_CARD [cite: 43, 59, 80]
}

int main(int argc, char * argv[]) {
    int32_t socket_lavagna;                         // File descriptor del socket per le richieste di connessione TCP
    struct sockaddr_in indirizzo_lavagna;           // Indirizzo del server lavagna
    uint32_t addrlen = sizeof(indirizzo_lavagna);   // Lunghezza della struttura in byte

    fd_set descrittori_lettura;                     // Set dei descrittori su cui fare polling
    int32_t socket_utenti[MAX_UTENTI] = {0};        // Array per gestire i socket attivi
    int32_t max_sd;                                 // Socket con ID massimo
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
        max_sd = socket_lavagna;

        // Aggiunge i socket degli utenti già connessi al set e calcola il massimo
        for (int32_t i = 0; i < MAX_UTENTI; i++) {
            int32_t sd = socket_utenti[i];
            if (sd > 0) {
                FD_SET(sd, &descrittori_lettura);
                max_sd = (sd > max_sd) ? sd : max_sd;
            }
        }

        // Configura timeout (es. 1.30 min per PING_USER) 
        timeout.tv_sec = 90; 
        timeout.tv_usec = 0;

        int32_t num_descrittori_pronti = select(max_sd + 1, &descrittori_lettura, NULL, NULL, &timeout);
        if (num_descrittori_pronti < 0) {
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
            int32_t socket_utente = accept(socket_lavagna, (struct sockaddr *)&indirizzo_lavagna, &addrlen);
            if (socket_utente < 0) {
                perror("Accept fallita: ");
                continue;
            }
            
            // Salva il nuovo socket nell'array per monitorarlo con la select()
            for (int32_t i = 0; i < MAX_UTENTI; i++) {
                if (socket_utenti[i] == 0) {
                    socket_utenti[i] = socket_utente;
                    break;
                }
            }
        }

        // Gestione messaggi dagli utenti (ACK, QUIT, CARD_DONE)
        for (int32_t i = 0; i < MAX_UTENTI; i++) {
            // int32_t sd = socket_utenti[i];
            // if (FD_ISSET(sd, &descrittori_lettura)) {
            //     char buffer[1024] = {0};
            //     int bytes_letti = read(sd, buffer, 1024); 

            //     if (bytes_letti <= 0) {
            //         // L'utente si è disconnesso bruscamente [cite: 42, 81]
            //         close(sd);
            //         socket_utenti[i] = 0;
            //     } else {
            //         // Processa comando (es. HELLO, QUIT, CARD_DONE) [cite: 40, 41, 62]
            //         // Se HELLO: registra porta e invia HANDLE_CARD 
            //         show_lavagna();
            //     }
            // }
        }
    }

    return 0;
}