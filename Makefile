CC = gcc
CFLAGS = -Wall
SRCS_PARENT = parent.c
SRCS_PLAYER = player.c
SRCS_GUI = gui.c
OBJS_PARENT = $(SRCS_PARENT:.c=.o)
OBJS_PLAYER = $(SRCS_PLAYER:.c=.o)
OBJS_GUI = $(SRCS_GUI:.c=.o)

# Default values for ROUND_TIME and GAME_TIME
ROUND_TIME_DEFAULT = 45
GAME_TIME_DEFAULT = 180

# If ROUND_TIME and GAME_TIME are not defined, use default values
ROUND_TIME ?= $(ROUND_TIME_DEFAULT)
GAME_TIME ?= $(GAME_TIME_DEFAULT)

ifdef GUI
CFLAGS += -DGUI
endif

all: player parent gui

parent: $(OBJS_PARENT)
	$(CC) $(CFLAGS) -DROUND_TIME=$(ROUND_TIME) -DGAME_TIME=$(GAME_TIME) -o $@ $^

player: $(OBJS_PLAYER)
	$(CC) $(CFLAGS) -o $@ $^

gui: $(OBJS_GUI)
	$(CC) $(CFLAGS) -o $@ $^ -lglut -lGLU -lGL -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $<

run: all
	./parent

clean:
	rm -f parent player gui $(OBJS_PARENT) $(OBJS_PLAYER) $(OBJS_GUI)
