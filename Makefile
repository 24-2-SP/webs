CC = gcc
CFLAGS = -Wall -g
LDFLAGS = 

# Source Files
SRC = reverse_proxy.c cache.c
OBJ = $(SRC:.c=.o)
EXEC = reverse_proxy

# Targets
all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJ) $(EXEC)

run:
	./$(EXEC)

