int state_init(int argc, char *argv[]);

enum Mode
{
	PRACTICE,
	PLAY,
};

struct State
{
	// general params
	unsigned int totalmem;
	unsigned int mode;
	unsigned int boardsize;
	int console;
	int quickplay;
	int verbose;
	
};
extern struct State state;
