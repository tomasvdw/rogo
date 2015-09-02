#include <stdio.h>
#include "inttypes.h"

#include "rogo.h"
#include "game.h"
#include "state.h"
#include "stats.h"
#include "rand.h"
#include "program.h"
#include "goban.h"
#include "timing.h"
#include "utils.h"

#include "engine.h"
#include "hash.h"

// game handling

void game_Pass()
{
	goban_pass();
	goban_backup();
	return;
}


extern int rounds;

int counter;

// Cretae a move for the current player
unsigned int game_Genmove()
{
	int bestmove;

	engine_init();
	hash_init();

	//playout a game
	for(counter=0;counter< rounds; counter++)
	{

		engine_tryvariation(0);

	}

	bestmove = engine_getbestmove();

	if (bestmove != 0)
	{
		int s = goban_move(bestmove);
		goban_backup();
		return s;
	}
	else
	{
		goban_pass();
		goban_backup();
		return 0;
	}



//		goban_pass();
//
//	goban_backup();
//	return pos;
	
}


int game_Play(unsigned int pos)
{
	int result = goban_move(pos);

	goban_backup();
	return result;
}


