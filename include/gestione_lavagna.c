#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "common.h"
#include "gestione_lavagna.h"
#include "protocollo_lavagna.h"

// Stato globale della lavagna: stesse variabili di lavagna.c
extern struct Card * lavagna;
extern struct Utente * lista_utenti;

// Colonne come stringhe: utile per show_lavagna()
const char * stringhe_colonne[NUM_COLONNE] = {
    [TODO] = "TODO",
    [DOING] = "DOING",
    [DONE] = "DONE"
};

// Stampa a video la lavagna
void show_lavagna() {
    // Strutture temporanee per organizzare la visualizzazione
    struct Card * colonne_visuali[NUM_COLONNE][MAX_CARD_DISPLAY];
    int card_per_colonna[NUM_COLONNE] = {0};
    memset(colonne_visuali, 0, sizeof(colonne_visuali));

    // Scorre la lista una sola volta e inserisce le card nella colonna giusta
    struct Card * card = lavagna;
    while (card != NULL) {
        enum Colonne colonna = card->colonna;
        if (card_per_colonna[colonna] < MAX_CARD_DISPLAY) {
            colonne_visuali[colonna][card_per_colonna[colonna]++] = card;
        }
        card = card->successiva;
    }

    // Calcola altezza lavagna
    int max_altezza = 0;
    for (int i = 0; i < NUM_COLONNE; i++) {
        max_altezza = (card_per_colonna[i] > max_altezza) ? card_per_colonna[i] : max_altezza;
    }

    // Stringa per disegnare linea orizzontale
    char linea_lunghezza_colonna[LARGHEZZA_DISPLAY_COLONNA + 1] = {0};
    memset(linea_lunghezza_colonna, '-', LARGHEZZA_DISPLAY_COLONNA);
    // Stringa che contiene l'id della lavagna (porta)
    char nome_lavagna[LARGHEZZA_DISPLAY_COLONNA + 1] = {0};
    sprintf(nome_lavagna, "LAVAGNA %d", PORTA_LAVAGNA);
    // Stringa con i nomi delle colonne
    char nomi_colonne[NUM_COLONNE][LARGHEZZA_DISPLAY_COLONNA + 1];
    for (int col = 0; col < NUM_COLONNE; col++)
        strcpy(nomi_colonne[col], stringhe_colonne[col]);

    // Inizio stampa
    printf("\n");

    // Linea superiore
    printf("+-%s-+-%s-+-%s-+\n", linea_lunghezza_colonna, linea_lunghezza_colonna, linea_lunghezza_colonna);
    
    // Nome della lavagna
    printf("| %-*s |\n", LARGHEZZA_DISPLAY_COLONNA * NUM_COLONNE + 3 * (NUM_COLONNE - 1), nome_lavagna);
    
    // Separatore
    printf("+-%s-+-%s-+-%s-+\n", linea_lunghezza_colonna, linea_lunghezza_colonna, linea_lunghezza_colonna);
    
    // Titoli Colonne
    printf("|");
    for (int col = 0; col < NUM_COLONNE; col++)
        printf(" %-*s |", LARGHEZZA_DISPLAY_COLONNA, nomi_colonne[col]);
    printf("\n");
    
    // Separatore
    printf("+-%s-+-%s-+-%s-+\n", linea_lunghezza_colonna, linea_lunghezza_colonna, linea_lunghezza_colonna);
    
    // Stampa contenuto
    for (int riga = 0; riga < max_altezza; riga++) {
        printf("| ");
        for (int col = 0; col < NUM_COLONNE; col++) {
            struct Card * card = colonne_visuali[col][riga];
            
            // Buffer finale che verrà stampato (fisso a LARGHEZZA_DISPLAY_COLONNA char + terminatore)
            char buffer_cella[LARGHEZZA_DISPLAY_COLONNA + 1]; 
            memset(buffer_cella, 0, sizeof(buffer_cella));

            if (card != NULL) {
                // Crea il testo completo in un buffer temporaneo grande
                char buffer_temp[LUNGHEZZA_TESTO * 2]; // Abbastanza grande per contenere tutto
                if (col == DOING && card->utente != NULL)
                    snprintf(buffer_temp, sizeof(buffer_temp), "[%d] %s (u:%d)", card->id, card->testo, card->utente->porta_utente);
                else
                    snprintf(buffer_temp, sizeof(buffer_temp), "[%d] %s", card->id, card->testo);

                // Controlla se entra nella colonna
                size_t len = strlen(buffer_temp);
                if (len > LARGHEZZA_DISPLAY_COLONNA) {
                    // Se troppo lungo taglia e aggiunge "..."
                    // Copia i primi LARGHEZZA_DISPLAY_COLONNA - 3 caratteri
                    strncpy(buffer_cella, buffer_temp, LARGHEZZA_DISPLAY_COLONNA - 3);
                    buffer_cella[LARGHEZZA_DISPLAY_COLONNA - 3] = '\0'; // Terminatore
                    strcat(buffer_cella, "..."); // Aggiunge i puntini
                } 
                else {
                    // Se entra copia tutto
                    strcpy(buffer_cella, buffer_temp);
                }
            } 
            else {
                // Cella vuota
                buffer_cella[0] = '\0'; 
            }

            // Stampa la cella con padding a destra per allineare la tabella
            // Nota: %-*s assicura che occupi sempre LARGHEZZA_DISPLAY_COLONNA spazi
            printf("%-*s | ", LARGHEZZA_DISPLAY_COLONNA, buffer_cella);
        }
        printf("\n");
    }
    
    // Chiusura tabella
    printf("+-%-*s-+-%-*s-+-%-*s-+\n\n", 
        LARGHEZZA_DISPLAY_COLONNA, "----------------------------------------", 
        LARGHEZZA_DISPLAY_COLONNA, "----------------------------------------", 
        LARGHEZZA_DISPLAY_COLONNA, "----------------------------------------");
}

