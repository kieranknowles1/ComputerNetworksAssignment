/* Sample solution for the LunarLander assignment
 * KV5002
 *
 * Dr Alun Moon
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include <pthread.h>
#include <semaphore.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>

#include "libnet.h"
#include "console.h"

#include <ctype.h>
#include <curses.h>

// Dummy header to silence false positive errors in VS code
// All content is disabled with #ifndef and -D in Makefile
#include "dummy.h"

/* semaphores and global variables for communication */

struct command {
	float thrust;
	float rotn;
} landercommand ;
sem_t cmdlock;

struct state {
    float x, y, O;
    float dx, dy, dO;
} landerstate;
sem_t statelock;

enum condstate { Flying, Down, Crashed };
struct condition {
    float fuel;
    float altitude;
    int contact;
} landercond;
sem_t condlock;

const char* condstate_to_string(enum condstate state) {
    switch (state) {
        case Flying: return "Flying";
        case Down: return "Down";
        case Crashed: return "Crashed";
        default: assert(false);
    }
}


/*** There are five threads defined for the controller ***/

/* Keyboard scanning routine
	Runs in own thread, interprets user input.
	Generates command structures for transmission to lander.
*/
int last;
void *keyboard(void *data)
{
	landercommand.thrust = 0;
	landercommand.rotn=0;
    while (true) {
        int key;
        while ((key = key_pressed()) == ERR){;}   /* wait for a key-press */

		last=key;
		sem_wait(&cmdlock);/*Enter critical section*/
        switch (key) {
        case KEY_UP:
            landercommand.thrust += 2;
			if( landercommand.thrust > 100 ) landercommand.thrust=100;
            break;
        case KEY_DOWN:
            landercommand.thrust -= 2;
			if( landercommand.thrust < 0 ) landercommand.thrust=0;
            break;
        case KEY_RIGHT:
    		landercommand.rotn +=0.1;
	 		if( landercommand.rotn >= 1 ) landercommand.rotn=1;
            break;
        case KEY_LEFT:
            landercommand.rotn-=0.1;
			if( landercommand.rotn <= -1.0 ) landercommand.rotn=-1;
            break;
        }
		sem_post(&cmdlock);/*Exit critical section*/
    }
}

/*-----------------------------------------*/

/* Display management -- threaded
	Updates display with diagnostic information
*/
void *display(void *data)
{
    while (true) {
		sem_wait(&condlock);
		switch(landercond.contact) {
			case Flying:
				lcd_set_colour(3,0);
				lcd_write_at(1,30,"Flying ");
				break;
			case Down:
				lcd_set_colour(2,.0);
				lcd_write_at(1,30,"Down   ");
				break;
			case Crashed:
				lcd_set_colour(1,0);
				lcd_write_at(1,30,"Crashed");
				break;
		}
		lcd_set_colour(7,0);
		lcd_write_at(0,0, "fuel %f", landercond.fuel);
		lcd_write_at(1,0, "alt  %f", landercond.altitude);
		sem_post(&condlock);


		sem_wait(&statelock);
		lcd_write_at(3,0, "x %-6.1f  x' %-8.6f", landerstate.x, landerstate.dx);
		lcd_write_at(4,0, "y %-6.1f  y' %-8.6f", landerstate.y, landerstate.dy);
		lcd_write_at(5,0, "O %-6.3f  O' %-8.6f", landerstate.O, landerstate.dO);
		sem_post(&statelock);

		sem_wait(&cmdlock);
		lcd_write_at(3,30,"thrust %6.1f", landercommand.thrust);
		lcd_write_at(4,30,"rotn %6.1f", landercommand.rotn);
		sem_post(&cmdlock);


		switch(last){
			case KEY_UP:
				lcd_write_at(0,40,"up   ");
				break;
			case KEY_DOWN:
				lcd_write_at(0,40,"down ");
				break;
			case KEY_LEFT:
				lcd_write_at(0,40,"left ");
				break;
			case KEY_RIGHT:
				lcd_write_at(0,40,"right");
				break;
			default:
				lcd_write_at(0,40,"%c   ", last);
		}

        usleep(500000);
    }
}

/*-----------------------------------------*/

