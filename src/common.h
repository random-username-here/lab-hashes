#ifndef I_COMMON
#define I_COMMON

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#define ASSERT(x)\
	do {\
		if (!(x)) {\
			fprintf(stderr, "%s:%d: assertion `%s` failed\n", __FILE__, __LINE__, #x);\
			abort();\
		}\
	} while (0)

// Hash table stuff
// implemented separately for each one

void htab_init(void);
void htab_deinit(void);
void htab_add(const char *word);
void htab_print_info(void);
void htab_dump(void);
int htab_count(const char *word);

#endif
