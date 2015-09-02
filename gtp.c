#define _CRT_SECURE_NO_DEPRECATE 1
#define _CRT_NONSTDC_NO_DEPRECATE 1

// gtp.c


#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <time.h>
#include "inttypes.h"

#include "rogo.h"
#include "goban.h"
#include "program.h"
#include "stats.h"
#include "state.h"
#include "game.h"
#include "utils.h"
#include "rules.h"
#include "engine.h"

static int id;

typedef int commandfunc(const char *param);

commandfunc quit;
commandfunc protocolVersion;
commandfunc name;
commandfunc version;
commandfunc knownCommand;
commandfunc listCommands;
commandfunc play;
commandfunc genmove;
commandfunc showBoard;
commandfunc boardSize;
commandfunc komi;
commandfunc clearBoard;

commandfunc query;
commandfunc try_variation;
commandfunc playSeq;

struct Command
{
	commandfunc *f;
	char name[25];
} gtp_commands[] = {
	{ boardSize, "boardsize" },
	{ clearBoard, "clear_board" },
	{ genmove, "genmove" },
	{ knownCommand, "known_command" },
	{ komi, "komi" },
	{ listCommands, "list_commands" },
	{ name, "name" },
	{ play, "play" },
	{ protocolVersion, "protocol_version"},
	{ query, "query" },
	{ quit, "quit" },
	{ version, "version" },
	{ showBoard, "showboard" },
	{ try_variation, "try" },
	{ playSeq, "play_seq" },
};




void gtp_init()
{
}

/* normalize whitespace and remove commands of buf; */
void gtp_preprocessCommand(char *buf)
{
	char *p = buf;
	while (*p)
	{
		if (*p == '\t')
			*p = ' ';

		if (*p == ' ' && p > buf && *(p-1) == ' ')
			memmove(p-1, p, buf - p + sizeof(buf));

		if (*p == '\n' || *p == '\r' || *p == '#')
			break;
		p++;
	}
	*p = 0;
}

void gtp_success(char *msg)
{
	if (id==-1 && strlen(msg)==0)
		out(fileGTP, "=\n\n");
	else if (id==-1 && strlen(msg)>0)
		out(fileGTP, "= %s\n\n", msg);
	else if (id!=-1 && strlen(msg)==0)
		out(fileGTP, "=%d\n\n", id);
	else
		out(fileGTP, "=%d %s\n\n", id, msg);
}


void gtp_error(const char *msg)
{
	if (id==-1)
		out(fileGTP, "? %s\n\n", msg);
	else
		out(fileGTP, "?%d %s\n\n", id, msg);
}

int gtp_processCommand(char *buf)
{
	int n;
	char *cmd, *param;

	gtp_preprocessCommand(buf);

	id = -1;

	// see if we have an id
	cmd = buf;
	if (*buf >= '0' && *buf <= '9')
	{
		// we have an id
		id = atoi(buf);

		cmd = strchr(cmd, ' ');
		if (!cmd)
		{
			gtp_error("syntax error"); // no command specified
			return -1;
		}
		cmd++;
	}
	else if (strlen(cmd) == 0)
		return -1;

	// find parameter
	param = strchr(cmd, ' ');
	if (param)
	{
		*param = 0;
		param++;
	}

	// match command from command-table
	for(n=0; n < sizeof(gtp_commands) / sizeof(gtp_commands[0]); n++)
		if (strcasecmp(gtp_commands[n].name, cmd)==0)
		{
			// invoke command
			return gtp_commands[n].f(param);
		}

	gtp_error("unknown command"); // no command specified

	return -1;
}




int protocolVersion(const char *param)
{
	gtp_success("2");
	return -1;
}

int listCommands(const char *param)
{
	int n;
	char allcommands[2048];
	allcommands[0]=0;

	assert(sizeof allcommands > sizeof gtp_commands * 2);

	for(n=0; n < sizeof gtp_commands / sizeof gtp_commands[0]; n++)
	{
		strcat(allcommands, gtp_commands[n].name);
		strcat(allcommands, "\n");
	}

	allcommands[strlen(allcommands)-1] = 0;

	gtp_success(allcommands);

	return -1;
}

int quit(const char *param)
{
	gtp_success("");
	return 0;
}


int version(const char *param)
{
	gtp_success(VERSION);
	return -1;
}

int name(const char *param)
{
	gtp_success(NAME);
	return -1;
}

int knownCommand(const char *param)
{
	int n;
	for(n=0; n < sizeof(gtp_commands) / sizeof(gtp_commands[0]); n++)
		if (strcasecmp(param, gtp_commands[n].name) == 0)
		{
			gtp_success("true");
			return -1;
		}

	gtp_success("false");
	return -1;

}


