#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "common.h"
#include "gestione_lavagna.h"
#include "protocollo_lavagna.h"

// Stato globale della lavagna
extern struct Card * lavagna;
extern struct Utente * lista_utenti;

// Stampa a video la lavagna
void show_lavagna() {
    // Strutture temporanee per organizzare la visualizzazione
    struct Card * colonne_visuali[NUM_COLONNE][MAX_CARD_DISPLAY];
    uint16_t card_per_colonna[NUM_COLONNE] = {0};
    memset(colonne_visuali, 0, sizeof(colonne_visuali));

    // Scorriamo la lista una sola volta
    struct Card * card = lavagna;
    while (card != NULL) {
        enum Colonne colonna = card->colonna;
        if (card_per_colonna[colonna] < MAX_CARD_DISPLAY) {
            colonne_visuali[colonna][card_per_colonna[colonna]++] = card;
        }
        card = card->successiva;
    }

    // Calcolo altezza lavagna
    uint16_t max_altezza = 0;
    for (uint16_t i = 0; i < NUM_COLONNE; i++) {
        max_altezza = (card_per_colonna[i] > max_altezza) ? card_per_colonna[i] : max_altezza;
    }

    // Inizio stampa
    printf("\n");
    // Linea superiore
    printf("+-%-*s-+-%-*s-+-%-*s-+\n", 
        LARGHEZZA_DISPLAY_COLONNA, "----------------------------------------", 
        LARGHEZZA_DISPLAY_COLONNA, "----------------------------------------", 
        LARGHEZZA_DISPLAY_COLONNA, "----------------------------------------");
    
    // Titoli Colonne
    printf("| %-*s | %-*s | %-*s |\n", 
        LARGHEZZA_DISPLAY_COLONNA, "TO DO", 
        LARGHEZZA_DISPLAY_COLONNA, "DOING", 
        LARGHEZZA_DISPLAY_COLONNA, "DONE");
    
    // Separatore
    printf("+-%-*s-+-%-*s-+-%-*s-+\n", 
        LARGHEZZA_DISPLAY_COLONNA, "----------------------------------------", 
        LARGHEZZA_DISPLAY_COLONNA, "----------------------------------------", 
        LARGHEZZA_DISPLAY_COLONNA, "----------------------------------------");

    // Stampa contenuto
    for (int riga = 0; riga < max_altezza; riga++) {
        printf("| ");
        for (int col = 0; col < NUM_COLONNE; col++) {
            struct Card * card = colonne_visuali[col][riga];
            char buffer[LARGHEZZA_DISPLAY_COLONNA + 10]; // Buffer per formattare la singola cella

            if (card != NULL) { // Se c'è una card, formattiamo l'output
                if (col == DOING && card->utente != NULL) // Per DOING mostriamo anche la porta dell'utente
                    snprintf(buffer, LARGHEZZA_DISPLAY_COLONNA, "[%d] %s (u:%d)", card->id, card->testo, card->utente->porta_utente);
                else // Per TODO e DONE solo ID e Testo
                    snprintf(buffer, LARGHEZZA_DISPLAY_COLONNA, "[%d] %s", card->id, card->testo);
            } 
            else // Cella vuota
                buffer[0] = '\0';

            // Stampa la cella con padding a destra (%-*s) per allineare le barre verticali
            printf("%-*s | ", LARGHEZZA_DISPLAY_COLONNA, buffer);
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

// Sposta una card in una nuova colonna e mostra la nuova lavagna
void sposta_card(uint16_t id, enum Colonne nuova_colonna, struct Utente * nuovo_utente) {
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

// Genera array porte per il protocollo
void get_porte_attive(uint16_t * porte_utenti, uint16_t * num_utenti, struct Utente * utente_richiedente) {
    *num_utenti = 0;
    struct Utente * utente = lista_utenti;
    while (utente != NULL && *num_utenti < MAX_UTENTI) {
        if (utente->attivo && utente != utente_richiedente) {
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
        uint16_t porte_utenti[MAX_UTENTI];
        uint16_t num_utenti;
        get_porte_attive(porte_utenti, &num_utenti, utente);
        invia_messaggio(utente->socket_utente, CMD_HANDLE_CARD, card, porte_utenti, num_utenti);
    }
}

// Restituisce il primo id per card libero
uint16_t get_prossimo_card_id_libero() {
    uint16_t id = 1;
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
        perror("Memoria esaurita: ");
        return;
    }
    memset(nuova_card, 0, sizeof(struct Card));

    nuova_card->id = get_prossimo_card_id_libero();
    nuova_card->colonna = colonna;
    snprintf(nuova_card->testo, LUNGHEZZA_TESTO, "%s", testo);
    nuova_card->ultima_modifica = time(NULL);
    nuova_card->successiva = NULL;

    if (lavagna == NULL)
        lavagna = nuova_card;
    else {
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
            card->ultima_modifica = time(NULL); // Resetta il timer dei 90 secondi
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

// Controlla se un utente sta lavorando su una card
uint16_t is_utente_occupato(struct Utente * utente) {
    struct Card * card = lavagna;
    while (card != NULL) {
        if (card->colonna == DOING && card->utente == utente) {
            return 1; // Occupato
        }
        card = card->successiva;
    }
    return 0; // Libero
}

// Restituisce il puntatore all'utente a cui verrà assegnata la card o NULL
struct Utente * get_utente_libero_min_porta() {
    struct Utente * candidato = NULL;
    struct Utente * utente = lista_utenti;
    while (utente != NULL) {
        if (utente->attivo && !is_utente_occupato(utente)) {
            candidato = (candidato == NULL || utente->porta_utente < candidato->porta_utente) ? utente : candidato;
        }
        utente = utente->successivo;
    }
    return candidato;
}

// Invia lista utenti a un utente
void invia_lista_utenti(struct Utente * utente) {
    uint16_t porte_utenti[MAX_UTENTI];
    uint16_t num_utenti;
    get_porte_attive(porte_utenti, &num_utenti, utente);
    invia_messaggio(utente->socket_utente, CMD_USER_LIST, NULL, porte_utenti, num_utenti);
}

// Disattiva un utente
void disattiva_utente(struct Utente * utente) {
    interrompi_lavoro_utente(utente);
    utente->porta_utente = 0;
    utente->attivo = 0;
    utente->tempo_invio_ping = 0;
}

// Attiva un utente 
void attiva_utente(struct Utente * utente, uint16_t porta_utente) {
    utente->porta_utente = porta_utente;
    utente->attivo = 1;
}

// Distrugge un utente assicurandosi di aver interrotto i suoi lavori
struct Utente * distruggi_utente(struct Utente * utente, struct Utente * precedente) {
    disattiva_utente(utente);
    close(utente->socket_utente);

    if (precedente == NULL) lista_utenti = utente->successivo;
    else precedente->successivo = utente->successivo;

    struct Utente * prossimo = utente->successivo;
    free(utente);
    return prossimo;
}

// Crea un utente e lo inserisce nella lista
void crea_utente(uint32_t socket_utente) {
    struct Utente * nuovo_utente = malloc(sizeof(struct Utente));
    memset(nuovo_utente, 0, sizeof(struct Utente));

    nuovo_utente->socket_utente = socket_utente;
    nuovo_utente->attivo = 0; // Non ancora attivo finché non manda HELLO
    nuovo_utente->tempo_invio_ping = 0;
    
    nuovo_utente->successivo = lista_utenti;
    lista_utenti = nuovo_utente;
}