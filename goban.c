//goban.c

#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include "inttypes.h"

#include "rogo.h"
#include "rand.h"
#include "goban.h"
#include "state.h"
#include "utils.h"


//#define FINDLIFE 0

struct Goban goban;
static struct Goban goban_bak;

static unsigned long long color_hash;

// array of relative position indices
static const int rel[] = { 1, MAX_BOARDSIZE, -1, -MAX_BOARDSIZE,
						1 + MAX_BOARDSIZE, -1 + MAX_BOARDSIZE,
						1 - MAX_BOARDSIZE, -1 - MAX_BOARDSIZE };

void fullchecklife(struct Intersection *i);

static void switchturn()
{
	//unsigned int p;
	int x,y;
	goban.turn = -goban.turn;
	goban.move++;

	assert(CURCOLOR == (1-goban.turn) / 2);

	// our hash is "situational" and should change
	// with the color
	goban.hash ^= color_hash;

	for(x=1; x <= state.boardsize; x++)
		for(y=1; y <= state.boardsize; y++)
		{
			unsigned int p = POS(x,y);
			if (goban.is[p].state)
				goban.is[p].state ^= 0x2;
		}

}


void goban_init()
{
	unsigned int p;

	color_hash = rnd64();
	goban.hash = rnd64();

	goban.ko = 0;
	goban.score = 0;
	goban.turn = BLACK;
	goban.move = 0;
	goban.empty = state.boardsize * state.boardsize;
	goban.lastmove = 0;

	memset(goban.is, 0, sizeof(goban.is));

	for (p = 0; p < POSCOUNT; p++)
	{
		if (X(p) == 0 || Y(p) == 0 || X(p) > state.boardsize || Y(p) > state.boardsize)
		{
			/* set border-bit */
			goban.is[p].state |= BORDER;
		}

		/* set lib-count */
		goban.is[p].libs = 4;
		if (X(p) == 1 || X(p) == state.boardsize)
			goban.is[p].libs--;
		if (Y(p) == 1 || Y(p) == state.boardsize)
			goban.is[p].libs--;

		goban.is[p].hash[0] = rnd64();
		goban.is[p].hash[1] = rnd64();
	}

	goban_backup();
}

int goban_getOwner(int pos)
{
#ifdef FINDLIFE
	if ((goban.is[pos].state & LIFE) == 0)
		return 0;
#endif

	if (goban.is[pos].state & STONE)
	{
		return (goban.is[pos].state & MINE) ? goban.turn : goban.turn * -1;
	}
	else if (goban.is[pos].libs == 0)
	{
		int mine = -1;
		int opp = -1;
		int n;
		struct Intersection *i = &(goban.is[pos]); // shortcut
		for(n=0; n < 4; n++)
		{
			struct Intersection *nb = (i+rel[n]); // store neighbour
			if (nb->state & BORDER)  
				continue;
			if ((nb->state & STONE) == 0)
				return 0;
			if (nb->state & MINE)
				opp = 0;
			else
				mine = 0;
		}
		if (mine)
			return goban.turn;
		else if (opp)
			return goban.turn * -1;
		else 
			return 0;
	}
	else 
		return 0;

}

void goban_pass()
{
	switchturn();
}