int play(const char *param)
{
	char *s;
	int c;
	int x,y;

	s = NULL;


	if (param) s = strchr(param,' ')+1;
	if (s==(char*)1 || !(*s))
		return gtp_error("two parameters required"), -1;

	if (param[0] == 'w' || param[0] == 'W')
		c = WHITE;
	else if (param[0] == 'b' || param[0] == 'B')
		c = BLACK;
	else
		return gtp_error("invalid color"), -1;

	if (c != goban.turn)
		return gtp_error("invalid color"), -1;

	if (strcasecmp(s, "pass") == 0)
	{
		game_Pass();
		gtp_success("");
		return -1;
	}

	if (strlen(s) < 2 || strlen(s) > 3)
		return gtp_error("invalid vertex"), -1;


	x = (toupper(s[0])-64);
	y = (s[1]-48);
	if (s[2]!=0)
		y = (y*10) + (s[2]-48);

	if (x >= 10) x--;	// skip j

	if (x<1 || y<1 || (unsigned)x> state.boardsize || (unsigned)y>state.boardsize)
		return gtp_error("vertex out of bounds"), -1;


	if (!game_Play(POS(x, y)))
		return gtp_error("illegal move"), -1;

	gtp_success("");

	return -1;
}

int genmove(const char *param)
{
	unsigned int pos;
	int c;
	char s[4];

	if (!param)
		return gtp_error("parameter required"), -1;

	if (param[0] == 'w')
		c = WHITE;
	else if (param[0] == 'b')
		c = BLACK;
	else
		return gtp_error("invalid color"), -1;

	if (c != goban.turn)
		return gtp_error("invalid color"), -1;

	pos = game_Genmove();

	if (!pos)
		return gtp_success("pass"), -1;

	s[0] = (char)X(pos) + 64;
	if ((char)s[0] >= 'I')
		s[0]++;
	if (Y(pos) >= 9)
	{
		s[1] = (char)((Y(pos)) / 10) + 48;
		s[2] = (char)((Y(pos)) % 10) + 48;
		s[3] = 0;
	}
	else
	{
		s[1] = (char)(Y(pos)) + 48;
		s[2]=0;
	}
	gtp_success(s);

	return -1;
}

int showBoard(const char *param)
{
	outboard(stdout);

	gtp_success("");
	return -1;
}

int clearBoard(const char *param)
{
	goban_init();
	program_init();

	gtp_success("");
	return -1;
}

int komi(const char *param)
{
	if (!param)
		return gtp_error("parameter required"), -1;

	rules.komi = atof(param);

	gtp_success("");
	return -1;
}

int boardSize(const char *param)
{
	int newsize;

	if (!param)
		return gtp_error("parameter required"), -1;

	newsize = atoi(param);

	if (newsize >= MAX_BOARDSIZE
		|| newsize < MIN_BOARDSIZE
		|| (newsize % 2) != 1)
		return gtp_error("invalid boardsize"), -1;

	if ((int)state.boardsize != newsize)
	{
		state.boardsize = (unsigned int)newsize;
		goban_init();
		engine_init();
		program_init();
	}

	gtp_success("");
	return -1;
}

int query(const char *param)
{
	//stats_show(param, fileGTP);
	engine_show(param, fileGTP);
	gtp_success("");
	return -1;
}

int playSeq(const char *param)
{
	goban_playstring(param);
	goban_backup();
	gtp_success("");
	return -1;
}

int try_variation(const char *param)
{
	clock_t time;
	int n, m=0, count = 1;
	if (param)
	{
		char *p = strchr(param, ' ');
		if (p != NULL)
		{
			int x,y;
			*p = 0;
			p++;
			x = (toupper(p[0])-64);
			y = (p[1]-48);
			if (p[2]!=0)
				y = (y*10) + (p[2]-48);

			if (x >= 10) x--;	// skip j
			m = POS(x,y);
		}
		count = atoi(param);
	}
	if (count==0) count=1;

	time = clock();
	for(n=0; n< count; n++)
    {

		engine_tryvariation(m);
       if (n % 1000 == 999)
        engine_show("14 d4", fileGTP);

    }

	time = clock() - time;

	out(fileGTP, "\nDone (%d @ %p) in %d secs @ %d tries/sec\n", 
            count, m,
			(int)((double)time / CLOCKS_PER_SEC),
			(int)((double)count / ((double)time / CLOCKS_PER_SEC))
	);

	return -1;
}

