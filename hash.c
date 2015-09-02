
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "rogo.h"
#include "goban.h"
#include "hash.h"
#include "rand.h"
#include "utils.h"

// simple open-addressing hash-table of moves


#define MAX_NODES_BITS 26
#define MAX_NODES (1 << MAX_NODES_BITS) 

struct Node nodes[MAX_NODES];


#define HASH_LOOKUP(hash) ((hash) & (MAX_NODES-1))
#define HASH_CHECK(hash) (hash)

static uint64_t nodetype_hash[16];
static uint64_t target_hash[POSCOUNT];
static uint64_t move_hash[POSCOUNT];

int hash_node_count;


void hash_init()
{
	int n;
	unsigned p;

	memset(nodes, 0, sizeof(nodes));
	hash_node_count = 0;

    // setup random hash values
	for(n=0; n < 16; n++) 
		nodetype_hash[n] = rnd64();

	for(p=0; p < POSCOUNT; p++) 
	{
		target_hash[p] = rnd64();
		move_hash[p] = rnd64();
	}

}

// generates a hash from a move/target/pos combination
uint64_t hash_get(int move, int target, uint64_t goban_hash, int nodeType) 
{
	uint64_t hash = nodetype_hash[nodeType];

	if (nodeType & NODETYPE_MOVE)
		hash ^= move_hash[move];

	if (nodeType & NODETYPE_TARGET)
		hash ^= target_hash[target];

	if (nodeType & NODETYPE_POS)
		hash ^= goban_hash;

	return hash;
}


void hash_set_node(uint64_t hash, int win)
{
	struct Node *curNode = nodes + HASH_LOOKUP(hash);
	while  (curNode->hash != HASH_CHECK(hash) 
			 && curNode->visits)
	{
		curNode++;
		if (curNode >= nodes + MAX_NODES)
			curNode = nodes;
	}

	if (!curNode->visits)
	{
		curNode->hash = HASH_CHECK(hash);
		hash_node_count++;
	}
	curNode->visits++;
	curNode->wins += win;
}

void hash_set_node_special(uint64_t hash, double winrate)
{
	struct Node *curNode = nodes + HASH_LOOKUP(hash);
	while  (curNode->hash != HASH_CHECK(hash) 
			 && curNode->visits)
	{
		curNode++;
		if (curNode >= nodes + MAX_NODES)
			curNode = nodes;
	}

	if (!curNode->visits)
	{
		curNode->hash = HASH_CHECK(hash);
		hash_node_count++;
	}
	curNode->visits = 100000;
	curNode->wins = winrate * curNode->visits;
}

struct Node * hash_get_node(uint64_t hash)
{
	struct Node *curNode = nodes + HASH_LOOKUP(hash);
	while  (curNode->hash != HASH_CHECK(hash) 
			 && curNode->visits)
	{
		curNode++;
		if (curNode >= nodes + MAX_NODES)
			curNode = nodes;
	}

//	else if (create)
//		out(fileGTP, "Existing hash at %p/%p/%d (%x) wtih %x %d\n"
//				, pos, target, nodeType
//				, hash, curNode, curNode->visits);
	return curNode;
}

