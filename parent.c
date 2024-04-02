#include "config.h"

typedef void (*sighandler_t)(int);

sighandler_t sigset(int sig, sighandler_t disp);

int num_of_balls = 0;
int current_round = 0;
int num_of_lost_rounds_teamA = 0;
int num_of_lost_rounds_teamB = 0;
int num_of_balls_teamA = 0;
int num_of_balls_teamB = 0;
int current_balls_teamA = 0;
int current_balls_teamB = 0;

void create_public_fifo();
void initialize_player(int id, pid_t next_player_pid, pid_t team_leader_pid);
void parent_signals_handler(int signum);
void print_player(struct Player player);
int generate_energy(int type);
void parent_set_signals();
void start_round();
void end_round();

pid_t players_pids[2 * NUM_PLAYERS];
struct Player players[2 * NUM_PLAYERS]; // 6 players each team. 12 players in total
pid_t parent_pgid;

// int public_fifo_fd;
pid_t pid;
pid_t next_player_pid;
pid_t team_leader_pid;

int running = 1;

int main(int argc, char *argv[])
{

    if (argc != 1)
    {
        fprintf(stderr, "Usage: %s No arguments needed\n", argv[0]);
        exit(1);
    }
    pid_t parent_pgid = getpgrp();
    int i = 2 * NUM_PLAYERS - 1; // 11
    for (; i > -1; i--)
    {
        sleep(1); // to make sure that the players are created in order
        pid = fork();
        if (pid == -1)
        {
            perror("fork");
            exit(1);
        }

        if (pid == 0)
        {
            if (setpgid(0, parent_pgid) == -1)
            {
                perror("setpgid");
                exit(1);
            }
            execl("./player", "player", NULL);
            perror("execl");
            exit(1);
        }
        else
        {
            // sleep(2);
            players_pids[i] = pid;
            if (i == 11)
            {
                next_player_pid = -1; // supposed to be the pid of player with index 6 (team B first player)
                team_leader_pid = -1; // supposed to be the pid of player with index 5 (team A leader)
            }
            else if (i == 5)
            {
                next_player_pid = -1;               // should be player with id 0 (team A first player)
                team_leader_pid = players_pids[11]; // point to the leader of other team (team B leader)
            }
            else
            {
                next_player_pid = players_pids[i + 1]; // previous created player
                team_leader_pid = -1;
            }
            initialize_player(i, next_player_pid, team_leader_pid);
        }
    }

    create_public_fifo();
    parent_set_signals();
    sleep(1);
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        printf("Player %d pid: %d\n", i, players_pids[i]);
    }
    // set next player and team leader for each player
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        if (i == 11) // team B leader
        {
            players[i].next_player_pid = players_pids[6]; // first player in team B
            players[i].team_leader_pid = players_pids[5]; // team A leader
        }
        else if (i == 5) // team A leader
        {
            players[i].next_player_pid = players_pids[0];  // first player in team A
            players[i].team_leader_pid = players_pids[11]; // team B leader
        }
        else
        {
            players[i].next_player_pid = players_pids[i + 1]; // next player
            players[i].team_leader_pid = -1;                  // no team leader
        }
    }
    printf("====================================\n");
    printf("Sending signals to the players to receive their info from the private fifo\n");
    printf("====================================\n");
    fflush(stdout);
    // sen signal for each player and then write the player on its private fifo!
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        kill(players_pids[i], SIGUSR1);
        sleep(1);
        char fifo_name[20];
        sprintf(fifo_name, "/tmp/fifo%d", players_pids[i]);
        int private_fifo_fd = open(fifo_name, O_WRONLY);
        if (private_fifo_fd == -1)
        {
            perror("open");
            exit(1);
        }
        if (write(private_fifo_fd, &players[i], sizeof(struct Player)) == -1)
        {
            perror("writing to private fifo");
            exit(1);
        }
        printf("Writing player of id %d and pid %d to private fifo %s\n", players[i].id, players_pids[i], fifo_name);
        close(private_fifo_fd);
        sleep(2);
        fflush(stdout);
    }
    printf("Players written to private fifos\n");
    sleep(1);
    start_round();
    printf("====================================\n");
    while (1)
    {
        printf("Parent process is waiting for a signal\n");
        pause();
    }

    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        wait(NULL);
    }
    printf("Parent process is done\n");

    return 0;
}

