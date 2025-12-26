# Compilatore e flag
CC = gcc
CFLAGS = -Wall -Wextra -O2

# Target di default 
all: lavagna utente

# Compilazione Lavagna
lavagna: lavagna.c include/common.h include/include_lavagna.h include/protocollo_lavagna.h
	$(CC) $(CFLAGS) -o lavagna lavagna.c include/protocollo_lavagna.c

# Compilazione Utente
utente: utente.c include/common.h include/include_utente.h include/protocollo_utente.h
	$(CC) $(CFLAGS) -o utente utente.c include/protocollo_utente.c

# Pulizia dei file compilati
clean:
	rm -f lavagna utente

# Phony targets per evitare conflitti con file che si chiamano "clean" o "all"
.PHONY: all clean