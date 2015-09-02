
// main header




#define NAME "rogo"
#define VERSION "0.1"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define FILE_VERSION_MAGIC (('R'<<24)+('G'<<16)+('D'<<8)+'1')

#define MAX_BOARDSIZE 32
#define MIN_BOARDSIZE 3


#define SAVE_DELAY (10*60)


#define BLACK 1
#define WHITE -1




extern int signalSTOP;
extern int signalBREAK;

#ifdef WIN32
#define inline _inline
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

#include "inttypes.h"
