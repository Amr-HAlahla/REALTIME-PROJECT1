#include "config.h"

void player_signals_handler(int signum);
void set_player_signals();
void print_player_info();
void leader_receive_ball();
void player_receive_ball();
void handle_low_energy();
void player_throw_ball();
void leader_throw_ball();

struct Player current_player;
int pause_flag = 0;
int energy_flag = 0;

int last_player_id = 0;
int next_player_id = 0;
int ball_id = 0;

int main(int argc, char *argv[])
{
    srand(time(NULL) + getpid() + current_player.energy);
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
        // Generate a sleep time between 0.5 and 2 seconds
        int energy_level = current_player.energy;
        double sleep_time;

        if (current_player.id == 5 || current_player.id == 11)
        {
            // This is a leader player
            sleep_time = 2 - ((energy_level - MIN_LEADER_ENERGY) /
                              (double)(MAX_LEADER_ENERGY - MIN_LEADER_ENERGY) * 1.5);
        }
        else
        {
            // This is a normal player
            sleep_time = 2 - ((energy_level - MIN_PLAYER_ENERGY) /
                              (double)(MAX_PLAYER_ENERGY - MIN_PLAYER_ENERGY) * 1.5);
        }
        sleep_time = fabs(sleep_time); // get the absolute value
        // printf("Sleep time for player %d: %f seconds\n", current_player.id, sleep_time);
        sleep_time = sleep_time < 0.5 ? 0.5 : (sleep_time > 2 ? 2 : sleep_time);
        if (sleep_time < 0)
        {
            printf("Player %d has negative sleep time\n", current_player.id);
            exit(1);
        }

        usleep((unsigned int)(sleep_time * 1000000));
        if (pause_flag == 0)
        {
            // now the player should throw the ball to the next player
            if (current_player.energy >= THROW_ENERGY_COST)
            {
                player_throw_ball();
            }
            else
            {
                printf("Player %d does not have enough energy (%d) to throw the ball\n", current_player.id, current_player.energy);
                energy_flag = 1;
                while (energy_flag && pause_flag == 0)
                {
                    handle_low_energy();
                }
                if (pause_flag == 0)
                {
                    player_throw_ball();
                }
            }
        }
    }
}

void player_throw_ball()
{
    if (pause_flag == 0)
    {
        current_player.energy -= THROW_ENERGY_COST;
        current_player.has_ball--;
        // send signal to the next player to receive the ball
        if (current_player.id == 10 || current_player.id == 4) // last player in the team
        {
            // team leader can recognize that the ball is thrown from the last player
            kill(current_player.next_player_pid, SIGINT);
            printf("Player %d has thrown the ball to the leader player %d, and have energy %d\n",
                   current_player.id, current_player.next_player_pid, current_player.energy);
        }
        else
        {
            kill(current_player.next_player_pid, SIGUSR2);
            printf("Player %d has thrown the ball to player %d, and have energy %d\n",
                   current_player.id, current_player.next_player_pid, current_player.energy);
        }
    }
}

void leader_throw_ball()
{
    if (pause_flag == 0)
    {
        current_player.energy -= THROW_ENERGY_COST;
        current_player.has_ball--;
        // send signal to the leader of other team to receive the ball
        kill(current_player.team_leader_pid, SIGUSR2);
        printf("Player %d has thrown the ball to the leader of the other team, and have energy %d\n",
               current_player.id, current_player.energy);
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
            int energy_level = current_player.energy;
            double sleep_time;
            sleep_time = 2 - ((energy_level - MIN_LEADER_ENERGY) /
                              (double)(MAX_LEADER_ENERGY - MIN_LEADER_ENERGY) * 1.5);
            sleep_time = fabs(sleep_time); // make sure the sleep time is positive
            // printf("Sleep time for leader player %d: %f seconds\n", current_player.id, sleep_time);
            sleep_time = sleep_time < 0.5 ? 0.5 : (sleep_time > 2 ? 2 : sleep_time);
            if (sleep_time < 0)
            {
                printf("Player %d has negative sleep time\n", current_player.id);
                exit(1);
            }
            usleep((unsigned int)(sleep_time * 1000000));

            if (pause_flag == 0)
            {
                // now the leader should throw the ball to the leader of the other team
                if (current_player.energy >= THROW_ENERGY_COST)
                {
                    leader_throw_ball();
                }
                else
                {
                    energy_flag = 1; // energy is not enough
                    printf("Leader Player %d does not have enough energy (%d) to throw the ball\n", current_player.id, current_player.energy);
                    while (energy_flag && pause_flag == 0)
                    {
                        handle_low_energy();
                    }
                    if (pause_flag == 0)
                    {
                        leader_throw_ball();
                    }
                }
            }
        }
    }
}

void handle_low_energy()
{
    /*
    if the player does not have enough energy to throw the ball
    he should sleep short time between 0.2 and 1 seconds to recharge energy
    energy will be recharged by range between THROW_ENERGY_COST/2 and 2*THROW_ENERGY_COST
    */
    if (pause_flag == 0)
    {
        int energy_level = current_player.energy;
        printf("Player %d is recharging energy\n", current_player.id);
        double sleep_time = (rand() % 8 + 2) / 10.0; // generate a random sleep time between 0.2 and 1 seconds
        usleep((unsigned int)(sleep_time * 1000000));
        // recharge energy by a random value between THROW_ENERGY_COST/2 and 2*THROW_ENERGY_COST
        int range = 3 * THROW_ENERGY_COST / 2;
        int lower_bound = THROW_ENERGY_COST / 2;
        current_player.energy += (rand() % range) + lower_bound;
        if (current_player.energy >= THROW_ENERGY_COST) // if the energy is enough now to throw the ball
        {
            energy_flag = 0; // exit the loop (No problems with energy level)
        }
        else
        {
            energy_flag = 1; // continue the loop
        }
        // print how much energy have been recharged, and the new energy level
        printf("Player %d has slept for %0.3f Seconds, recharged %d energy, and now has %d energy\n",
               current_player.id, sleep_time, current_player.energy - energy_level, current_player.energy);
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
        close(read_fd);
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
        // don't do anything
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