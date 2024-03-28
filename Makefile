CC = gcc
CFLAGS = -Wall
SRCS_PARENT = parent.c player_functions.c
SRCS_PLAYER = player.c player_functions.c
OBJS_PARENT = $(SRCS_PARENT:.c=.o)
OBJS_PLAYER = $(SRCS_PLAYER:.c=.o)

all: parent player

parent: $(OBJS_PARENT)
	$(CC) $(CFLAGS) -o $@ $^

player: $(OBJS_PLAYER)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS_PARENT) $(OBJS_PLAYER) parent player
