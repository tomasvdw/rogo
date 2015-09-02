
#ifdef _WIN32
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif

#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#ifndef _WIN32
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "inttypes.h"


#include "rogo.h"
#include "program.h"
#include "state.h"
#include "gtp.h"
#include "persist.h"
#include "stats.h"
#include "rand.h"
#include "goban.h"
#include "timing.h"
#include "utils.h"
#include "engine.h"
#include "hash.h"

#define HIST_LINES 1000
#define HIST_FILE "/home/tomas/.rogo_history"

int stab_power = 2;
int dist_pow = 3;
double utc_fac = 0.3;
int move_lim = 60;
int rounds = 100000;


int signalSTOP = 0;
int signalBREAK = 0;

void testgame_setup()
{
    // some interesting initial positions
	if (state.boardsize==5)
	{
		goban_playstring("A2 A3 PASS A4 B2 B3 PASS B4 C2 C3 PASS C4");

		goban_playstring("D2 D3 PASS D4");
		//goban_playstring("E2 E4");
	}
	if (state.boardsize==7)
	{
		//goban_playstring("A3 A4 B3 B4 C3 C4 D3 D4 E3 E4");
		//goban_playstring("F3 F4");
//		goban_playstring("A2 A5 B2 B5 C2 C5 D2 D5 E2 E5 F2 F5");
//
//		// difficult endgame
//		goban_playstring("B2 C2 B3 C3 B4 D3 B5 E3 C4 D4 D5 E5 E6 F6 D6 F4 C6");

//
//  starts good at 100k fails after 400k
//		goban_playstring("E4 C5 B5 B4 C4 D4 C3 B6");
//		goban_playstring("C6 D5 D3 E5 F5 E3");

		// difficult fuseki
		//goban_playstring("D4 C3 C4 D3 E3 E2");

	}
	if (state.boardsize==6)
	{
		goban_playstring("A3 A4 B3 B4 C3 C4 D3 D4 E3 E4");
		//goban_playstring("F3 F4");
//		goban_playstring("A2 A5 B2 B5 C2 C5 D2 D5 E2 E5 F2 F5");
	}

	//goban_playstring("D5 D3 B2 E4 E5 D1 B3 E3 C2 D2 C3 B1 A2 B4 A5 D4 E2 E1 C5 E2 C1 C4 A4 A3 A1 A3 B5 E4 D1 D4 E2 A3 C4 B4");
	//goban_playstring("C2 D2 C3 D3 C4 D4 C5 D5 C6 D6");
 }

#ifdef _WIN32
BOOL WINAPI WinCtrlHandler(DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		signalSTOP = -1;

	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
		signalBREAK = -1;
		break;
	}

	return TRUE;
}
#endif
extern int bestmove;
extern int counter;
int main(int argc, char* argv[])
{

	assert(sizeof(int) == 4);
	assert(sizeof(long long) == 8);

	/* arguments are stored in state structure */
	fileGTP = stdout;
	if (!state_init(argc, argv)) return 0;

	state.boardsize = 7;

#ifdef _WIN32
	SetConsoleCtrlHandler(WinCtrlHandler, TRUE);
	SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

	if (!state.console)
		ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif


	rnd_init();
	//timing_init();
	program_init();
	goban_init();

	hash_init();

	gtp_init();
	//persist_init();


	goban_backup();
    // testgame_setup();

	engine_init();

	if (0)
	{
//		out(fileGTP, "Starting: %d\n", counter++);
//		// testing single move
		counter++;

		engine_tryvariation(0);
		goban_restore();
		if (counter % rounds == 0)
		{
			out(fileInfo, "\n\n**** PLAYING: %t %p \n", goban.turn, bestmove);
			goban_move(bestmove);
			goban_backup();
			outboard(fileInfo);
			out(fileInfo, "\n\n");
			engine_init();
		}

	}

    // main gtp loop windows
#ifdef _WIN32
	while (!signalSTOP)
	{
        char buffer[1024];

		fgets(buffer, 1024, stdin);


        if (!buffer) 
            signalSTOP = -1;
        else 
        {

            if (!gtp_processCommand(buffer))
                signalSTOP = -1;

            free(buffer);
        }
	}


#else

    read_history(HIST_FILE);

    // main gtp loop linux
	while (!signalSTOP)
	{
        char *buffer = readline(NULL);

        if (!buffer) 
            signalSTOP = -1;
        else 
        {
            add_history(buffer);

            if (!gtp_processCommand(buffer))
                signalSTOP = -1;

            free(buffer);
        }
	}

    {
       int en;
    if (en = write_history(HIST_FILE) != 0)
    printf("Error writing history (%d, %s)", en, strerror(errno));

 }
#endif
	return 0;

}

