

/* Single include for engine.c
*  with debugging functions
*/

void dumpmoves(int target, int nodetype, FILE *f);
void expect(const char *good_moves);
void engine_show(const char *paramin, FILE *f);


void dumpmoves(int target, int nodetype, FILE *f) {
	int x,y;

	for(y=state.boardsize; y >= 1; y--)
	{
		for(x=1; x <= state.boardsize; x++)
		{
			//struct MoveInfo move_info = { 0 };

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
			{
				struct Node *node = hash_get_node(hash_get(

						POS(x,y), target, goban.hash,
						nodetype));
				double winrate = (double) node->wins / (double) node->visits;

				out(f, "%4.1f ", winrate * state.boardsize * state.boardsize);

			}
		}
		out(f, "\n");
	}

	return ;
}


void expect(const char *good_moves);

void engine_show(const char *paramin, FILE *f) {
	int x,y;
	int nodetype;
    char buf[1024], *param = buf;
    strcpy(param, paramin);
	//struct Node *parent;

    out(f, "Query: %s", param);


	target=0;
    if (param && strncmp(param, "after", 5) == 0)
    {
       goban_backup();
       param += 6;
       if (*param == '"')
       {
          char *p;
          *param ++;
          p = strchr(param, '"');
          *p = 0;
          goban_playstring(param);
          param += strlen(param)+1;

       }
       else
       {
          char *p = param;
          //goban_move(scanpos(&p));
          param++;

       }
       engine_show(param, f);
       goban_restore();
       return;
    }
	if (param && strncmp(param, "expect", 6) == 0)
	{
		expect(param + 7);
		return;

	}
	else if (param && strncmp(param, "topres", 6) == 0)
    {
		char *p = strchr(param, ' ');
        goban_backup();
		if (p)
		{
			*p = 0;
			p++;

			x = (toupper(p[0])-64);
			y = (p[1]-48);
			if (p[2]!=0)
				y = (y*10) + (p[2]-48);

			if (x >= 10) x--;	// skip j

            goban_move(POS(x,y));
        }
        mode = 1;
        trygame(0, 0);
        outboard(fileGTP);
        goban_restore();
        mode = 0;

    }
	else if (param && strncmp(param, "posstat", 7) == 0)
	{
		int tot=0;
		for (y= state.boardsize; y >= 1; y--)
		{
			for(x=1; x <= state.boardsize; x++)
			{
				struct PosStat *p = pos_stat + POS(x,y);
				tot = p->total;
				out(fileGTP, "%5d/%5d ", p->won, p->lost);
			}
			out(fileGTP, "\n");
		}
		out(fileGTP, "\nTotal=%d\n", tot);
		return;
	}
	else if (param && strncmp(param, "uct", 3) == 0)
	{
		int wins, visits, parvisits;
		double uct;
		char *p = (char*)param;

		double winrate;
		p = strtok(p, " ");
		p = strtok(NULL, " ");
		wins = atoi(p);
		p = strtok(NULL, " ");
		visits = atoi(p);
		p = strtok(NULL, " ");
		parvisits = atoi(p);
		winrate = (double)wins / ((double)visits+1);
		uct = getuct(winrate, visits, parvisits, 0);

		out(f, "%3d/%3d/%3d/%.1f\n", wins, visits, 
					parvisits, uct);
		return;
	}
	else if (param && strncmp(param, "dump", 4) == 0)
	{
		FILE * ff = fopen("posdump.dat", "w");
		int yt, ym, xt,xm;

		for(xm = 1; xm <= state.boardsize; xm++)
		for(ym = 1; ym <= state.boardsize; ym++)
		for(xt = 1; xt <= state.boardsize; xt++)
		for(yt = 1; yt <= state.boardsize; yt++)
		{

			char c = '.';
			struct PosStat * ps = pos_stat + POS(xm, ym);
			struct Node * n1, *n2;


			if (goban.is[POS(xm, ym)].state & STONE)
			{
				if (goban.is[POS(xm, ym)].state & MINE)
					c = 'X';
				else
					c = 'O';
			}
			out(ff, "%d\t%d\t%d\t%d\t", xm, ym, xt, yt);
			out(ff, "%c\t", c);
			out(ff, "%d\t%d\t%d\t", ps->total, ps->won, ps->lost);


			n1 = hash_get_node(hash_get(POS(xm,ym), POS(xt,yt), goban.hash, NODETYPE_TARGET|NODETYPE_POS|NODETYPE_MOVE));
			n2 = hash_get_node(hash_get(POS(xm,ym), POS(xt,yt), goban.hash, NODETYPE_TARGET|NODETYPE_POS));

			out(ff, "%d\t%d\t%d\t%d\n",
					n1->wins, n1->visits, n2->wins, n2->visits);


		}

		fclose(f);

	}
	else if (param && strncmp(param, "node", 3) == 0)
	{
		int pos, target;
		char *p = (char*)param;
		struct Node *node;
		p = strtok(p, " ");
		p = strtok(NULL, " ");
		x = (toupper(p[0])-64);
		y = (p[1]-48);
		if (p[2]!=0) y = (y*10) + (p[2]-48);
		if (x >= 10) x--;	// skip j
		pos = POS(x,y);

		p = strtok(NULL, " ");
		x = (toupper(p[0])-64);
		y = (p[1]-48);
		if (p[2]!=0) y = (y*10) + (p[2]-48);
		if (x >= 10) x--;	// skip j
		target = POS(x,y);
		p = strtok(NULL, " ");
		nodetype = atoi(p);
		node = hash_get_node(hash_get(pos, target, goban.hash, nodetype));
		out(f, "Node pos=%p target=%p nt=%d, p/hash/wins/visits = %x/%x/%d/%d\n", 
				pos, target, nodetype, node, node->hash, node->wins, node->visits);
		

		return;
	}
		
	out(fileGTP, "Tries: %d, Success: %d (%d%%) nodes=%d\n"
			, tries, success, 100 * success / tries, hash_node_count);
		

	if (param)
	{
		char *p = strchr(param, ' ');
		if (p)
		{
			*p = 0;
			p++;

			x = (toupper(p[0])-64);
			y = (p[1]-48);
			if (p[2]!=0)
				y = (y*10) + (p[2]-48);

			if (x >= 10) x--;	// skip j

			target = POS(x,y);
			p = strchr(p, ' ');
			if (p) cc = atoi(p+1);
		}
		nodetype = atoi(param);
	}
	else
		return;

	out(f, "engine state target=%p nodetype=%d: \n\n", target, 
			nodetype);

	//parent = hash_get_node(hash_get(0, target, goban.hash, nodetype));

	dumpmoves(target, nodetype, fileGTP);

	
}



