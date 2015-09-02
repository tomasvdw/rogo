
#define _USE_MATH_DEFINES

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include "inttypes.h"

#include "rogo.h"
#include "engine.h"
#include "utils.h"
#include "state.h"
#include "goban.h"
#include "program.h"
#include "rand.h"
#include "timing.h"
#include "hash.h"



#define USE_TARGET NODETYPE_TARGET

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

#ifndef EXAGGERATE_RATE
#define EXAGGERATE_RATE 5
#endif


#ifndef MAXDEPTH
#define MAXDEPTH 100
#endif

#ifndef UCT_K
#define UCT_K 2 
#endif

#ifndef UPDATE_SPECIAL_RATE
#define UPDATE_SPECIAL_RATE 1
#endif

#ifndef MIN_SPECIAL_VISITS
#define MIN_SPECIAL_VISITS 2
#endif

#ifndef ADD_RANDOM
#define ADD_RANDOM 0.02
#endif

#define LOG_MOVES_RATE 1

int cc=1;

int mode = 0;

// general stats for a pos
struct PosStat {
	int total;
	int won;
	int lost;
};

struct PosStat pos_stat[POSCOUNT];

// the stats for a single move 
// in a single position
struct MoveInfo {
	double wins;
	double visits;
	double winrate;
	double parent_visits;
	double uct;
};

static int tries = 0;
static int success = 0;

static int target;


int bestmove;

void engine_init()
{
	hash_init();
	srand((unsigned)time(NULL));
	tries = 0;
	success = 0;

	memset(pos_stat, 0, sizeof(pos_stat));
}



// makes high winrate higher and low winrate lower
double exaggerate_winrate(double winrate, int count) {
	int n;
	for (n=0; n < count; n++) {
		winrate = sin((winrate - 0.5) * M_PI) / 2.0 + 0.5;
	}
	return winrate;
}

// determine stability
// 1 = always win/lose
// 0 = varable
static double stab(int pos)
{
	double min = (double)MIN(pos_stat[pos].won, pos_stat[pos].lost);
	double max = (double)MAX(pos_stat[pos].won, pos_stat[pos].lost);
	return min/max;
}

// classic utc
static double getuct(double winrate, int visits, int parent_visits, int nodetype)
{
   return winrate + sqrt((log((double)parent_visits+1)) 
   / (UCT_K * (double) visits+1));
}

// gets the currently determined best-move from the hashtables
int engine_getbestmove() {
	int bestmove = 0;
	double bestwinrate = 0;
	int x,y;
	for(y = state.boardsize; y >= 1; y--)
	{
		for (x = 1; x <= state.boardsize; x++)
		{
			struct Node *node = hash_get_node(hash_get(
				POS(x,y), 0, goban.hash, 
				NODETYPE_POS|NODETYPE_MOVE|NODETYPE_SPECIAL));
			double winrate = (double) node->wins / (double) node->visits;
			if (winrate > bestwinrate)
			{
				bestwinrate = winrate;
				bestmove = POS(x,y);
			}
		}
	}

	return bestmove;
}

// move_info should contain parentvisits; others are filled in
void get_move_value(int move, struct MoveInfo *move_info, int nodetype)
{
	uint64_t hash;
	struct Node * node;
	if (move_info->parent_visits == 0)
	{
		move_info->uct = 0;
/*
		hash = hash_get(move, target, goban.hash, nodetype);
		node = hash_get_node(hash);

		assert(node->wins == 0);
		assert(node->visits == 0);
*/
		return;
	}

	hash = hash_get(move, target, goban.hash, NODETYPE_POS|USE_TARGET|NODETYPE_MOVE);
	node = hash_get_node(hash);

	move_info->wins = node->wins;
	move_info->visits = node->visits;
	if (move_info->visits > 0)
		move_info->winrate = (double)move_info->wins / ((double)move_info->visits);
	else
		move_info->winrate = 0;


	//if (move_info->parent_visits > state.boardsize * state.boardsize * 10)
    if (0)
	{
		hash = hash_get(move, target, goban.hash, NODETYPE_POS|NODETYPE_MOVE|NODETYPE_SPECIAL);
		node = hash_get_node(hash);

		if (node->visits > 0)
			move_info->winrate = (double)node->wins / ((double)node->visits+1);
	}



	//assert(move_info->visits <= move_info->parent_visits);

	move_info->uct = getuct(move_info->winrate, move_info->visits, move_info->parent_visits, nodetype);


	if (0)
	{
		// get the winrate from the special node
		hash = hash_get(move, target, goban.hash, nodetype);
		node = hash_get_node(hash);

		if (node->visits == 0)
		{
			move_info->wins = 0;
			move_info->visits = 0;
			move_info->winrate = 0.0;
		}
		else
		{
			move_info->winrate = (double)node->wins / ((double)node->visits+1);
			hash = hash_get(move, target, goban.hash, 
					NODETYPE_POS|NODETYPE_TARGET|NODETYPE_MOVE);
			node = hash_get_node(hash);
			move_info->wins = node->wins;
			move_info->visits = node->visits;
		}

		move_info->uct = getuct(move_info->winrate, move_info->visits, move_info->parent_visits, nodetype);
	}
}



