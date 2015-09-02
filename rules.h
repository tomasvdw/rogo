

/* ruleset in used; only in use by "outer" game_ functions
*  engine uses simplified rules */

int rules_islegal(unsigned int pos);


struct Rules
{

	int allow_suicide;

	double komi;

};

extern struct Rules rules;