#define MAX_TIME 120

clock_t expect_once(const char *good_moves)
{
	clock_t good_since = 0;
	clock_t start = clock();
	clock_t log = clock();
	int count = 0;
	for(;;)
	{
		int m;
		char bestmove[3];
		int p;

		for(m=0; m < 100; m++)
			engine_tryvariation(0);
		
		count += m;

		p = engine_getbestmove();
		bestmove[0] = X(p) + 64;
		bestmove[1] = Y(p) + 48;
		bestmove[2] = 0;

		if (clock() - log > 5 * CLOCKS_PER_SEC)
		{
			out(fileGTP, "expecting %s, got %s good_since=%d current=%d\n",
				good_moves, bestmove,
				10 * good_since / CLOCKS_PER_SEC,
				10 * (clock() - start) / CLOCKS_PER_SEC);
			log = clock();
		}

		if (strstr(good_moves, bestmove))
		{
			clock_t extra = good_since;
			if (extra / CLOCKS_PER_SEC < 5) extra = 5 * CLOCKS_PER_SEC;
			if (extra / CLOCKS_PER_SEC > 60) extra = 60 * CLOCKS_PER_SEC;

			if (!good_since)
			{
				good_since = clock() - start;
			}
			else if (clock() - start > (good_since + extra))
			{
				// we're good
				//
				out(fileGTP, "Running at %d tries/sec\n", count / ((clock() - start) / CLOCKS_PER_SEC));
				return good_since;
			}
		}
		else
			good_since = 0; // reset

		if (good_since == 0 && clock() - start > MAX_TIME * CLOCKS_PER_SEC)
			break;
	}

	return 0;
}

void expect(const char *good_moves)
{
	int n;
	int times[3];

	for(n=0; n < 3; n++)
	{
		engine_init();
		{
			clock_t x = expect_once(good_moves);
			times[n] = 10 * x / CLOCKS_PER_SEC;
		}
	}

	out(fileGTP, "Result (%s): %d %d %d", good_moves, times[0], times[1], times[2]);
}
		
