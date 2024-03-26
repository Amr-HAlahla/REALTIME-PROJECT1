#include "config.h"

int num_of_balls;
int current_round;
int current_time;
int num_of_lost_rounds_teamA;
int num_of_lost_rounds_teamB;

pid_t players_pids[2 * (NUM_PLAYERS + 1)];    // 5 players + 1 leader for each team = 12
struct Player players[2 * (NUM_PLAYERS + 1)]; // 5 players + 1 leader for each team = 12

int main(int argc, char *argv[])
{

    // initialize array of players
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        players[i].id = -1;
        players[i].energy = -1;
        players[i].has_ball = -1;
        players[i].team_name = ' ';
    }
    // Initialize the global variables
    initialize();

    // // Pipes for communication with player processes
    // int pipe_teamA[NUM_PLAYERS][2];
    // int pipe_teamB[NUM_PLAYERS][2];

    // // Array to store player process IDs
    // pid_t player_pids_teamA[NUM_PLAYERS];
    // pid_t player_pids_teamB[NUM_PLAYERS];

    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        sleep(1); // to make sure that the players are created in order
        pid_t pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(1);
        }

        if (pid == 0)
        {
            // prepare the arguments for the player process (id, energy, has_ball, pipe to read and write, team)
            char id[5];
            sprintf(id, "%d", i);
            char energy[2];
            // type of the player (leader if i == 5 or 11) => 0 normal player, 1 leader
            int type = (i == 5 || i == 11) ? 1 : 0;
            sprintf(energy, "%d", generate_energy(type));
            // has_ball => initially no player has the ball
            char has_ball[2];
            sprintf(has_ball, "%d", 0);
            // team => either A or B
            char team[2];
            sprintf(team, "%c", (i < 6) ? 'A' : 'B');

            players[i].id = i;
            players[i].energy = atoi(energy);
            players[i].has_ball = atoi(has_ball);
            players[i].team_name = team[0];

            // execute the player process
            // printf("Player %d from team %c has %d energy and %s the ball\n", i, team[0], atoi(energy), atoi(has_ball) ? "has" : "does not have");
            printf("going to execute player process with id %d\n", i);
            execl("./player", "player", id, energy, has_ball, team, NULL);
            perror("execl");
            exit(1);
        }
        else
        {
            players_pids[i] = pid;
        }
    }

    struct sigaction act;
    act.sa_handler = quit_handler;
    act.sa_flags = SA_RESTART;
    sigemptyset(&act.sa_mask);

    if (sigaction(SIGQUIT, &act, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    // wait for all the player processes to finish
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        wait(NULL);
    }

    // sleep(10); // wait for 10 seconds untill all the players are created
    // // kill all the child processes
    // kill_all_childs();

    printf("Parent process is done\n");

    return 0;
}

void initialize()
{
    num_of_balls = 0;
    current_round = 0;
    current_time = 0;
    num_of_lost_rounds_teamA = 0;
    num_of_lost_rounds_teamB = 0;

    // seed the random number generator
    // srand(time(NULL));

    // // create the public FIFO, first remove it if it already exists
    // unlink(PUBLIC);
    // if (mkfifo(PUBLIC, 0666) == -1)
    // {
    //     perror("mkfifo");
    //     exit(1);
    // }
}

void quit_handler(int signum)
{
    printf("Parent is going to kill players!\n");
    kill_all_childs();
    sleep(2);
    printf("Parent is going to exit!\n");
    exit(0);
}

int generate_energy(int type)
{
    int energy;
    srand(time(NULL));
    // type 0 => normal player, type 1 => leader (diff in defined energy ranges)
    if (type == 0)
    {
        energy = (rand() % (MAX_PLAYER_ENERGY - MIN_PLAYER_ENERGY + 1)) + MIN_PLAYER_ENERGY;
    }
    else
    {
        energy = (rand() % (MAX_LEADER_ENERGY - MIN_LEADER_ENERGY + 1)) + MIN_LEADER_ENERGY;
    }
    return energy;
}

void kill_all_childs()
{
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        kill(players_pids[i], SIGQUIT);
    }
}