CC = gcc
CFLAGS = -Wall -g

SRC = main.c asyncServer.c http.c utils.c
OBJ = $(SRC:.c=.o)

all: asyncserver

asyncserver: $(OBJ)
	$(CC) $(CFLAGS) -o server $(OBJ)

clean:
	rm