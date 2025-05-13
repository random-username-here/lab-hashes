#include "common.h"
#include <immintrin.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdalign.h>
#include <x86intrin.h>

unsigned int crc32(__m256i word);

bool str_eq_mm256(__m256i a, __m256i b)
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

struct Htab_Node {
	__m256i key;
	int count;
	struct Htab_Node *next;
};

struct Htab {
	struct Htab_Node **buckets;
	int num_buckets, num_vals;
};


static struct Htab htab;

static inline size_t htab_bucket_for(__m256i key)
{
	return crc32(key) % htab.num_buckets;
}

void htab_add_node(struct Htab_Node *node)
{
	ASSERT(node);

	struct Htab_Node **place = &htab.buckets[htab_bucket_for(node->key)];
	while (*place != NULL)
		place = &((*place)->next);
	*place = node;
	node->next = NULL;
	++htab.num_vals;
}

struct Htab_Node *htab_get(__m256i word)
{
	for (
			struct Htab_Node *node = htab.buckets[htab_bucket_for(word)];
			node != NULL;
			node = node->next
	) {
		// movemask returns all bits set if all are equal, 
		if (str_eq_mm256(node->key, word))
			return node;
	}
	return NULL;
}

void htab_init(size_t num_buckets)
{
	htab.num_buckets = num_buckets;
	htab.num_vals = 0;
	htab.buckets = calloc(num_buckets, sizeof(*htab.buckets));
	ASSERT(htab.buckets);
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
	node->key = word;
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
	for (int i = 0; i < htab.num_buckets; ++i) {
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
	printf("Number of buckets: %d\n", htab.num_buckets);
	printf("Uniqie words: %d\n", htab.num_vals);
	printf("Load factor %f\n", htab.num_vals * 1.0 / htab.num_buckets);
}

void htab_dump(void)
{
	htab_print_info();
	for (int i = 0; i < htab.num_buckets; ++i) {
		printf("Bucket %d:\n", i);
		for (
				struct Htab_Node *node = htab.buckets[i];
				node != NULL; node = node->next
		) {
			printf("\t`%s` x %d (crc %x)\n",
				(const char*) &node->key, node->count, crc32(node->key));
		}
	}
}
