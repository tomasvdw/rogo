// rules.c

#include "inttypes.h"

#include "rogo.h"

#include "rules.h"
#include "state.h"
#include "goban.h"


struct Rules rules;

void rules_init()
{
}


// Should override to check if a move is
// legal in current ruleset
int rules_islegal(unsigned int pos)
{
	return -1;

}

