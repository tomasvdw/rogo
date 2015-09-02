

#include <stdlib.h>
#include <time.h>

#include "inttypes.h"

#include "rand.h"

/* defines and macros */

/*
**      takes bits 30..15 of each long and glues them together
*/
#define V(a,b)  ((((a) << 1) & 0xFFFF0000) | (((b) >> 14) & 0x0000FFFF))

/* global variables */

/*
**      these two are visible to the outside world if they want
*/
long _i_seed1_ = 1L;    /* current value of first generator */
long _i_seed2_ = 1L;    /* current value of second generator */

/* static global variables */

/*
**      these are all internal values
*/
static long _i_prim1_ = 2147483563L;
static long _i_prim2_ = 2147483399L;

static long _i_root1_ = 40014L;
static long _i_root2_ = 40692L;

/*
**      If you should choose to change primes or roots,
**      please be sure that these conditions hold:
**              (_i_quo?_ > _i_rem?_)
**              (_i_quo?_ > 0)
**              (_i_rem?_ > 0)
*/
static long _i_quo1_ = 53668L;  /* _i_prim1_ / _i_root1_ */
static long _i_quo2_ = 52774L;  /* _i_prim2_ / _i_root2_ */

static long _i_rem1_ = 12211L;  /* _i_prim1_ % _i_root1_ */
static long _i_rem2_ =  3791L;  /* _i_prim2_ % _i_root2_ */


/* seeds the random engine */
void rnd_init()
{
	_i_seed1_=(long)time((time_t*)0)%_i_prim1_;
	_i_seed2_=(long)time((time_t*)0)%_i_prim2_;
}

unsigned long long rnd64() {
	return ((unsigned long long) rnd() << 32) | rnd();
}

double rnd_d() {
	return (double)rnd() / UINT32_MAX;
}

/* Returns a random 32 bits number */
unsigned long rnd()
{
  ldiv_t temp;
  long alpha, beta;


  /*
  **  implement multiplication and modulo in such a way that
  **  intermediate values all fit in a long int.
  */
  temp = ldiv(_i_seed1_, _i_quo1_);
  alpha = _i_root1_ * temp.rem;
  beta = _i_rem1_ * temp.quot;

  /*
  **    normalize the intermediate values
  */
  if (alpha > beta) {
    _i_seed1_ = alpha - beta;
  } else {
    _i_seed1_ = alpha - beta + _i_prim1_;
  }

  temp = ldiv(_i_seed2_, _i_quo2_);
  alpha = _i_root2_ * temp.rem;
  beta = _i_rem2_ * temp.quot;

  if (alpha > beta) {
    _i_seed2_ = alpha - beta;
  } else {
    _i_seed2_ = alpha - beta + _i_prim2_;
  }

  return (unsigned long)( V(_i_seed1_, _i_seed2_) );

}