// get a current best move to try in a try-variation
int get_best_move(struct Node *parent, int nodetype, FILE *f) {
	int x,y;
	double bestuct = 0;
	int bestmove = 0;
	static double cache_uct[POSCOUNT];
	static int cache_status = 0;

	for(y=state.boardsize; y >= 1; y--)
	{
		for(x=1; x <= state.boardsize; x++)
		{
			struct MoveInfo move_info = { 0 };

			if (!ISLEGAL(POS(x,y)))
			{
				if (goban.is[POS(x,y)].state & STONE)
					if (goban.is[POS(x,y)].state & MINE)
						out(f, " *** ");
					else
						out(f, " ### ");
				else
						out(f, " ... ");


				continue;
			}

			if (0 && (nodetype == (NODETYPE_TARGET | NODETYPE_MOVE)) && cache_status) {
				move_info.uct = cache_uct[POS(x,y)];
			}
			else
			{

				move_info.parent_visits = parent->visits;

				get_move_value(POS(x,y), &move_info, nodetype);

				//cache_uct[POS(x,y)] = move_info.uct;


				out(f, "%4.1f ", move_info.winrate * state.boardsize * state.boardsize);
			}
				
			move_info.uct += rnd_d() / 1000000;

			if (move_info.uct > bestuct)
			{
				bestmove = POS(x,y);
				bestuct = move_info.uct;
			}
		}
		out(f, "\n");
	}

	
	cache_status = (nodetype == (NODETYPE_TARGET | NODETYPE_MOVE));
	//cache_status = 0;

	return bestmove;
}



void update_special_node(int move) {
	// store special node
	int x,y;
	double winrate = 0;
	double c = 0;
	for(y = state.boardsize; y >= 1; y--)
	{
		for(x = 1; x <= state.boardsize; x++)
		{		
			struct Node * nodeThis = 
				hash_get_node(hash_get(move, POS(x,y), goban.hash,
				NODETYPE_POS|NODETYPE_TARGET|NODETYPE_MOVE));

            struct Node *nodeParent =
               hash_get_node(hash_get(0, POS(x,y), goban.hash, NODETYPE_POS|NODETYPE_TARGET));
		
			if (nodeThis->visits)
			{
				double wr = ((double)nodeThis->wins / (double)nodeThis->visits);
				double wrPar = ((double)nodeParent->wins / (double)nodeParent->visits);
                /*double stb = stab(POS(x,y));
				stb = pow(stb, 2);
				stb=1;
				c+=stb;*/
                c += 1;
				wr = exaggerate_winrate(wr, EXAGGERATE_RATE) ;
				wrPar = 1;//exaggerate_winrate(wrPar, EXAGGERATE_RATE) ;

                winrate += wr / wrPar;//((wr - wrPar) + 1) / 2.0;// 0...1 - get_average_rating(move(x,y));
				//out(fileGTP, "Winrate for move %p, at target %p = %f - %f\n",
				//		move, POS(x,y), wr, wrPar);
			}
		}
	}

	winrate /= c;
				//out(fileGTP, "Winrate for move %p, %f\n", move, POS(x,y), winrate);

	hash_set_node_special(hash_get(move, 0, goban.hash, 
				NODETYPE_POS|NODETYPE_MOVE|NODETYPE_SPECIAL), winrate);

}