void start_round()
{
    // reset variables related to the current round
    num_of_balls_teamA = 0;
    num_of_balls_teamB = 0;
    current_balls_teamA = 0;
    current_balls_teamB = 0;
    num_of_balls = 0;
    // send signals to the leaders of the teams to start the round
    kill(players_pids[5], SIGUSR2); // team A leader
    printf("Sent signal to team A leader with pid %d and id %d\n", players_pids[5], players[5].id);
    current_balls_teamA = 1;
    kill(players_pids[11], SIGUSR2); // team B leader
    printf("Sent signal to team B leader with pid %d and id %d\n", players_pids[11], players[11].id);
    current_balls_teamB = 1;
    printf("Round %d has started\n", current_round);
    // set alarm for the round time
    alarm(ROUND_TIME);
}

void end_round()
{
    printf("Round %d has ended\n", current_round);
    printf("====================================\n");
    // send signals to the players to stop
    printf("Sending signals to the players to stop\n");
    kill(-parent_pgid, SIGTSTP); // send signal to all players to stop at same time
    fflush(stdout);
    sleep(1);
    // now send signals to the players to write their info to the private fifos
    printf("Sending signals to the players to write their info to the private fifos\n");
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        kill(players_pids[i], SIGQUIT);
    }
    // at the end of the round, read the players info from the private fifos
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        char fifo_name[20];
        sprintf(fifo_name, "/tmp/fifo%d", players_pids[i]);
        int read_fd = open(fifo_name, O_RDONLY);
        if (read_fd == -1)
        {
            perror("Error opening private fifo for reading");
            exit(1);
        }
        if (read(read_fd, &players[i], sizeof(struct Player)) == -1)
        {
            perror("Error reading from private fifo");
            exit(1);
        }
        // printf("Reading player of id %d and pid %d to private fifo %s\n", players[i].id, players_pids[i], fifo_name);
        close(read_fd);
    }
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        print_player(players[i]);
    }

    // check who won that round and update the lost rounds for each team
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        if (players[i].team_name == 'A')
        {
            if (players[i].has_ball)
            {
                num_of_balls_teamA += players[i].has_ball; // add the number of balls the player has
            }
        }
        else if (players[i].team_name == 'B')
        {
            if (players[i].has_ball)
            {
                num_of_balls_teamB += players[i].has_ball; // add the number of balls the player has
            }
        }
    }

    printf("Number of balls have been used in that round is %d\n", num_of_balls);
    // check who won the round and update the lost rounds for each team.
    if (num_of_balls_teamA > num_of_balls_teamB)
    {
        num_of_lost_rounds_teamA++;
        printf("Team A has %d balls and Team B has %d balls, Team B Won round %d\n", num_of_balls_teamA, num_of_balls_teamB, current_round);
    }
    else if (num_of_balls_teamA < num_of_balls_teamB)
    {
        num_of_lost_rounds_teamB++;
        printf("Team A has %d balls and Team B has %d balls, Team A Won round %d\n", num_of_balls_teamA, num_of_balls_teamB, current_round);
    }
    else
    {
        printf("Team A has %d balls and Team B has %d balls, Round %d is a draw\n", num_of_balls_teamA, num_of_balls_teamB, current_round);
    }
    printf("====================================\n");
    // print the number of lost rounds for each team
    printf("After %d dounds, Team A has lost %d rounds and Team B has lost %d rounds\n",
           current_round, num_of_lost_rounds_teamA, num_of_lost_rounds_teamB);
    current_round++;
    // end the game
    raise(SIGQUIT);
}

// handler for the parent process signals
void parent_signals_handler(int signum)
{
    if (signum == SIGUSR1)
    {
        printf("Parent received the signal to start the round\n");
        printf("=============================\n");
        start_round();
    }
    else if (signum == SIGQUIT)
    {
        printf("Parent received the signal to end the game\n");
        // kill all childs
        for (int i = 0; i < 2 * NUM_PLAYERS; i++)
        {
            printf("Killing player %d with pid %d\n", i, players_pids[i]);
            kill(players_pids[i], SIGTERM);
        }
        printf("Parent process is done\n");
        exit(0);
    }
    else if (signum == SIGTSTP)
    {
        // donothing
        printf("Parent process can't be stopped\n");
    }
    else if (signum == SIGUSR2)
    {
        // open public fifo for reading the team name
        int read_fd = open(PUBLIC, O_RDONLY);
        if (read_fd == -1)
        {
            perror("Error opening public fifo for reading");
            exit(1);
        }
        char team_name;
        if (read(read_fd, &team_name, sizeof(char)) == -1)
        {
            perror("Error reading from public fifo");
            exit(1);
        }
        close(read_fd);
        printf("Parent received the signal from the leader of team %c\n", team_name);
        printf("====================================\n");
        fflush(stdout);
        if (team_name == 'A')
        {
            current_balls_teamA--; // team A has throw the ball to the other team
            current_balls_teamB++; // team B has received the ball
            if (current_balls_teamA == 0)
            {
                // send new ball to the leader of team A
                kill(players_pids[5], SIGUSR2);
                printf("Parent sent new ball to the leader of team A\n");
                current_balls_teamA++; // team A received a new ball
                num_of_balls++;
            }
            else
            {
                printf("Request refused, team A still has %d balls\n", current_balls_teamA);
            }
        }
        else if (team_name == 'B')
        {
            current_balls_teamB--; // team B has throw the ball to the other team
            current_balls_teamA++; // team A has received the ball
            if (current_balls_teamB == 0)
            {
                // send new ball to the leader of team B
                kill(players_pids[11], SIGUSR2);
                printf("Parent sent new ball to the leader of team B\n");
                current_balls_teamB++; // team B received a new ball
                num_of_balls++;
            }
            else
            {
                printf("Request refused, team B still has %d balls\n", current_balls_teamB);
            }
        }
    }
    else if (signum == SIGALRM)
    {
        end_round();
    }
    else
    {
        printf("Parent received an unexpected signal\n");
    }
}

