#ifndef CONFIG_H
#define CONFIG_H

#include <math.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <wait.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>
#include <asm-generic/signal-defs.h>
#include <bits/sigaction.h>

#define PUBLIC "/tmp/PUBLIC"  // Public FIFO for communication between teams and parent
#define PRIVATE "/tmp/fifo"   // Private FIFO for communication between GUI and players!
#define PRIVATE2 "/tmp/fifo2" // Private FIFO for communication between GUI and Leaders!
#define B_SIZ (PIPE_BUF / 2)

// Number of players in each team
#define NUM_PLAYERS 6

// Minimum and maximum initial energy levels for players
#define MIN_PLAYER_ENERGY 100
#define MAX_PLAYER_ENERGY 150

// Minimum and maximum initial energy levels for team leads
#define MIN_LEADER_ENERGY 150
#define MAX_LEADER_ENERGY 200

// Maximum number of rounds a team can lose before the game ends
#define MAX_LOST_ROUNDS 3

// Energy cost for throwing a ball
#define THROW_ENERGY_COST 10

// Energy cost for picking up a dropped ball
#define PICKUP_ENERGY_COST 5

// Constants for game settings
#define NUM_ROUNDS 4 // 4 rounds

// // Define a constant for the number of balls
#define NUM_OF_BALLS 15

struct Player
{
    int id;
    int energy;
    int has_ball;
    char team_name;
    pid_t next_player_pid;
    pid_t team_leader_pid;
    pid_t gui_pid;
};

struct GUI_Player
{
    float x, y;
    int id;
};

typedef struct
{
    float x, y; // for gui position
    int last_player_id;
    // Add a boolean flag to indicate whether the ball is valid or not
    int valid; // 0 invalid, 1 valid
    /*Boolean flag to indeicate of the ball have passed through a leader*/
    // int visited; // 0 not visited, 1 visited
} Ball;

typedef struct
{
    // define array of balls of unknown size
    Ball balls[NUM_OF_BALLS];
} GameState;

typedef struct
{
    pid_t current_pid;
    pid_t next_pid;
} message;

#endif // CONFIG_H

/*make GUI=1*/