// Cerca una card in TODO e restituisce il puntatore (o NULL)
struct Card * get_card_todo() {
    struct Card * card = lavagna;
    while (card != NULL) {
        if (card->colonna == TODO && card->utente == NULL) return card;
        card = card->successiva;
    }
    return NULL;
}

// Trova una card per id, la sposta in una nuova colonna e le assegna un utente
void sposta_card(int id, enum Colonne nuova_colonna, struct Utente * nuovo_utente) {
    struct Card * card = lavagna;
    while (card != NULL) {
        if (card->id == id) {
            card->colonna = nuova_colonna;
            card->ultima_modifica = time(NULL);
            card->utente = nuovo_utente;
            break; 
        }
        card = card->successiva;
    }
}

// Genera un'array con tutte le le porte degli utenti attivi (hanno già inviato HELLO)
void get_porte_attive(int * porte_utenti, int * num_utenti, struct Utente * utente_richiedente) {
    *num_utenti = 0;
    struct Utente * utente = lista_utenti;
    while (utente != NULL && *num_utenti < MAX_UTENTI) {
        if (utente->attivo && utente != utente_richiedente) {
            // Invio solo gli utenti diversi da quello richiedente
            porte_utenti[(*num_utenti)++] = utente->porta_utente;
        }
        utente = utente->successivo;
    }
}

// Assegna la prima card libera all'utente, se non è specificato un utente la da all'utente con porta minore
void assegna_card(struct Utente * utente) {
    if (utente == NULL) utente = get_utente_libero_min_porta();
    struct Card * card = get_card_todo();
    if (utente != NULL && card != NULL) {
        card->utente = utente;
        int porte_utenti[MAX_UTENTI];
        int num_utenti;
        get_porte_attive(porte_utenti, &num_utenti, utente);
        // Invia CMD_HANDLE_CARD all'utente che prende in carica la card
        invia_messaggio(utente->socket_utente, CMD_HANDLE_CARD, card, porte_utenti, num_utenti);
    }
}

// Restituisce il primo id per card libero
int get_prossimo_card_id_libero() {
    int id = 1;
    struct Card * card = lavagna;
    while (card != NULL) {
        id = (card->id >= id) ? card->id + 1 : id;
        card = card->successiva;
    }
    return id;
}

// Crea una card e la inserisce nella lista
void crea_card(enum Colonne colonna, char * testo) {
    struct Card * nuova_card = (struct Card *)malloc(sizeof(struct Card));
    if (nuova_card == NULL) {
        perror("Memoria esaurita");
        return;
    }
    memset(nuova_card, 0, sizeof(struct Card));

    nuova_card->id = get_prossimo_card_id_libero();
    nuova_card->colonna = colonna;
    snprintf(nuova_card->testo, LUNGHEZZA_TESTO, "%s", testo);
    nuova_card->ultima_modifica = time(NULL);
    nuova_card->successiva = NULL;

    if (lavagna == NULL) {
        // La lista è vuota
        lavagna = nuova_card;
    }
    else {
        // Viene inserita in fondo alla lista, così da evitare starvation di lavori: coda FCFS (o FIFO)
        struct Card * card = lavagna;
        while (card->successiva != NULL) {
            card = card->successiva;
        }
        card->successiva = nuova_card;
    }
}

