

#define MAX_SEKI_INTERSECTIONS 6

#define POS(x, y)	((x) | ((y) << 5))
#define X(pos)		((pos) & 0x1F)
#define Y(pos)		(((pos) >> 5) & 0x1F)

#define POSCOUNT	(MAX_BOARDSIZE*MAX_BOARDSIZE)
#define BORDER 1
#define MINE 2
#define STONE 4
#define LIFE 8


#define OPPSTONE(state) ( ((state) & STONE) && (((state) & MINE) == 0) )
#define MYSTONE(state) ( ((state) & STONE) && ((state) & MINE) )

int		goban_islegalmove(unsigned int pos);
void	goban_init();
unsigned int		goban_move(unsigned int pos);
void 	goban_pass();
void	goban_backup();
void	goban_restore();
void	goban_dump(char *buffer);
void	goban_playstring(const char * s);
int 	goban_getOwner(int pos);


struct Intersection
{
	unsigned int state;
	unsigned int libs;
	unsigned int eyes;
	//int emptyPtr;
	unsigned long long hash[2];
	struct Intersection *next;
};

struct Goban
{
	struct Intersection is[POSCOUNT];
	struct Intersection *ko;

	int turn;
	int score;
	int move;
	int empty;
	int lastmove;
	unsigned long long hash;
};

#define CURCOLOR (goban.move & 1)

extern struct Goban goban;

#define ISLEGAL(pos) (&(goban.is[pos]) != goban.ko && ((goban.is[pos].state & (STONE|BORDER|LIFE)) == 0))

