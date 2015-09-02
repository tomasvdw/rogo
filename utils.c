#define _CRT_SECURE_NO_DEPRECATE

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "inttypes.h"

#include "rogo.h"
#include "state.h"
#include "utils.h"
#include "goban.h"

FILE *fileInfo, *fileGTP;

FILE *fileDbg[5];

/* output formatted string to file
   printf formatting supported
   plus: %p for position
         %r for result
		 %t for turn
*/
void out(FILE *f, char *format, ...)
{
	char buf[15];
	char *p = format;
	va_list params;

	if (!f)
		return;

	va_start( params, format);

	while (*p)
	{
		if (*p == '%')
		{
			char *q=p;
			char c;
			do
			{
				p++;
				c = (*p);

				if (c == 's')
				{
					fputs(va_arg(params, char *), f);
					break;
				} 
				else if (c == '%')
				{
					fputc('%', f);
					break;
				}
				else if (strchr("diouxX", c))
				{
					strncpy(buf, q, p-q+1);
					buf[p-q+1] = 0;
					fprintf(f, buf, va_arg(params, int));
					break;
				}
				else if (strchr("ceEfgG", c))
				{
					strncpy(buf, q, p-q+1);
					buf[p-q+1] = 0;
					fprintf(f, buf, va_arg(params, double));
					break;
				}
				else if (c=='c')
				{
					strncpy(buf, q, p-q+1);
					buf[p-q+1] = 0;
					fprintf(f, buf, va_arg(params, int));
					break;
				}
				else if (c=='p')
				{
                    /* output given POS human readable */
					int pos = va_arg(params, int);
					char x[] = "?ABCDEFGHJKLMNOPQRSTUVWXYZ";
					if (pos)
						fprintf(f, "%c%d", x[X(pos)], Y(pos));
					else
						fprintf(f, "PASS");
					break;
				}
				else if (c=='t')
				{
					int turn = va_arg(params, int);
					if (turn==BLACK)
						fputs("BLACK", f);
					else if (turn==WHITE)
						fputs("WHITE", f);
					else
						assert(0);
					break;
				}
				else if (c=='r')
				{
                    /* output give score as a W/B score */
					int score = va_arg(params, int);
					if (score < 0)
						sprintf(buf, "W+%d", -score);
					else
						sprintf(buf, "B+%d", score);
					while (strlen(buf)<5)
						strcat(buf," ");
					fputs(buf,f);
					break;
				}

			} while(*p);

		}
		else
			fputc(*p, f);
		p++;
	}
	va_end(params);

	fflush(f);
}

int in(const char *pattern, char *input, ...)
{
	char *p = (char*)pattern;
	char *i = input;

	while(*p)
	{
		if (*i == 0)
			return 0; // input string shorter then pattern

		else if (*p == '%')
		{

		}
		else if (*p != *i)
			return 0; // difference found

		i++;
		p++;
	}
	
	return -1; // success
}

// parse a human readable pos (ie h4) to a POS
int scanpos(char **p)
{
   int x,y;

   x = (toupper(*p[0])-64);
   if (x >= 10) x--;	// skip j
   y = (*p[1]-48);
   if (*p[2]!=0 && *p[2] != ' ')
   {
       y = (y*10) + (*p[2]-48);
       (*p)++;
   }
   *p += 2;

   if (x < 1 || x > 19 || y < 1 || y > 19)
      return 0;
   else 
      return POS(x,y);
}

// dumps current board
void outboard(FILE *f)
{
	unsigned int x,y;

	if(!f)
		return;

	for(y = state.boardsize;y >= 1;y--)
	{
		fprintf(f, "%2d  ", y);
		for(x=1;x<=state.boardsize;x++)
		{
			if (goban.is[POS(x,y)].state & STONE)
			{

				if (((goban.is[POS(x,y)].state & MINE)  && goban.turn == BLACK)
					|| ((goban.is[POS(x,y)].state & MINE) == 0 && goban.turn == WHITE))
					fputc('X', f);
				else
					fputc('O', f);
			}
			else
				fputc('.', f);

		}
		for(x=1;x<=state.boardsize;x++)
		{
			fprintf(f," %2d%2d %2d ", goban.is[POS(x,y)].state, goban.is[POS(x,y)].libs, goban.is[POS(x,y)].eyes);



		}

		fputc('\n', f);
	}

	fputs("\n    ", f);
	for(x=1;x<=state.boardsize;x++)
	{
		const char xlabel[] = "?ABCDEFGHJKLMNOPQRSTUVWXYZ";
		fputc(xlabel[x], f);
	}
	fprintf(f, "\n    Score: %d,  empty: %d   moves: %d\n", goban.score, goban.empty, goban.move);


	fflush(f);
}

