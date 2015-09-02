
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "inttypes.h"

#include "rogo.h"
#include "state.h"


struct State state;


static void helpmsg()
{
	puts("Rogo " VERSION "");
	puts("(c) 2005 Tomas van der Wansem\n");

	puts("  SYNTAX: rogo [-mn] [-sn] [-p]\n");
	puts("  -mn   Allocate n mb memory. This must be at least 100. (default 350)\n");
	puts("  -sn   Set initial boardsize to n.\n        Odd sizes between 5 and 25 allowed. (default 19)\n");
	puts("  -p    Run in practice-mode. The initial database will be improved\n        for the selected boardsize.\n");

	puts("  EXAMPLE: rogo -m750 -s9 -p");
	puts("           Runs rogo in practice-mode on a 9x9 board using 750 mb memory\n");


}


int state_init(int argc, char *argv[])
{
	int n;
	memset(&state, 0, sizeof(state));
	state.boardsize = 19;
	state.mode = PLAY;
	state.totalmem = 350 * 1024 * 1024;
	state.console = -1;
	state.quickplay = 0;
	state.verbose = 0;

	for(n=1; n < argc; n++)
	{
		if (argv[n][0] != '-')
		{
			puts("\nSyntax error in argument. Use rogo -h for help.\n");
			return 0;
		}

		switch (argv[n][1])
		{
		case 'q':
			{
				state.quickplay = -1;
				break;
			}
		case 'v':
			{
				state.verbose = -1;
				break;
			}
		case 'm':
			{
				int size = atoi(argv[n]+2);
				if (size < 100)
				{
					puts("\nNot enough memory specified. At least 100mb is required\n");
					return 0;
				}

				state.totalmem = size * 1024 * 1024;
				break;
			}
		case 'p':
			{
				if (argv[n][2] == 0)
					state.mode = PRACTICE;
				else
				{
					puts("\nUnrecognized syntax near -p. Use rogo -h for options\n");
					return 0;
				}
				break;
			}
		case 's':
			{
				int size = atoi(argv[n]+2);
				if (size < 5 || size > 25 || (size % 2 == 0))
				{
					puts("\nIllegal boardsize specified. Only odd boardsizes between 5 and 25 allowed\n");
					return 0;
				}

				state.boardsize = size;
				break;
			}
		case 'n':
			{
				state.console = 0;
				break;
			}
		case 'h':
			{
				helpmsg();
				return 0;
			}
		default:
			puts("\nUnrecognized option. Use rogo -h for available options.\n");
			return 0;

		}
	}


	return -1;
}

