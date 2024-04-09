#include <GL/glut.h>
#include "config.h"

#define PI 3.14159265

struct GUI_Player players[2 * NUM_PLAYERS];

GameState game_state;
int actual_balls_count = 0;
int pause_flag = 0;

// Team A positions
float teamA[6][2] = {
    {0.3, 0.1},  // player 0
    {0.5, 0.1},  // player 1
    {0.5, -0.1}, // player 2
    {0.3, -0.1}, // player 3
    {0.1, -0.1}, // player 4
    {0.1, 0.1}   // team leader A
};

// Team B positions
float teamB[6][2] = {
    {-0.3, 0.1},  // player 6
    {-0.5, 0.1},  // player 7
    {-0.5, -0.1}, // player 8
    {-0.3, -0.1}, // player 9
    {-0.1, -0.1}, // player 10
    {-0.1, 0.1}   // team leader B
};

//--------------------------------------------------------------------------------------------
// Function prototypes
void player_signals_gui_handler(int signum);
void set_player_gui_signals();
void updateGameBalls();
void player_throw_ball();
void leader_thrown_ball();
void trhow_ball_to_player();

//--------------------------------------------------------------------------------------------
// Function to display content on the window
void display()
{
    // printf("Actual balls count: %d\n", actual_balls_count);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw Team A members
    glColor3f(0.0, 1.0, 0.0); // Green color
    for (int i = 0; i < 6; ++i)
    {
        glPushMatrix();
        glTranslatef(players[i].x, players[i].y, 0.0);
        glutSolidSphere(0.02, 30, 30); // Draw a solid sphere representing a team member
        glPopMatrix();
    }

    // Draw Team B members
    glColor3f(0.0, 0.0, 1.0); // Blue color
    for (int i = 6; i < 12; ++i)
    {
        glPushMatrix();
        glTranslatef(players[i].x, players[i].y, 0.0);
        glutSolidSphere(0.02, 30, 30); // Draw a solid sphere representing a team member
        glPopMatrix();
    }

    // Draw the balls in the game if they are valid
    glColor3f(1.0, 0.0, 0.0); // Red color for balls
    for (int i = 0; i < NUM_OF_BALLS; ++i)
    {
        if (game_state.balls[i].valid)
        {
            glPushMatrix();
            glTranslatef(game_state.balls[i].x, game_state.balls[i].y, 0.0);
            glutSolidSphere(0.0188, 15, 15); // Draw a solid sphere representing the ball
            glPopMatrix();
        }
    }

    glutPostRedisplay(); // Mark the window for redisplay
    glFlush();
}

void timerCallback(int value)
{

    // Set up the next timer callback
    glutTimerFunc(1000, display, value);
}

