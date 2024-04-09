CC = gcc
CFLAGS = -Wall -DROUND_TIME=$(ROUND_TIME) -DGAME_TIME=$(GAME_TIME)
SRCS_PARENT = parent.c
SRCS_PLAYER = player.c
OBJS_PARENT = $(SRCS_PARENT:.c=.o)
OBJS_PLAYER = $(SRCS_PLAYER:.c=.o)

all: parent player

parent: $(OBJS_PARENT)
	$(CC) $(CFLAGS) -o $@ $^

player: $(OBJS_PLAYER)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

run: all
	./parent
	./player

clean:
	rm -f $(OBJS_PARENT) $(OBJS_PLAYER) parent player