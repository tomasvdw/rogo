



struct Node
{
	uint32_t wins;
	uint32_t visits;
	uint64_t hash;
};


#define NODETYPE_TARGET  0x1
#define NODETYPE_POS     0x2
#define NODETYPE_MOVE    0x4
#define NODETYPE_SPECIAL 0x8


void 			hash_init();

uint64_t 		hash_get(int move, int target, uint64_t goban_hash, int nodeType);
struct Node * 	hash_get_node(uint64_t hash);
void 			hash_set_node(uint64_t hash, int win);
void 			hash_set_node_special(uint64_t hash, double winrate);

extern int 		hash_node_count;

  