/* Parse condition reply message
*/
void parsecondition(char *m)
{
    char *line;
    char *rest;
    for (  /* split into lines lecture 07-2 slide 21 */
            line = strtok_r(m, "\r\n", &rest);
            line != NULL;
			line = strtok_r(NULL, "\r\n", &rest)
        ) {
        char *key, *value;
        key = strtok(line, ":");
        value = strtok(NULL, ":");

        sem_wait(&condlock);    /* Enter critical section */

        if (strcmp(key, "fuel") == 0)
            sscanf(value, "%f%%", &(landercond.fuel));

        if (strcmp(key, "altitude") == 0)
            sscanf(value, "%f", &(landercond.altitude));

        if (strcmp(key, "contact") == 0) {
            if (strcmp(value, "flying") == 0)
                landercond.contact = Flying;

            if (strcmp(value, "down") == 0)
                landercond.contact = Down;

            if (strcmp(value, "crashed") == 0)
                landercond.contact = Crashed;
        }
        sem_post(&condlock);    /* Exit critical section */
    }
}


/* Parse state message */
void parsestate(char *m)
{
    char *line;
    char *rest;
    for (           /* split into lines lecture 07-2 slide 21 */
            line = strtok_r(m, "\r\n", &rest);
            line != NULL; line = strtok_r(NULL, "\r\n", &rest)
        ) {
        char *key, *value;
        key = strtok(line, ":");
        value = strtok(NULL, ":");

        sem_wait(&statelock);   /*Enter critical section */

        if (strcmp(key, "x") == 0)
            sscanf(value, "%f", &(landerstate.x));

        if (strcmp(key, "y") == 0)
            sscanf(value, "%f", &(landerstate.y));

        if (strcmp(key, "O") == 0)
            sscanf(value, "%f", &(landerstate.O));

        if (strcmp(key, "x'") == 0)
            sscanf(value, "%f", &(landerstate.dx));

        if (strcmp(key, "y'") == 0)
            sscanf(value, "%f", &(landerstate.dy));

        if (strcmp(key, "O'") == 0)
            sscanf(value, "%f", &(landerstate.dO));

        sem_post(&statelock);   /*Exit critical section */
    }
}

/* Lander Communications.
	Communicates with the lander model,
	Sends commands and queries state,
	Parses and decodes messages

	Data passed is port number to use.
*/
void *lander(void *data)
{
    char* service = (char*)data;

    size_t msgsize = 1000;
    char msgbuf[msgsize];
    int l;
    struct addrinfo *landr;

    const char conditionq[] = "condition:?\n";
    const char stateq[] = "state:?\n";

    if (!getaddr("127.0.1.1", service, &landr))
        fprintf(stderr, "can't get lander address");;
    fprintf(stderr, "%s\n", landr->ai_canonname);

    l = mksocket();

    while (true) {
		int m;
        usleep(50000);          /* 20Hz = 0.05s = 50ms = 50000us */
        /* poll for condition */
        sendto(l, conditionq, strlen(conditionq), 0, landr->ai_addr,
               landr->ai_addrlen);

        m = recvfrom(l, msgbuf, msgsize, 0, NULL, NULL);
        msgbuf[m] = '\0';

        /* parse condition */
        parsecondition(msgbuf);


        /* poll for state */
        sendto(l, stateq, strlen(stateq), 0, landr->ai_addr,
               landr->ai_addrlen);

        m = recvfrom(l, msgbuf, msgsize, 0, NULL, NULL);
		msgbuf[m] = '\0';
		parsestate(msgbuf);


		/* format command to send to lander */
        sem_wait(&cmdlock);
		sprintf(msgbuf,
				"command:!\n"
				"main-engine: %f\n"
				"rcs-roll: %f\n"
				,
				landercommand.thrust, landercommand.rotn
			   );
        sem_post(&cmdlock);

		sendto(l, msgbuf, strlen(msgbuf), 0, landr->ai_addr, landr->ai_addrlen);
        m = recvfrom(l, msgbuf, msgsize, 0, NULL, NULL);
		msgbuf[m] = '\0';


		usleep(100000);
    }
}

/*-----------------------------------------*/