unsigned int goban_move(unsigned int pos)
{
	int totcapture = 0;
	int n, m;
	struct Intersection *i = &(goban.is[pos]); // shortcut
	int orgscore = goban.score;

	// reset ko
	goban.ko = 0;

	goban.lastmove = pos;

	if (pos==0)
	{
		switchturn();
		return 0;
	}

	// on board?
	assert(!(X(pos) < 1 || X(pos) > state.boardsize || Y(pos) < 1 || Y(pos) > state.boardsize));

	/* on empty intersection? */
	assert(ISLEGAL(pos));

	// filling ones own real eye is not allowed
	if (i->state == 0)
	{
		for(n=0; n < 4; n++)
		{
			struct Intersection *nb = (i+rel[n]); // store neighbour
			if ((nb->state & BORDER) == 0 
				&& (nb->state & (STONE|MINE)) != (STONE|MINE))
				break;
		}
		if (n==4)
		{
			// straight neighbours are mine or border
			// check corners
			int onborder=0, nofriendly=0;
			for(; n < 8; n++)
			{
				struct Intersection *nb = (i+rel[n]); // store neighbour
				if (nb->state & BORDER)
					onborder=1;
				else if ((nb->state & (MINE|STONE)) != (STONE|MINE))
					nofriendly++;


			}
			if (nofriendly + onborder <= 1)
			{
				// filling own eye =>
				// make move a pass
				switchturn();
				goban.lastmove = 0;
				return 0;
			}
		}
	}
	


	// neighbours will have less libs
	for(n=0; n < 4; n++)
		(i+rel[n])->libs--;

	// perform captures
	for(n=0; n < 4; n++)
	{
		struct Intersection *nb = (i+rel[n]); // store neighbour
		struct Intersection *p = nb;

		if (OPPSTONE(nb->state))
		{
			do
			{
				if (p->libs)
					break;
				p = p->next;
			} while (p != nb);

			// entire string has no libs?
			if (!p->libs)
			{
				// perform capture
				do
				{
					totcapture++;
					goban.score += goban.turn;
					for(m=0; m < 4; m++)
						(p+rel[m])->libs++;

					goban.hash ^= p->hash[1 - CURCOLOR];

					p->state = 0;
					p->eyes = 0;

					p = p->next;
				} while (p != nb);
			}
		}

	}

	goban.empty += totcapture;

	// update the board
	i->state = STONE | MINE;
	i->eyes = 0;
	i->next = i;
	goban.hash ^= i->hash[CURCOLOR];
	goban.score += goban.turn;
	goban.empty--;

	// insert new stone into string-list
	for(n=0; n < 4; n++)
	{
		struct Intersection *nb = (i+rel[n]); // store neighbour

		if (MYSTONE(nb->state))
		{
			// nb is now attached to new stone (i)

			// iterate i's path to see if nb is already in the loop
			struct Intersection *p = i->next;
			while (p != i && p != nb)
				p = p->next;

			if (p == nb)
				continue; // already attached

/*			if (nb->eyes)
			{
				p = i;
				do
				{
					p->eyes += nb->eyes;
					p = p->next;
				}
				while (p != i);
			}
			*/
			if (nb->state & LIFE)
				i->state |= LIFE;

			// insert i in nb's loop
			p = nb->next;
			nb->next = i->next;
			i->next = p;

//			nb->eyes = nb->next->eyes;
		}
	}

	// check for suicide
	if (!i->libs)
	{
		struct Intersection *p = i;
		while (!p->libs)
		{
			p = p->next;
			if (p == i)
			{
				// suicide
				do
				{
					goban.score -= goban.turn;
					for(n=0; n < 4; n++)
						(p+rel[n])->libs++;

					goban.hash ^= p->hash[CURCOLOR];
					p->state = 0;
					goban.empty++;

					p = p->next;
				} while (p!=i);

				break;
			}
		}
	}

#ifdef FINDLIFE
	if (i->libs <= 2)
		fullchecklife(i);
#endif
/*
	// check if an eye is made with this move
	if (goban.turn * (goban.score - orgscore) > 0) // only if no suicide 
	{
		for(n=0; n < 8; n++)
		{
			struct Intersection *eye = (i+rel[n]); // store neighbour
			if (eye->state > 0 || eye->libs > 0)
				continue;

			for(m = 0; m < 4; m++)
			{
				struct Intersection *nb = (eye+rel[m]); // store neighbour
				if ((nb->state & BORDER) == 0 
					&& (nb->state & (STONE|MINE)) != (STONE|MINE))
					break;
			}
			if (m==4)
			{
				// straight neighbours are mine or border
				// check corners
				int onborder=0, nofriendly=0;
				for(; m < 8; m++)
				{
					struct Intersection *nb = (eye+rel[m]); // store neighbour
					if (nb->state & BORDER)
						onborder=1;
					else if ((nb->state & (MINE|STONE)) != (STONE|MINE))
						nofriendly++;
				}
				if (nofriendly + onborder == 1) 
				{
					// we've made an eye
					// add eye count of string nb
					struct Intersection *p = i;
					do
					{
						p->eyes++;
						p = p->next;
					}
					while (p != i);
				}
			}
		}
	}
*/

	// ko if
	// 1) # captures = 1
	// 2) # of empty neighbours = 1
	// 3) # of friendly neighbours = 0

	if (totcapture == 1)
	{
		int en=0; // empty neighbours

		for (n=0; n<4;n++)
		{
			struct Intersection *nb = (i+rel[n]); // store neighbour

			if ((nb->state & (BORDER|STONE)) == 0)
			{
				if (++en == 1)
				{
					goban.ko = nb;
				}
				else
				{
					goban.ko = 0;
					break;
				}
			}
			else if (MYSTONE(nb->state))
			{
				// friendly neighbour => no ko
				goban.ko = 0;
				break;
			}
		}
	}

	

	// single stone suicide = pass
	if (goban.score == orgscore)
	{
		switchturn();
		goban.lastmove = 0;
		return pos;
	}
	else
	{
		switchturn();
		return pos;
	}
}

