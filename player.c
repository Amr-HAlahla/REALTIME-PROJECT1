#include "config.h"

void player_signals_handler(int signum);
void set_player_signals();
struct Player current_player;

int main(int argc, char *argv[])
{
    int i = 0;
    if (argc != 1)
    {
        fprintf(stderr, "Usage: %s No arguments needed\n", argv[0]);
        exit(1);
    }

    set_player_signals();
    while (1)
    {
        pause();
    }

    printf("Player %d has been killed\n", current_player.id);
    return 0;
}

void set_player_signals()
{
    if (sigset(SIGUSR1, player_signals_handler) == SIG_ERR)
    {
        perror("SIGUSR1");
        exit(1);
    }
    if (sigset(SIGUSR2, player_signals_handler) == SIG_ERR)
    {
        perror("SIGUSR2");
        exit(1);
    }
    if (sigset(SIGTERM, player_signals_handler) == SIG_ERR)
    {
        perror("SIGTERM");
        exit(1);
    }
}

void player_signals_handler(int signum)
{
    if (signum == SIGUSR1) // handle recevie ball
    {
        printf("Player has received the signal to read its info from the public fifo\n");
        int read_fd = open(PUBLIC, O_RDONLY);
        if (read_fd == -1)
        {
            perror("Error opening public fifo for reading");
            exit(1);
        }
        // printf("Player %d opened public fifo for reading\n", current_player.id);
        if (read(read_fd, &current_player, sizeof(struct Player)) == -1)
        {
            perror("Error reading from public fifo");
            exit(1);
        }
        // printf("Player %d read from public fifo\n", current_player.id);
        close(read_fd);
        sleep(1);
        // print current player info
        // printf("Player %d from team %c has %d energy and %s the ball\n", current_player.id, current_player.team_name, current_player.energy, current_player.has_ball ? "has" : "does not have");
    }
    else if (signum == SIGUSR2)
    {
        // printf("Player %d has received USR2 signal\n", current_player.id);
        printf("Player %d from team %c has %d energy and %s the ball and next player is %d and team leader is %d\n",
               current_player.id, current_player.team_name, current_player.energy, current_player.has_ball ? "has" : "does not have", current_player.next_player_pid, current_player.team_leader_pid);
        fflush(stdout);
        // exit(0);
    }
    else if (signum == SIGTERM)
    {
        printf("Player %d has been killed\n", current_player.id);
        exit(0);
    }
}