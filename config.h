#ifndef CONFIG_H
#define CONFIG_H

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

#define PUBLIC "/tmp/PUBLIC" // Public FIFO for communication between teams and parent
#define B_SIZ (PIPE_BUF / 2)

// Number of players in each team
#define NUM_PLAYERS 5

// Minimum and maximum initial energy levels for players
#define MIN_PLAYER_ENERGY 50
#define MAX_PLAYER_ENERGY 100

// Minimum and maximum initial energy levels for team leads
#define MIN_LEADER_ENERGY 100
#define MAX_LEADER_ENERGY 150

// Time for a round in seconds
#define ROUND_TIME 180 // 3 minutes

// Maximum number of rounds a team can lose before the game ends
#define MAX_LOST_ROUNDS 3

// Energy cost for throwing a ball
#define THROW_ENERGY_COST 10

// Energy cost for picking up a dropped ball
#define PICKUP_ENERGY_COST 5

// Constants for game settings
#define NUM_ROUNDS 8

struct Player
{
    int id;
    int energy;
    int has_ball;
    char team_name;
};

// struct message
// {
//     char fifo_name[B_SIZ];
//     char message[B_SIZ];
// };

// define all the functions that are used in the parent and player processes
void initialize();                                                                          // initialize the game
int generate_energy(int type);                                                              // generate the initial energy level for a player
void initialize_player(struct Player *player, int id, int energy, int has_ball, char team); // initialize a player
void print_player(struct Player player);                                                    // print the player's information
void kill_all_childs();                                                                     // kill all the child processes
void player_quit_handler(int signum);                                                       // handler for the SIGCHLD signal
void quit_handler(int signum);                                                              // handler for the SIGQUIT signal
void start_round();
void reset_players_status();
void end_round();

#endif // CONFIG_H
