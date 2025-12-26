# Compilatore e flag
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Target di default 
all: lavagna utente

# Compilazione Lavagna
lavagna: lavagna.c common.h
	$(CC) $(CFLAGS) -o lavagna lavagna.c

# Compilazione Utente
utente: utente.c common.h
	$(CC) $(CFLAGS) -o utente utente.c

# Pulizia dei file compilati
clean:
	rm -f lavagna utente

# Phony targets per evitare conflitti con file che si chiamano "clean" o "all"
.PHONY: all clean