// create a public fifo
void create_public_fifo()
{

    // create private fifos for each player , nameing will belike this "fifo%d" where %d is the pid of the player
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        char fifo_name[20];
        sprintf(fifo_name, "/tmp/fifo%d", players_pids[i]);
        if (access(fifo_name, F_OK) == 0)
        {
            if (remove(fifo_name) == -1)
            {
                perror("Error removing fifo");
                exit(-1);
            }
        }
        if (mkfifo(fifo_name, 0666) == -1)
        {
            perror("Error creating fifo");
            exit(-1);
        }
    }
    printf("Private fifos created\n");

    remove(PUBLIC);
    if (mkfifo(PUBLIC, 0666) == -1)
    {
        perror("Error");
        exit(-1);
    }
}

int generate_energy(int id)
{
    srand(time(NULL));
    int range;
    // it its id is 5 or 11 then it is a team leader, else its normal player
    if (id == 5 || id == 11)
    {
        range = MAX_LEADER_ENERGY - MIN_LEADER_ENERGY + 1;
        return (rand() % range) + MIN_LEADER_ENERGY;
    }
    else
    {
        range = MAX_PLAYER_ENERGY - MIN_PLAYER_ENERGY + 1;
        return (rand() % range) + MIN_PLAYER_ENERGY;
    }
}
// initialize the player
void initialize_player(int id, pid_t next_player_pid, pid_t team_leader_pid)
{
    struct Player player;
    player.id = id;
    player.energy = generate_energy(id);
    player.has_ball = 0;
    player.team_name = id < 6 ? 'A' : 'B';
    player.next_player_pid = next_player_pid;
    player.team_leader_pid = team_leader_pid;
    players[id] = player;
}

void parent_set_signals()
{
    struct sigaction sa;
    sa.sa_handler = parent_signals_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("Error setting signal handler for SIGUSR1");
        exit(1);
    }

    struct sigaction sa2;
    sa2.sa_handler = parent_signals_handler;
    sa2.sa_flags = SA_RESTART; // restart the system call if it is interrupted
    sigemptyset(&sa2.sa_mask); // clear the signal set
    if (sigaction(SIGQUIT, &sa2, NULL) == -1)
    {
        perror("Error setting signal handler for SIGQUIT");
        exit(1);
    }

    struct sigaction sa3;
    sa3.sa_handler = parent_signals_handler;
    sa3.sa_flags = SA_RESTART;
    sigemptyset(&sa3.sa_mask);
    if (sigaction(SIGTSTP, &sa3, NULL) == -1)
    {
        perror("Error setting signal handler for SIGTSTP");
        exit(1);
    }

    struct sigaction sa4;
    sa4.sa_handler = parent_signals_handler;
    sa4.sa_flags = SA_RESTART;
    sigemptyset(&sa4.sa_mask);
    if (sigaction(SIGUSR2, &sa4, NULL) == -1)
    {
        perror("Error setting signal handler for SIGINT");
        exit(1);
    }

    struct sigaction sa5;
    sa5.sa_handler = parent_signals_handler;
    sa5.sa_flags = SA_RESTART;
    sigemptyset(&sa5.sa_mask);
    if (sigaction(SIGALRM, &sa5, NULL) == -1)
    {
        perror("Error setting signal handler for SIGALRM");
        exit(1);
    }
}

void print_player(struct Player player)
{
    printf("Player %d from team %c has %d energy and next player is %d and team leader is %d and has %d balls\n",
           player.id, player.team_name, player.energy, player.next_player_pid, player.team_leader_pid, player.has_ball);
}
