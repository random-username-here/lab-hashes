#include "common.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#define NUM_QUERIES 10000000
#define MAX_WORD_LEN 32

bool is_word(char c)
{
	return isalnum(c);
}

char normalize(char c)
{
	return tolower(c);
}

volatile int counts;

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Expected file name to read words from as second argument, and nothing more\n");
		return 1;
	}

	// Read the file into buffer

	printf("Reading file...\n");

	FILE *input = fopen(argv[1], "r");
	if (!input) {
		printf("Failed to open `%s`: %s", argv[1], strerror(errno));
		return 1;
	}

	fseek(input, 0, SEEK_END);
	int file_size = ftell(input);
	fseek(input, 0, SEEK_SET);

	char *source = calloc(file_size+1, sizeof(*source));
	ASSERT(source != NULL);

	fread(source, file_size, 1, input);
	source[file_size] = 0;
	fclose(input);

	// Find word starting locations, put \0 on word ends in the buffer

	printf("Splitting into words...\n");

	int word_pos_avail = 16, word_pos_len = 0;
	int *word_pos = calloc(word_pos_avail, sizeof(*word_pos));
	int word_begin = 0;
	ASSERT(word_pos);

	for (int i = 0; i <= file_size; ++i) {
		if ((i == 0 || !is_word(source[i-1])) && is_word(source[i])) {
			if (word_pos_avail == word_pos_len) {
				// this realloc is not safe from memory leak on fail,
				// but we will crash afterwards, so we don't care
				word_pos = realloc(word_pos, sizeof(*word_pos) * word_pos_avail * 2);
				ASSERT(word_pos);
				word_pos_avail *= 2;
			}
			word_begin = i;
			word_pos[word_pos_len++] = i;
		} else if (i != 0 && is_word(source[i-1]) && !is_word(source[i])) {
			if(i - word_begin > MAX_WORD_LEN) {
				printf("Note: too long word `%s` ignored\n",
						source + word_pos[word_pos_len-1]);
				word_pos_len--;
			}
			source[i] = '\0';
		}

		if (is_word(source[i]))
			source[i] = normalize(source[i]);
	}

	// Load hash table

	printf("Loading into hash table\n");

	htab_init();

	for (int i = 0; i < word_pos_len; ++i) {
		htab_add(source + word_pos[i]);
	}

	// when some implementation misbehaves
	//htab_dump();
	//exit(0);

	htab_print_info();

	// Searching
	
	srand(42);

	printf("Searching words\n");
	printf("Doing %d random queries to word counts\n", NUM_QUERIES);

	clock_t begin = clock();

	for (int i = 0; i < NUM_QUERIES; ++i) {
		const char *word = source + word_pos[rand() % word_pos_len];
		counts = htab_count(word);
		//printf("Word `%s` occured %d times\n", word, counts);
	} 

	clock_t end = clock();

	printf("It took %f ms\n", (end - begin) * 1000.0 / CLOCKS_PER_SEC);

	// Free afterwards

	htab_deinit();
	free(source);
	free(word_pos);

	return 0;
}
