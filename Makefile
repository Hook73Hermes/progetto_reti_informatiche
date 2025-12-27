# Compilatore e flag
CC = gcc
CFLAGS = -Wall

# Target di default 
all: lavagna utente

# Compilazione lavagna
lavagna: lavagna.c include/common.h include/*_lavagna.[hc]
	$(CC) $(CFLAGS) -o lavagna lavagna.c include/common.h include/*_lavagna.c

# Compilazione utente
utente: utente.c include/common.h include/*_utente.[hc]
	$(CC) $(CFLAGS) -o utente utente.c include/common.h include/*_utente.c

# Pulizia dei file compilati
clean:
	rm -f lavagna utente

# Phony targets per evitare conflitti con file che si chiamano "clean" o "all"
.PHONY: all clean