// Cerca la card assegnata e resetta l'ultima modifica
void resetta_timer_lavoro_utente(struct Utente * utente) {
    utente->tempo_invio_ping = 0; 
    struct Card * card = lavagna;
    while(card != NULL) {
        if (card->colonna == DOING && card->utente == utente) {
            // Resetta il timer dei TEMPO_PRIMA_DEL_PING secondi
            card->ultima_modifica = time(NULL);
            break;
        }
        card = card->successiva;
    }
}

// Cerca card assegnate a questo utente e le rimette in TODO
void interrompi_lavoro_utente(struct Utente * utente) {
    struct Card * card = lavagna;
    while (card != NULL) {
        if (card->colonna != DONE && card->utente == utente) {
            card->colonna = TODO;
            card->utente = NULL;
            card->ultima_modifica = time(NULL);
        }
        card = card->successiva;
    }
}

// Controlla se un utente sta lavorando su una card (1 = occupato, 0 = libero)
int is_utente_occupato(struct Utente * utente) {
    struct Card * card = lavagna;
    while (card != NULL) {
        if (card->colonna == DOING && card->utente == utente) {
            // L'utente sta lavorando su una card
            return 1;
        }
        card = card->successiva;
    }
    return 0; // Libero
}

// Restituisce il puntatore all'utente libero con numero di porta minore (o NULL)
struct Utente * get_utente_libero_min_porta() {
    struct Utente * candidato = NULL;
    struct Utente * utente = lista_utenti;
    while (utente != NULL) {
        if (utente->attivo && !is_utente_occupato(utente)) {
            // L'utente è libero, se è quello di porta minore diventa il candidato
            candidato = (candidato == NULL || utente->porta_utente < candidato->porta_utente) ? utente : candidato;
        }
        utente = utente->successivo;
    }
    return candidato;
}

// Invia lista utenti a un utente
void invia_lista_utenti(struct Utente * utente) {
    int porte_utenti[MAX_UTENTI];
    int num_utenti;
    get_porte_attive(porte_utenti, &num_utenti, utente);
    // Invia CMD_USER_LIST all'utente giusto
    invia_messaggio(utente->socket_utente, CMD_USER_LIST, NULL, porte_utenti, num_utenti);
}

// Conta quanti utenti sono attualmente nella lista
int conta_utenti_connessi() {
    int count = 0;
    struct Utente * utente = lista_utenti;
    while (utente != NULL) {
        count++;
        utente = utente->successivo;
    }
    return count;
}

// Disattiva un utente
void disattiva_utente(struct Utente * utente) {
    interrompi_lavoro_utente(utente);
    utente->porta_utente = 0;
    utente->attivo = 0;
    utente->tempo_invio_ping = 0;
}

// Attiva un utente 
void attiva_utente(struct Utente * utente, int porta_utente) {
    utente->porta_utente = porta_utente;
    utente->attivo = 1;
}

// Distrugge un utente assicurandosi di aver interrotto i suoi lavori, restituisce il prossimo utente
struct Utente * distruggi_utente(struct Utente * utente, struct Utente * precedente) {
    disattiva_utente(utente);
    close(utente->socket_utente);

    // Elimina l'utente dalla linked list
    if (precedente == NULL) lista_utenti = utente->successivo;
    else precedente->successivo = utente->successivo;

    // Libera la memoria restituendo l'utente successivo
    struct Utente * prossimo = utente->successivo;
    free(utente);
    return prossimo;
}

// Crea un utente e lo inserisce nella lista
void crea_utente(int socket_utente) {
    struct Utente * nuovo_utente = malloc(sizeof(struct Utente));
    memset(nuovo_utente, 0, sizeof(struct Utente));

    nuovo_utente->socket_utente = socket_utente;
    nuovo_utente->attivo = 0;
    nuovo_utente->tempo_invio_ping = 0;
    
    //Inserisce l'utente nella linked list: inserimento a fronte, la lista non è ordinata
    nuovo_utente->successivo = lista_utenti;
    lista_utenti = nuovo_utente;
}