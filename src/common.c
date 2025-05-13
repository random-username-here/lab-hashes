#include "common.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <immintrin.h>
#include <math.h>

#define NUM_MEASUREMENTS 20
#define NUM_QUERIES 5000000
#define NUM_BUCKETS 503
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

	if (argc != 2 && argc != 3) {
		printf("Usage: %s [-p] TEXT_FILE\n", argv[0]);
		return 1;
	}

	bool profile_mode = false;
	int filename_pos = 1;
	if (strcmp(argv[1], "-p") == 0) {
		filename_pos++;
		profile_mode = true;
	}

	if (filename_pos >= argc) {
		printf("File name expected\n");
		return 1;
	}

	// Read the file into buffer

	printf("Reading file...\n");

	FILE *input = fopen(argv[filename_pos], "r");
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

	htab_init(NUM_BUCKETS);

	for (int i = 0; i < word_pos_len; ++i) {
		htab_add(source + word_pos[i]);
	}

	// when some implementation misbehaves
	//htab_dump();
	//exit(0);

	htab_print_info();

	// Searching
	
	srand(42);
	size_t *randoms = calloc(NUM_QUERIES, sizeof(*randoms));
	ASSERT(randoms);
	for (int i = 0; i < NUM_QUERIES; ++i)
		randoms[i] = rand() % word_pos_len;

	printf("Searching words\n");

	if (!profile_mode) {
		printf("Doing %d measurements of %d queries\n", NUM_MEASUREMENTS, NUM_QUERIES);

		float *times = calloc(NUM_MEASUREMENTS, sizeof(*times));
		ASSERT(times);

		for (int i = 0; i < NUM_MEASUREMENTS; ++i) {

			uint64_t begin = __rdtsc();

			for (int j = 0; j < NUM_QUERIES; ++j) {
				const char *word = source + word_pos[randoms[j]];
				counts = htab_count(word);
			}

			uint64_t end = __rdtsc();
			times[i] = (end - begin) * 1.0f / NUM_QUERIES;
			printf("time %f\n", times[i]);
		} 

		float avg_time = 0, std_dev = 0;

		for (int i = 0; i < NUM_MEASUREMENTS; ++i)
			avg_time += times[i];
		avg_time /= NUM_MEASUREMENTS;

		for (int i = 0; i < NUM_MEASUREMENTS; ++i) {
			float diff = (times[i] - avg_time);
			std_dev += diff * diff;
		}
		std_dev /= NUM_MEASUREMENTS;
		std_dev = sqrtf(std_dev);

		printf("It took %f +- %f cycles per search\n", avg_time, 3 * std_dev);

	} else {
		printf("Doing %d queries\n", NUM_QUERIES);

		for (int j = 0; j < NUM_QUERIES; ++j) {
			const char *word = source + word_pos[randoms[j]];
			counts = htab_count(word);
		}
	}
	// Free afterwards

	htab_deinit();
	free(source);
	free(word_pos);

	return 0;
}
