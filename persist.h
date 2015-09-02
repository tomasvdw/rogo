
// persistence currently unused 
void persist_create();
void persist_init();
void persist_save();
int persist_load();
void persist_backup();

struct FileHeader
{
	char ident[4];
	unsigned int rogovermajor;
	unsigned int rogoverminor;
	long long fights;
	long long seconds;
	long long improvements;
	long long bitmutations;
};

extern struct FileHeader fileHeader;