//--------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{

    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        players[i].id = atoi(argv[i]);
        // players[i].has_ball = 0;
        if (i < 6)
        {
            players[i].x = teamA[i][0];
            players[i].y = teamA[i][1];
        }
        else
        {
            players[i].x = teamB[i - 6][0];
            players[i].y = teamB[i - 6][1];
        }
    }

    // print all pids
    for (int i = 0; i < 2 * NUM_PLAYERS; i++)
    {
        printf("Player %d PID: %d\n", i, players[i].id);
    }

    set_player_gui_signals();

    // initialize the ball position for all balls
    for (int i = 0; i < NUM_OF_BALLS; i++)
    {
        game_state.balls[i].x = 0.1;
        game_state.balls[i].y = 0.1;
        game_state.balls[i].last_player_id = -1;
        game_state.balls[i].valid = 0;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(400, 100);
    glutCreateWindow("Beach Ball Simulation");

    glClearColor(0.0, 0.0, 0.0, 1.0); // Set clear color to black
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(-1.0, 1.0, -1.0, 1.0); // Set the coordinate system

    glutDisplayFunc(display);

    glutTimerFunc(20, timerCallback, 0);
    glutMainLoop();
    // Start the animation
    // while (1)
    // {
    //     pause();
    // }

    return 0;
}
void player_throw_ball()
{
    /*
        when players throws a ball
        open PRIVATE fifo and read the pid of the process that has thrown the ball
    */
    if (!pause_flag)
    {
        pid_t pid;
        int read_fd = open(PRIVATE, O_RDONLY);
        if (read_fd == -1)
        {
            perror("Error opening private fifo for reading");
            exit(1);
        }
        if (read(read_fd, &pid, sizeof(pid_t)) == -1)
        {
            perror("Error reading from private fifo");
            exit(1);
        }
        close(read_fd);
        printf("GUI PID %d\n", pid);
        // find the player that has received the ball, and update the ball position
        int player_index = -1;
        int next_player_index = -1;
        for (int i = 0; i < 2 * NUM_PLAYERS; i++)
        {
            if (players[i].id == pid && pause_flag == 0)
            {
                player_index = i;
                break;
            }
        }
        if (player_index == -1)
        {
            printf("Error: Invalid player id\n");
            return;
        }

        if (pause_flag)
        {
            printf("GUI Interrupted\n");
            fflush(stdout);
            return;
        }
        if (player_index == 5 && pause_flag == 0)
        {
            /*1st Case: Team A leader has thrown the ball, he will pass to player 0*/
            next_player_index = 0;
            for (int j = 0; j < actual_balls_count; j++)
            {
                if (game_state.balls[j].last_player_id == players[5].id)
                {
                    printf("Throw Ball : P%d ---> P%d\n", player_index, next_player_index);
                    game_state.balls[j].x = players[next_player_index].x;
                    game_state.balls[j].y = players[next_player_index].y;
                    game_state.balls[j].last_player_id = players[next_player_index].id;
                    game_state.balls[j].valid = 1;
                    updateGameBalls();
                    break;
                }
            }
        }
        else if (player_index == 11 && pause_flag == 0)
        {
            /*2nd Case: Team B leader has thrown the ball, he will pass to player 6*/
            next_player_index = 6;
            for (int j = 0; j < actual_balls_count; j++)
            {
                if (game_state.balls[j].last_player_id == players[11].id)
                {
                    printf("Throw Ball : P%d ---> P%d\n", player_index, next_player_index);
                    game_state.balls[j].x = players[next_player_index].x;
                    game_state.balls[j].y = players[next_player_index].y;
                    game_state.balls[j].last_player_id = players[next_player_index].id;
                    game_state.balls[j].valid = 1;
                    updateGameBalls();
                    break;
                }
            }
        }
        else
        {
            /*3rd Case: Normal player has thrown the ball, he will pass to the next player*/
            next_player_index = player_index + 1;
            for (int j = 0; j < actual_balls_count; j++)
            {
                if (game_state.balls[j].last_player_id == players[player_index].id)
                {
                    printf("Throw Ball : P%d ---> P%d\n", player_index, next_player_index);
                    game_state.balls[j].x = players[next_player_index].x;
                    game_state.balls[j].y = players[next_player_index].y;
                    game_state.balls[j].last_player_id = players[next_player_index].id;
                    game_state.balls[j].valid = 1;
                    updateGameBalls();
                    break;
                }
            }
        }
    }
}

void leader_thrown_ball()
{
    /* When Leader throws the ball to the leader of the other team*/
    if (pause_flag == 0)
    {
        pid_t pid;
        int read_fd = open(PRIVATE2, O_RDONLY);
        if (read_fd == -1)
        {
            perror("Error opening private fifo for reading");
            exit(1);
        }
        if (read(read_fd, &pid, sizeof(pid_t)) == -1)
        {
            perror("Error reading from private fifo");
            exit(1);
        }
        close(read_fd);

        printf("GUI received Leader PID %d\n", pid);
        int leader_index = -1;
        int next_leader_index = -1;
        for (int i = 0; i < 2 * NUM_PLAYERS; i++)
        {
            if (players[i].id == pid)
            {
                leader_index = i;
                break;
            }
        }
        if (leader_index == -1)
        {
            printf("Error: Invalid player id\n");
            return;
        }

        if (pause_flag)
        {
            printf("GUI Interrupted\n");
            fflush(stdout);
            return;
        }
        if (leader_index == 5 && pause_flag == 0)
        {
            /*1st Case: Team A leader has thrown the ball, he will pass to the leader of Team B*/
            next_leader_index = 11;
            for (int j = 0; j < actual_balls_count; j++)
            {
                if (game_state.balls[j].last_player_id == players[leader_index].id)
                {
                    printf("Leaders Throw Ball : P%d ---> P%d\n", leader_index, next_leader_index);
                    game_state.balls[j].x = players[next_leader_index].x;
                    game_state.balls[j].y = players[next_leader_index].y;
                    game_state.balls[j].last_player_id = players[next_leader_index].id;
                    game_state.balls[j].valid = 1;
                    updateGameBalls();
                    break;
                }
            }
        }
        else if (leader_index == 11 && pause_flag == 0)
        {
            /*2nd Case: Team B leader has thrown the ball, he will pass to the leader of Team A*/
            next_leader_index = 5;
            for (int j = 0; j < actual_balls_count; j++)
            {
                if (game_state.balls[j].last_player_id == players[next_leader_index].id)
                {
                    printf("Leaders Throw Ball : P%d ---> P%d\n", leader_index, next_leader_index);
                    game_state.balls[j].x = players[next_leader_index].x;
                    game_state.balls[j].y = players[next_leader_index].y;
                    game_state.balls[j].last_player_id = players[next_leader_index].id;
                    game_state.balls[j].valid = 1;
                    updateGameBalls();
                    break;
                }
            }
        }
        else
        {
            printf("Error: Invalid leader id %d\n", leader_index);
        }
    }
}

