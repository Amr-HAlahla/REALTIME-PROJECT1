#include "config.h"

int num_of_balls;
int current_round;
int current_time;
int num_of_lost_rounds_teamA;
int num_of_lost_rounds_teamB;

// create a public fifo
void create_public_fifo()
{
    if (access(PUBLIC, F_OK) == 0)
    {
        if (unlink(PUBLIC) == -1)
        {
            perror("Error");
            exit(-1);
        }
    }

    if (mkfifo(PUBLIC, 0666) == -1)
    {
        perror("Error");
        exit(-1);
    }
}

void initialize()
{
    num_of_balls = 0;
    current_round = 0;
    current_time = 0;
    num_of_lost_rounds_teamA = 0;
    num_of_lost_rounds_teamB = 0;
}

pid_t players_pids[2 * (NUM_PLAYERS + 1)]; // 5 players + 1 leader for each team = 12
struct Player players[2 * (NUM_PLAYERS + 1)];
int pipe_fd[2];

int main(int argc, char *argv[])
{
    // Initialize the global variables
    initialize();
    create_public_fifo();

    // try open public fifo for reading and writing
    int public_fifo_fd = open(PUBLIC, O_RDWR);
    if (public_fifo_fd == -1)
    {
        perror("open");
        exit(1);
    }

    if (pipe(pipe_fd) == -1)
    {
        perror("pipe");
        exit(1);
    }

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
            char read_fd[10];
            char write_fd[10];
            sprintf(read_fd, "%d", pipe_fd[0]);
            sprintf(write_fd, "%d", pipe_fd[1]);
            execl("./player", "player", read_fd, write_fd, NULL);
            perror("execl");
            exit(1);
        }
        else
        {
            // parent process
            initialize_player(&players[i], i);
            // write the player to the pipe
            write_player_to_pipe(pipe_fd[1], &players[i]);
            // save the pid of the player and set the player in the players array
            players_pids[i] = pid;
        }
    }

    if (sigset(SIGUSR1, usr1_handler) == -1)
    {
        perror("sigset");
        exit(1);
    }

    if (sigset(SIGQUIT, quit_handler) == -1)
    {
        perror("sigset");
        exit(1);
    }

    sleep(2);
    // write the players to the public fifo in order to make functions file store them
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        write(public_fifo_fd, &players[i], sizeof(struct Player));
        printf("Writing player %d to the public fifo\n", i);
    }

    read_players_from_fifo(public_fifo_fd);
    // also write the pids, each one alone
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        write(public_fifo_fd, &players_pids[i], sizeof(pid_t));
        printf("Writing pid %d of the player %d to the public fifo\n", players_pids[i], i);
    }

    read_pids_from_fifo(public_fifo_fd);

    while (1)
    {
        pause();
    }
    // wait for all the player processes to finish
    // for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    // {
    //     wait(NULL);
    // }

    printf("Parent process is done\n");

    return 0;
}

void quit_handler(int signum)
{
    printf("Parent is going to kill players!\n");
    kill_all_childs();
    sleep(2);
    printf("Parent is going to exit!\n");
    exit(0);
}

void kill_all_childs()
{
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        kill(players_pids[i], SIGQUIT);
    }
}

void start_round()
{
    if (current_round != 0)
    {
        for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
        {
            reset_players_status(&players[i]);
        }
    }
    // start the round
    printf("Round %d has started\n", current_round);
    // wake up all the players using usr1 signal
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        // printf("Sending SIGUSR1 to player %d\n", players_pids[i]);
        sleep(1);
        kill(players_pids[i], SIGUSR1);
    }

    printf("Waiting for round to finish\n");
    sleep(ROUND_TIME);

    printf("Reading updated player structs from pipe\n");
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        printf("Reading player %d\n", i);
        read_player_from_pipe(pipe_fd[0], &players[i]);
    }

    printf("Finished reading player structs from pipe\n");

    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        printf("Printing players after round %d\n", current_round);
        print_player(players[i]);
    }
}

void end_round()
{
    printf("Round %d has ended\n", current_round);
    current_round++;
    // print players status after the round ends
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        print_player(players[i]);
    }

    // send stop signals to all the players
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        kill(players_pids[i], SIGSTOP);
    }
}

void usr1_handler(int signum)
{
    printf("Parent received the signal to start the round\n");
    start_round();
}