

/* scanf/printf variants */

int in(const char *pattern, char *input, ...);
void out(FILE *f, char *format, ...);
void outboard(FILE *f);

extern FILE *fileInfo, *fileGTP;
extern FILE *fileDbg[5];

#define POPBIT(r) ((r)>>=1,(r)&1)