// recursive function to try a game
int trygame(int depth, int doUpdate) {

	struct Node *parent = (struct Node*)0;
	uint64_t goban_hash = goban.hash;
	int myturn = goban.turn;
	int result;
	int pos;
	int win;
	int x,y;
	int haspoints;
	uint64_t hash;



	if (depth == MAXDEPTH)
		return goban_getOwner(target);

	// see if we can use nt3
	hash = hash_get(0, target, goban.hash, NODETYPE_POS |USE_TARGET);
	parent = hash_get_node(hash);

	pos = get_best_move(
			parent,
			(NODETYPE_POS| USE_TARGET|  NODETYPE_MOVE),
			NULL
	);

	if (parent->visits >= MIN_SPECIAL_VISITS
			&& parent->visits % UPDATE_SPECIAL_RATE == 0) 
	{
		update_special_node(pos);
	}

    if (mode == 1)
    {
       int p;
       for(y = state.boardsize; y >= 1; y--)
           for(x = 1; x <= state.boardsize; x++)
               update_special_node(POS(x,y));

        p = engine_getbestmove();
        if (p) pos = p;
    }

	goban_move(pos);

#if LOG_MOVES_RATE > 0
	if (depth == 0 && tries % LOG_MOVES_RATE == 0)
	{
		out(fileGTP, "Target %p ", target);
	}
	if (depth < 10 && tries % LOG_MOVES_RATE == 0)
		out(fileGTP, "%d%p ", 0, pos);
#endif

	result = trygame(depth+1, parent->visits > 0);

#if LOG_MOVES_RATE > 0
	if (depth == 0 && tries % LOG_MOVES_RATE == 0)
       out(fileGTP, " res=%d, %r\n", result, goban.score);
#endif

	haspoints = 0;
	for(y = state.boardsize; y >= 1; y--)
		for(x = 1; x <= state.boardsize; x++)
		{
			int targ = POS(x,y);
            if (targ != target) continue;
			result = goban_getOwner(targ);
			if (!result) continue;
			haspoints = 1;
		}
    if (!haspoints)
    {
       //out(fileInfo, "Failure getOwner(target)=%d\n", goban_getOwner(POS(7,5)));
       //outboard(fileInfo);
    }
	if (haspoints && doUpdate)
		for(y = state.boardsize; y >= 1; y--)
			for(x=1; x <= state.boardsize; x++)
			{
				int targ = POS(x,y);
                  if (targ != target) continue;
				result = goban_getOwner(targ);
				if (!result) continue;

				win = (result == myturn ? 1 : 0);

				// store nt 3
				hash_set_node(hash_get(pos, targ, goban_hash,
						NODETYPE_POS|NODETYPE_TARGET|NODETYPE_MOVE), win);
				hash_set_node(hash_get(pos, targ, goban_hash,
						NODETYPE_POS|NODETYPE_TARGET), win);

			}

    // update target-agnostic nodes (generic uct)
    if (haspoints && doUpdate && goban.score)
    {
       win = (myturn * goban.score > 0 ? 1 : 0);
       hash_set_node(hash_get(pos, 0, goban_hash,
               NODETYPE_POS|NODETYPE_MOVE), win);
       hash_set_node(hash_get(pos, 0, goban_hash,
               NODETYPE_POS), win);
    }

	if (haspoints && depth == 0)
	{

		for(y = state.boardsize; y >= 1; y--)
			for(x=1; x <= state.boardsize; x++)
			{
				int targ = POS(x,y);
				result = goban_getOwner(targ);
				pos_stat[targ].total++;

				if (result == myturn)
					pos_stat[targ].won++;
				else if (result == -myturn)
					pos_stat[targ].lost++;

			}
		//outboard(fileGTP);
		//update_special_node(pos);
		return 1;
	}
	
	return 0;


	return result;
}


int engine_tryvariation(int forcetarget)
{

	int n;
	if (forcetarget)
		target = forcetarget;
	else
		target = POS(1+ rand() % state.boardsize, 1+ rand() % state.boardsize);
	//target = POS(7,3);
				
	n = trygame(0, -1);
	if (n)
		success++;
	tries++;
	goban_restore();

	return n;
}


// special dbg funcs
#include "engine_dbg.h"

