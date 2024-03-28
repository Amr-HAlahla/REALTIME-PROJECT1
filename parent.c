#include "config.h"

int num_of_balls;
int current_round;
int current_time;
int num_of_lost_rounds_teamA;
int num_of_lost_rounds_teamB;

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

int main(int argc, char *argv[])
{
    // Initialize the global variables
    initialize();

    int pipe_fd[2];
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
            if (write(pipe_fd[1], &players[i], sizeof(struct Player)) == -1)
            {
                perror("writing player to pipe: ");
                exit(1);
            }
            // save the pid of the player and set the player in the players array
            players_pids[i] = pid;
            set_player(&players[i]);
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

    // wait for all the player processes to finish
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        wait(NULL);
    }

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
    // start the round
    printf("Round %d has started\n", current_round);
    // wake up all the players using usr1 signal
    for (int i = 0; i < 2 * (NUM_PLAYERS + 1); i++)
    {
        sleep(1);
        kill(players_pids[i], SIGUSR1);
    }
    sleep(ROUND_TIME);
    printf("Round %d has ended\n", current_round);
    current_round++;
}

void usr1_handler(int signum)
{
    printf("Parent received the signal to start the round\n");
    start_round();
}