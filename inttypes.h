
#define __WIN32__
#ifdef _WIN32

typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef int int32_t;
typedef unsigned int uint32_t;

typedef short int16_t;
typedef unsigned short uint16_t;

typedef char int8_t;
typedef unsigned char uint8_t;

#define UINT32_MAX 0xFFFFFFFF

#else
#include <inttypes.h>
#endif
