#include "config.h"

void player_signals_handler(int signum);
void set_player_signals();
void print_player_info();
void leader_receive_ball();
void player_receive_ball();

struct Player current_player;
int pause_flag = 0;
struct Player copy_player;

int main(int argc, char *argv[])
{
    // int i = 0;
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
void player_receive_ball()
{
    if (pause_flag == 0)
    {
        printf("Player pid %d and id %d has received the ball\n", getpid(), current_player.id);
        current_player.has_ball++;
        if (current_player.team_name == 'A')
        {
            // sleep for 0.5 seconds
            usleep(500000);
        }
        else if (current_player.team_name == 'B')
        {
            // sleep for 1 second
            usleep(1000000);
        }
        if (pause_flag == 0)
        {
            // now the player should throw the ball to the next player
            if (current_player.energy >= THROW_ENERGY_COST)
            {
                current_player.energy -= THROW_ENERGY_COST;
                current_player.has_ball--;
                // send signal to the next player to receive the ball
                if (current_player.id == 10 || current_player.id == 4) // last player in the team
                {
                    // team leader can recognize that the ball is thrown from the last player
                    kill(current_player.next_player_pid, SIGINT);
                    printf("Player %d has thrown the ball to the leader player %d\n", current_player.id, current_player.next_player_pid);
                }
                else
                {
                    kill(current_player.next_player_pid, SIGUSR2);
                    printf("Player %d has thrown the ball to player %d\n", current_player.id, current_player.next_player_pid);
                }
            }
            else
            {
                printf("Player %d does not have enough energy (%d) to throw the ball\n", current_player.id, current_player.energy);
            }
        }
    }
}

void leader_receive_ball()
{
    if (pause_flag == 0)
    {
        // this the leader, and he should throw the ball the leader of the other team
        printf("Leader Player %d has received the ball\n", current_player.id);
        if (current_player.id != 5 && current_player.id != 11)
        {
            printf("Player %d is not the leader of the team\n", current_player.id);
            exit(1);
        }
        else
        {
            current_player.has_ball++;
            if (current_player.team_name == 'A')
            {
                // sleep for 0.5 seconds
                usleep(500000);
            }
            else if (current_player.team_name == 'B')
            {
                // sleep for 1 second
                usleep(1000000);
            }
            if (pause_flag == 0)
            {
                // now the leader should throw the ball to the leader of the other team
                if (current_player.energy >= THROW_ENERGY_COST)
                {
                    current_player.energy -= THROW_ENERGY_COST;
                    current_player.has_ball--;
                    // send signal to the leader of other team to receive the ball
                    kill(current_player.team_leader_pid, SIGUSR2);
                    printf("Player %d has thrown the ball to the leader of the other team\n", current_player.id);
                    // after leader throws the ball, ask parent for a new ball.
                    kill(getppid(), SIGUSR2);
                    // write the team name to the public fifo
                    int write_fd = open(PUBLIC, O_WRONLY);
                    if (write_fd == -1)
                    {
                        perror("Error opening public fifo for writing");
                        exit(1);
                    }
                    if (write(write_fd, &current_player.team_name, sizeof(char)) == -1)
                    {
                        perror("Error writing to public fifo");
                        exit(1);
                    }
                    close(write_fd);
                }
                else
                {
                    printf("Leader Player %d does not have enough energy (%d) to throw the ball\n", current_player.id, current_player.energy);
                }
            }
        }
    }
}

void player_signals_handler(int signum)
{
    if (signum == SIGUSR1) // handle recevie ball
    {
        // open private fifo for reading
        char private_fifo[20];
        sprintf(private_fifo, "/tmp/fifo%d", getpid());
        int read_fd = open(private_fifo, O_RDONLY);
        if (read_fd == -1)
        {
            perror("Error opening private fifo for reading");
            exit(1);
        }
        // printf("Player %d opened private fifo for reading\n", current_player.id);
        if (read(read_fd, &current_player, sizeof(struct Player)) == -1)
        {
            perror("Error reading from private fifo");
            exit(1);
        }
        // printf("Reading player of id %d and pid %d to private fifo %s\n", current_player.id, getpid(), private_fifo);
        close(read_fd);
        // print current player info
        // print_player_info();
    }
    else if (signum == SIGUSR2)
    {
        player_receive_ball();
    }
    else if (signum == SIGINT)
    {
        // call function to handle the signal
        leader_receive_ball();
    }
    // handle stop signal (will be sent from the parent when the round ends)
    else if (signum == SIGTSTP)
    {
        // flip the pause flag value
        pause_flag = !pause_flag;
    }

    else if (signum == SIGQUIT)
    {
        char private_fifo[20];
        sprintf(private_fifo, "/tmp/fifo%d", getpid());
        int write_fd = open(private_fifo, O_WRONLY);
        if (write_fd == -1)
        {
            perror("Error opening private fifo for writing");
            exit(1);
        }
        if (write(write_fd, &copy_player, sizeof(struct Player)) == -1)
        {
            perror("Error writing to private fifo");
            exit(1);
        }
        close(write_fd);
    }
}

void set_player_signals()
{

    struct sigaction sa;
    sa.sa_handler = player_signals_handler;
    sa.sa_flags = SA_RESTART; // restart the system call if at all possible
    sigemptyset(&sa.sa_mask); // initialize the signal set
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("Error setting signal handler for SIGUSR1");
        exit(1);
    }

    struct sigaction sa2;
    sa2.sa_handler = player_signals_handler;
    sa2.sa_flags = SA_RESTART;
    sigemptyset(&sa2.sa_mask);
    if (sigaction(SIGUSR2, &sa2, NULL) == -1)
    {
        perror("Error setting signal handler for SIGUSR2");
        exit(1);
    }

    struct sigaction sa3;
    sa3.sa_handler = player_signals_handler;
    sa3.sa_flags = SA_RESTART;
    sigemptyset(&sa3.sa_mask);
    if (sigaction(SIGINT, &sa3, NULL) == -1)
    {
        perror("Error setting signal handler for SIGINT");
        exit(1);
    }

    struct sigaction sa4;
    sa4.sa_handler = player_signals_handler;
    sa4.sa_flags = SA_RESTART;
    sigemptyset(&sa4.sa_mask);
    if (sigaction(SIGTSTP, &sa4, NULL) == -1)
    {
        perror("Error setting signal handler for SIGTSTP");
        exit(1);
    }

    struct sigaction sa5;
    sa5.sa_handler = player_signals_handler;
    sa5.sa_flags = SA_RESTART;
    sigemptyset(&sa5.sa_mask);
    if (sigaction(SIGQUIT, &sa5, NULL) == -1)
    {
        perror("Error setting signal handler for SIGQUIT");
        exit(1);
    }
}

void print_player_info()
{
    printf("Player %d from team %c has %d energy and next player is %d and team leader is %d and has %d balls\n",
           current_player.id, current_player.team_name, current_player.energy, current_player.next_player_pid, current_player.team_leader_pid, current_player.has_ball);
}