/* Dashboard Communications.
	Formats and sends data messages to the dashboard.
*/
void *dashboard(void *data)
{
   	size_t bufsize = 1024;
	char buffer[bufsize];
	int d;
	struct addrinfo *daddr;

	if( !getaddr("127.0.1.1", (char*)data, &daddr))
		fprintf(stderr,"cant get dash address");
	d = mksocket();

    while (true) {
        // Format buffer for dashboard
        // Keep landercond locked for a minimal amount of time
        sem_wait(&condlock);
        sprintf(buffer,
            "fuel:%f\n"
            "altitude:%f\n",
            landercond.fuel,
            landercond.altitude
        );
        sem_post(&condlock);

        // Send formatted buffer using the previously acquired address
        sendto(d, buffer, strlen(buffer), 0, daddr->ai_addr, daddr->ai_addrlen);

		usleep(500000);
	}
}

/*-----------------------------------------*/

/* Data Logging.
	Periodically logs data to a file.
*/

const int MICROSECONDS_PER_SECOND = 1000 * 1000;

// 5hz
const int DATA_LOG_SLEEP_MICROSECONDS = MICROSECONDS_PER_SECOND / 5;
const char* DATA_LOG_FILE = "lander.log";

void *datalogging(void *data)
{
    FILE* file = fopen(DATA_LOG_FILE, "w");

    int index = 1;
	while(true) {
        fprintf(file, "Log entry %d:\n", index++);

        // User input
        sem_wait(&cmdlock);
        fprintf(file,
            " Commands:\n"
            "  Rotation: %.1f\n"
            "  Thrust: %.1f\n",
            landercommand.rotn,
            landercommand.thrust
        );
        sem_post(&cmdlock);

        sem_wait(&condlock);

        // Fuel, altitude, and contact state
        fprintf(file,
            " Condition:\n"
            "  Fuel: %.1f%%\n"
            "  Altitude: %.1f\n"
            "  State: %s\n",
            landercond.fuel,
            landercond.altitude,
            condstate_to_string(landercond.contact)
        );
        sem_post(&condlock);

        // Position and velocity
        sem_wait(&statelock);
        fprintf(file,
            " Lander state:\n"
            "  Position: %-6.1fx, %-6.1fy\n"
            "  Velocity: %-6.1fx, %-6.1fy\n"
            "  Angle: %-6.3fO\n"
            "  Angular velocity: %-6.3fO'\n",
            landerstate.x, landerstate.y,
            landerstate.dx, landerstate.dy,
            landerstate.O,
            landerstate.dO
        );
        sem_post(&statelock);

        // Need to manually flush as the file is never closed normally
        fflush(file);

        usleep(DATA_LOG_SLEEP_MICROSECONDS);
	}
}

/*-----------------------------------------*/


/* Entry point for program.
	arguments are the (local) port numbers for
	the lander and the dashboard
*/
int main(int argc, char *argv[])
{
    pthread_t kscan;            /* keyboard */
    pthread_t dsply;            /* display */
    pthread_t lndr;             /* lander comms */
    pthread_t dash;             /* dashboard */

    int e;

    /* initialise semaphores */
	sem_init( &condlock, 0, 1);
	sem_init( &statelock, 0, 1);
	sem_init( &cmdlock, 0, 1);


    /* initialise and start display */
    console_init();

    /* Start Threads .... */

    if ((e = pthread_create(&dsply, NULL, display, NULL)))
        fprintf(stderr, "not created display thread: %s\n", strerror(e));


    if ((e = pthread_create(&kscan, NULL, keyboard, NULL)))
        fprintf(stderr, "not cheated keybord thread: %s\n", strerror(e));




    if ((e = pthread_create(&lndr, NULL, lander, argv[1])))
        fprintf(stderr, "not created lander thread: %s\n", strerror(e));

    // Start dashboard and data logging threads
    if ((e = pthread_create(&dash, NULL, dashboard, argv[2])))
        fprintf(stderr, "Failed to create dashboard thread: %s\n", strerror(e));

    pthread_t dataLogThread;
    if ((e = pthread_create(&dataLogThread, NULL, datalogging, NULL)))
        fprintf(stderr, "Failed to create logging thread: %s\n", strerror(e));


    pthread_join(dsply, NULL);
}