void assigne_ball_teamA()
{
    /* used to assign new ball for a leader of team */
    actual_balls_count++;
    printf("Team A received a new ball ==== Ball count: %d\n", actual_balls_count);
    game_state.balls[actual_balls_count - 1].x = players[5].x;
    game_state.balls[actual_balls_count - 1].y = players[5].y;
    game_state.balls[actual_balls_count - 1].last_player_id = players[5].id;
    game_state.balls[actual_balls_count - 1].valid = 1;
    updateGameBalls();
}

void assigne_ball_teamB()
{
    /* used to assign new ball for a leader of team */
    actual_balls_count++;
    printf("Team B received a new ball ==== Ball count: %d\n", actual_balls_count);
    game_state.balls[actual_balls_count - 1].x = players[11].x;
    game_state.balls[actual_balls_count - 1].y = players[11].y;
    game_state.balls[actual_balls_count - 1].last_player_id = players[11].id;
    game_state.balls[actual_balls_count - 1].valid = 1;
    updateGameBalls();
}

void player_signals_gui_handler(int signum)
{
    // notify the gui to read from pipe
    if (signum == SIGUSR1)
    {
        if (!pause_flag)
        {
            /* Player thrown a ball to the next player*/
            player_throw_ball();
        }
    }
    if (signum == SIGUSR2)
    {
        if (!pause_flag)
        {
            /* Leader thrown a ball to the leader of the other team */
            leader_thrown_ball();
        }
    }

    else if (signum == SIGBUS)
    {
        /* New ball hse been thrown to the team A */
        if (pause_flag == 0)
        {
            assigne_ball_teamA();
        }
    }
    else if (signum == SIGCONT)
    {
        /* New ball has been thrown to the team B */
        if (pause_flag == 0)
        {
            assigne_ball_teamB();
        }
    }
    else if (signum == SIGINT)
    {
        pause_flag = !pause_flag;
        printf("Pause flag: %d\n", pause_flag);
        if (pause_flag)
        {
            usleep(300000); // sleep for 300 ms
            printf("GUI is paused\n");
            actual_balls_count = 0;
            // reset game state
            for (int i = 0; i < NUM_OF_BALLS; i++)
            {
                game_state.balls[i].x = 0.1;
                game_state.balls[i].y = 0.1;
                game_state.balls[i].last_player_id = -1;
                game_state.balls[i].valid = 0;
            }
        }
        else
        {
            // new round is about to start
            printf("GUI is resumed\n");
            // remove and recreate the PRIVATE fifo
            unlink(PRIVATE);
            if (mkfifo(PRIVATE, 0666) == -1)
            {
                perror("Error creating private fifo");
                exit(1);
            }
            printf("Recreate private fifo\n");
        }
    }
}

