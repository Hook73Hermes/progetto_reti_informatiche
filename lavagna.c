#include "include/common.h"
#include "include/include_lavagna.h"
#include "include/protocollo_lavagna.h"

// Stato globale della lavagna
struct Card * lavagna;
struct Utente * lista_utenti;

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
            char testo[LUNGHEZZA_TESTO];
            if (card_inizio_colonna[num_colonna]) {
                strcpy(testo, card_inizio_colonna[num_colonna]->testo);
                card_inizio_colonna[num_colonna] = card_inizio_colonna[num_colonna]->successiva;
                colonne_modificate++;
            }
            for (u_int16_t i = strlen(testo) - 1; i < LUNGHEZZA_TESTO - 1; i++) {
                testo[i] = ' ';
            }
            testo[LUNGHEZZA_TESTO - 1] = '\0';
            printf("%s", testo);
        }
        printf("|\n");
        // Esce quando non viene appesa nessuna card alla lavagna
        if (colonne_modificate == 0)
            break;
    }
    printf("\n---------------------\n\n");
}

// Cerca una card in TODO e restituisce il puntatore (o NULL)
struct Card * trova_card_todo() {
    struct Card * curr = lavagna;
    while (curr != NULL) {
        if (curr->colonna == TODO) return curr;
        curr = curr->successiva;
    }
    return NULL;
}

// Sposta una card in una nuova colonna
void sposta_card(uint16_t id, enum Colonne nuova_colonna, struct Utente * nuovo_utente) {
    struct Card * curr = lavagna;
    while (curr != NULL) {
        if (curr->id == id) {
            curr->colonna = nuova_colonna;
            curr->ultima_modifica = time(NULL);
            curr->utente = nuovo_utente;
            break; 
        }
        curr = curr->successiva;
    }
}

// Genera array porte per il protocollo
void get_porte_attive(uint16_t * porte_utenti, uint16_t * num_utenti, struct Utente * utente_richiedente) {
    *num_utenti = 0;
    struct Utente * curr = lista_utenti;
    while (curr != NULL && *num_utenti < MAX_UTENTI) {
        if (curr->attivo && curr != utente_richiedente) {
            porte_utenti[(*num_utenti)++] = curr->porta_utente;
        }
        curr = curr->successivo;
    }
}

int32_t main() {
    uint32_t socket_lavagna;                        // File descriptor del socket per le richieste di connessione TCP
    struct sockaddr_in indirizzo_lavagna;           // Indirizzo del server lavagna
    uint32_t addrlen = sizeof(indirizzo_lavagna);   // Lunghezza della struttura in byte

    fd_set descrittori_lettura;                     // Set dei descrittori su cui fare polling
    uint32_t max_socket;                            // Socket con ID massimo
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

        // Polling dei socket
        uint32_t num_descrittori_pronti = select(max_socket + 1, &descrittori_lettura, NULL, NULL, &timeout);
        if (num_descrittori_pronti == (uint32_t)-1) {
            if (errno == EINTR) { // Errore non fatale: una chiamata di sistema è stata interrotta
                continue;
            } 
            else { // Errore fatale (es. memoria esaurita o socket non valido)
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
            if (socket_utente > 0) {
                struct Utente * utente = malloc(sizeof(struct Utente));
                memset(utente, 0, sizeof(struct Utente));
                utente->socket_utente = socket_utente;
                utente->attivo = 0; // Non ancora attivo finché non manda HELLO
                utente->successivo = lista_utenti;
                lista_utenti = utente;
            }
        }

        // Gestione messaggi dagli utenti
        struct Utente * curr = lista_utenti;
        struct Utente * prev = NULL; // Utile per rimozione (QUIT)

        while (curr != NULL) {
            if (FD_ISSET(curr->socket_utente, &descrittori_lettura)) {
                struct Messaggio_utente_lavagna msg;
                int32_t bytes_letti = ricevi_messaggio(curr->socket_utente, &msg);

                if (bytes_letti == 0) { // Utente disconnesso
                    close(curr->socket_utente);
                    if (prev == NULL) lista_utenti = curr->successivo;
                    else prev->successivo = curr->successivo;
                    
                    struct Utente * temp = curr;
                    curr = curr->successivo;
                    free(temp);
                    
                    // TODO: Spostare card da DOING a TODO [cite: 42]
                    show_lavagna();
                    continue;
                } 
                else { // Gestione comandi
                    switch (msg.comando_utente) {
                        case CMD_HELLO:
                            curr->porta_utente = msg.porta_utente;
                            curr->attivo = 1;
                            
                            // Utente appena connesso, se c'è almeno una card in todo gli viene assegnata
                            struct Card * card = trova_card_todo();
                            if (card != NULL) {
                                uint16_t porte_utenti[MAX_UTENTI];
                                uint16_t num_utenti;
                                get_porte_attive(porte_utenti, &num_utenti, curr);
                                invia_messaggio(curr->socket_utente, CMD_HANDLE_CARD, card, porte_utenti, num_utenti);
                            }
                            break;

                        case CMD_ACK_CARD: // Utente conferma presa in carico
                            sposta_card(msg.id_card, DOING, curr);
                            show_lavagna();
                            break;

                        case CMD_REQUEST_USER_LIST: // Utente chiede lista per fare Review [cite: 60]
                            {
                                uint16_t porte[MAX_UTENTI];
                                uint16_t num;
                                get_porte_attive(porte, &num, curr);
                                invia_messaggio(curr->socket_utente, CMD_USER_LIST, NULL, porte, num);
                            }
                            break;

                        case CMD_CARD_DONE:
                            // Utente ha finito -> Sposta in DONE [cite: 62]
                            sposta_card(msg.id_card, DONE, NULL);
                            
                            // Assegna subito un nuovo task se disponibile [cite: 88]
                            struct Card * next_job = trova_card_todo();
                            if (next_job != NULL) {
                                uint16_t porte[MAX_UTENTI];
                                uint16_t num;
                                get_porte_attive(porte, &num, curr);
                                invia_messaggio(curr->socket_utente, CMD_HANDLE_CARD, next_job, porte, num);
                            }
                            show_lavagna();
                            break;
                            
                        case CMD_CREATE_CARD:
                            // Aggiungi nuova card in TODO
                            // Implementare logica di append alla lista card
                            show_lavagna();
                            break;
                    }
                }
            }
            // Avanzamento lista
            prev = curr;
            curr = curr->successivo;
        }
    }

    exit(EXIT_SUCCESS);
}