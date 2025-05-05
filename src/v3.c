#include "common.h"
#include <immintrin.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <x86intrin.h>

#define MAX_LOAD_FACTOR 20
#define INITIAL_BITS 4

struct Htab_Node {
	__m256i word;
	int count;
	struct Htab_Node *next;
};

struct Htab {
	struct Htab_Node **buckets;
	int num_bits, num_vals;
};

// see v3-crc32.s
unsigned int crc32(__m256i word);

bool mm256_str_eq(__m256i a, __m256i b)
{
	// https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/sysdeps/x86_64/multiarch/strcmp-avx2.S#L101
	//
	
	__m256i diff = _mm256_cmpeq_epi8(a, b);
	// now we have 255 where they are equal, 0 where not
	
	__m256i min = _mm256_min_epu8(diff, a);
	// 0 if chars are not equal or one has `\0` there
	
	__m256i not_eq_or_null = _mm256_cmpeq_epi8(min, _mm256_set1_epi8(0));
	// Has 255-s where not equal or \0

	int mask = _mm256_movemask_epi8(not_eq_or_null);
	int num_same = __builtin_ctz(mask);

	// all bytes up to num_same are equal
	// a[num_same] != b[num_same] or a[num_same] == b[num_same] == \0
	
	char a_byte = ((char*) &a)[num_same],
		 b_byte = ((char*) &b)[num_same];

	return a_byte == b_byte;
}

void print_mm256(const char* label, __m256i word)
{
	printf("%s: ", label);
	for (int i = 0; i < sizeof(word); ++i)
		printf("%02x", ((unsigned char*) &word)[i]);
	printf("\n");
}

static struct Htab htab;

#define lower_n_bits(x, n) ((x) & ((1 << (n)) - 1))
#define htab_num_buckets() (1 << (htab.num_bits))
#define htab_bucket_for(x) lower_n_bits(crc32((x)), htab.num_bits)

void htab_grow(void);

void htab_add_node(struct Htab_Node *node)
{
	ASSERT(node);

	if (htab.num_vals / htab_num_buckets() >= MAX_LOAD_FACTOR)
		htab_grow();

	struct Htab_Node **place = &htab.buckets[htab_bucket_for(node->word)];
	while (*place != NULL)
		place = &((*place)->next);
	*place = node;
	node->next = NULL;
	++htab.num_vals;
}

void htab_grow()
{
	int old_num_buckets = htab_num_buckets();
	struct Htab_Node **new_buckets = calloc(old_num_buckets * 2, sizeof(*new_buckets));
	struct Htab_Node **old_buckets = htab.buckets;
	htab.buckets = new_buckets;
	htab.num_bits++;
	htab.num_vals = 0;

	for (int i = 0; i < old_num_buckets; ++i) {
		for (struct Htab_Node *node = old_buckets[i]; node;) {
			struct Htab_Node *next = node->next;
			htab_add_node(node);
			node = next;
		}
	}
}


struct Htab_Node *htab_get(__m256i word)
{
	for (
			struct Htab_Node *node = htab.buckets[htab_bucket_for(word)];
			node != NULL;
			node = node->next
	) {
		if (mm256_str_eq(node->word, word))
			return node;
	}
	return NULL;
}

void htab_init(void)
{
	htab.num_bits = INITIAL_BITS;
	htab.buckets = calloc(htab_num_buckets(), sizeof(*htab.buckets));
	ASSERT(htab.buckets);
	htab.num_vals = 0;
}

void htab_add(const char *word_ptr)
{
	__m256i word = _mm256_loadu_si256((const __m256i*) word_ptr);
	struct Htab_Node *existing = htab_get(word);
	if (existing) {
		existing->count++;
		return;
	}

	struct Htab_Node *node = aligned_alloc(sizeof(__m256i), sizeof(*node));
	ASSERT(node);
	node->count = 1;
	node->word = word;
	node->next = NULL;
	htab_add_node(node);
}

int htab_count(const char *word)
{
	struct Htab_Node *node = htab_get(_mm256_loadu_si256((const __m256i*) word));
	return node ? node->count : 0;
}

void htab_deinit(void)
{
	for (int i = 0; i < htab_num_buckets(); ++i) {
		for (
				struct Htab_Node *node = htab.buckets[i];
				node != NULL;
		) {
			struct Htab_Node *next = node->next;
			free(node);
			node = next;
		}
	}
	free(htab.buckets);
}

void htab_print_info(void)
{
	printf("Number of buckets: %d (%d last bits used as a key)\n", htab_num_buckets(), htab.num_bits);
	printf("Uniqie words: %d\n", htab.num_vals);
	printf("Load factor %f\n", htab.num_vals * 1.0 / htab_num_buckets());
}

void htab_dump(void)
{
	htab_print_info();
	for (int i = 0; i < htab_num_buckets(); ++i) {
		printf("Bucket %d:\n", i);
		for (
				struct Htab_Node *node = htab.buckets[i];
				node != NULL; node = node->next
		) {
			printf("\t`%s` x %d (crc %x)\n", (const char*) &node->word, node->count, crc32(node->word));
		}
	}
}
