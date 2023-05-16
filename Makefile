CC = gcc
CFLAGS = `pkg-config --cflags gtk+-3.0 vte-2.91`
LIBS = `pkg-config --libs gtk+-3.0 vte-2.91`

travvyterm: main.c
	$(CC) $(CFLAGS) -o travvyterm main.c $(LIBS)