// check if eye2 is an eye and all neighbours are connected to eye
int checkeye(struct Intersection *eye2, struct Intersection *eye)
{
	int n;

	for(n=0; n < 4; n++)
	{
		struct Intersection *nb = (eye2+rel[n]); // store neighbour	
		struct Intersection *p = nb;

		// border ? this nb is OK
		if (nb->state & BORDER)
			continue;

		// oppstone ? no live
		if (!(nb->state & MINE))
			return 0;

		do
		{
			if (p+rel[0] == eye 
				|| p+rel[1] == eye 
				|| p+rel[2] == eye 
				|| p+rel[3] == eye)
			{
				break;
			}

			p = p->next;
		}
		while (p!=nb);

		if (p==nb)
		{
			// this eye2 is not connected to eye
			return 0;
		}

	}
	return -1;
}

int checklife(struct Intersection *eye)
{
	struct Intersection *othereye[4] = { 0,0,0,0};

	// check four borders
	int n,m;

	for(n=0; n < 4; n++)
	{
		struct Intersection *nb = (eye+rel[n]); // store neighbour
		struct Intersection *p;

		// border ? this nb is OK
		if (nb->state & BORDER)
			continue;

		// oppstone ? no live
		if (!(nb->state & MINE))
			return 0;

		// find another eye
		p = nb;
		do
		{
			for(m=0; m < 4; m++)
			{
				struct Intersection *eye2;
				eye2 = p+rel[m];
				if (eye2!=eye &&
					eye2->libs == 0 && 
					eye2->state == 0)
				{
					// check if all borders of this eye2 are connet
					if (checkeye(eye2, eye))
					{
						othereye[n] = eye2;
						break;
					}
				}
			}
			if (m <4) // we had success?
				break;

			p=p->next;
		}
		while(p!=nb);

		if (p==nb)
			// no second eye found
			return 0;
	}

	// live found, mark it
	eye->state |= LIFE;

	for(n=0; n < 4; n++)
	{
		struct Intersection *nb = (eye+rel[n]); // store neighbour
		struct Intersection *p = nb;
		
		if (nb->state & BORDER)
			continue;
		if (othereye[n]) 
			othereye[n]->state |= LIFE;
		do
		{
			p->state |= LIFE;
			p = p->next;
		}
		while (nb!=p);
		
	}
	return -1;
}

void fullchecklife(struct Intersection *i)
{
	struct Intersection *p = i;

	do {
		int n;
		for (n=0; n < 4; n++)
		{
			struct Intersection *nb = (p+rel[n]); // store neighbour

			if (nb->libs == 0 && nb->state == 0)
			{
				if (checklife(nb))
					return;
			}
	
		}

		p = p->next;
	}
	while (p != i);

}
	



void	goban_backup()
{
	memcpy(&goban_bak, &goban, sizeof(goban));
}

void	goban_restore()
{
	memcpy(&goban, &goban_bak, sizeof(goban));

}

void goban_playstring(const char * s)
{
	char buf[2048];
	char * p;
	strcpy(buf,s);
	p = strtok(buf, " ");
	while (p)
	{
		int x = toupper(p[0]) - 64;
		int y = p[1] - 48;
		int pos = POS(x,y);
		if (p[0]=='P' && p[1]=='A')
			pos = 0;

		out(fileGTP, "Playing %p\n", pos);
		goban_move(pos);
//		outboard(fileGTP);
		p = strtok(NULL, " ");
	}
}