void trhow_ball_to_player()
{
    /*
    when player throws ball to another player
    open PRIVATE fifo and read the pid of both, the current player and the next player
    */
    /*open PRIVATE fifo to read message*/
    if (!pause_flag)
    {
        int read_fd = open(PRIVATE, O_RDONLY);
        if (read_fd == -1)
        {
            perror("Error opening private fifo for reading");
            exit(1);
        }
        message msg;
        if (read(read_fd, &msg, sizeof(message)) == -1)
        {
            perror("Error reading from private fifo");
            exit(1);
        }
        close(read_fd);
        printf("GUI: PID %d --> PID %d\n", msg.current_pid, msg.next_pid);
        int index_of_current_player = -1;
        int index_of_next_player = -1;
        /*find the index of the current player*/
        for (int j = 0; j < 2 * NUM_PLAYERS; j++)
        {
            if (players[j].id == msg.current_pid)
            {
                index_of_current_player = j;
                break;
            }
        }
        /*find the index of the next player*/
        for (int i = 0; i < 2 * NUM_PLAYERS; i++)
        {
            if (players[i].id == msg.next_pid)
            {
                index_of_next_player = i;
                break;
            }
        }
        if (pause_flag)
        {
            printf("GUI is paused\n");
            // fflush(stdout);
            return;
        }
        if (index_of_current_player == -1 || index_of_next_player == -1)
        {
            // printf("Error: Invalid player id");
            return;
        }
        // int new_ball_flag = 0;
        int ball_found = 0;
        if (pause_flag == 0)
        {
            /*find the ball that belongs to the player with the current_pid and
            move the ball from the player with the current_pid to the player with the next_pid*/
            for (int i = 0; i < actual_balls_count; i++)
            {
                if (game_state.balls[i].last_player_id == msg.current_pid)
                {
                    printf("Ball thrown: %d --> %d\n", index_of_current_player, index_of_next_player);
                    // the ball founded, update its position
                    game_state.balls[i].x = players[index_of_next_player].x;
                    game_state.balls[i].y = players[index_of_next_player].y;
                    game_state.balls[i].last_player_id = msg.next_pid;
                    game_state.balls[i].valid = 1;
                    updateGameBalls();
                    ball_found = 1;
                    break;
                }
            }

            if (!ball_found)
            {
                /*if the ball is not found, then it is a new ball*/
                for (int i = 0; i < actual_balls_count; i++)
                {
                    if (game_state.balls[i].last_player_id == -1)
                    {
                        printf("New Ball thrown: %d --> %d\n", index_of_current_player, index_of_next_player);
                        // the ball founded, update its position
                        game_state.balls[i].x = players[index_of_current_player].x;
                        game_state.balls[i].y = players[index_of_current_player].y;
                        game_state.balls[i].last_player_id = msg.current_pid;
                        game_state.balls[i].valid = 1;
                        updateGameBalls();
                        break;
                    }
                }
            }
        }
    }
}

void set_player_gui_signals()
{
    struct sigaction sa;
    sa.sa_handler = player_signals_gui_handler;
    sa.sa_flags = SA_RESTART;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        perror("Error setting signal handler for SIGUSR1");
        exit(1);
    }

    struct sigaction sa2;
    sa2.sa_handler = player_signals_gui_handler;
    sa2.sa_flags = SA_RESTART;
    sigemptyset(&sa2.sa_mask);
    if (sigaction(SIGUSR2, &sa2, NULL) == -1)
    {
        perror("Error setting signal handler for SIGUSR2");
        exit(1);
    }

    struct sigaction sa3;
    sa3.sa_handler = player_signals_gui_handler;
    sa3.sa_flags = SA_RESTART;
    sigemptyset(&sa3.sa_mask);
    if (sigaction(SIGBUS, &sa3, NULL) == -1)
    {
        perror("Error setting signal handler for SIGBUS");
        exit(1);
    }

    struct sigaction sa4;
    sa4.sa_handler = player_signals_gui_handler;
    sa4.sa_flags = SA_RESTART;
    sigemptyset(&sa4.sa_mask);
    if (sigaction(SIGINT, &sa4, NULL) == -1)
    {
        perror("Error setting signal handler for SIGUSR4");
        exit(1);
    }

    struct sigaction sa5;
    sa5.sa_handler = player_signals_gui_handler;
    sa5.sa_flags = SA_RESTART;
    sigemptyset(&sa5.sa_mask);
    if (sigaction(SIGCONT, &sa5, NULL) == -1)
    {
        perror("Error setting signal handler for SIGCONT");
        exit(1);
    }
    // make the GUI ignoring TSTP signal
    signal(SIGTSTP, SIG_IGN);
}

/////////////////////////////////////////////////////////////////////////////////////

void updateGameBalls()
{
    glutPostRedisplay();
}
