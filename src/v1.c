#include "common.h"
#include <string.h>

// CRC32 hash of given string
 __attribute__ ((noinline))
unsigned int crc32(const char *key)
{
	const unsigned char *buf = (const unsigned char *) key;

	unsigned int crc = 0xffffffff;
	while (*buf) {
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
		buf++;
	}
	return crc;
}

//
// The hash table
//

struct Htab_Node {
	char *key;
	int count;
	struct Htab_Node *next;
};

struct Htab {
	struct Htab_Node **buckets;
	int num_buckets, num_vals;
};

static struct Htab htab;

static inline size_t htab_bucket_for(const char *key)
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

 __attribute__ ((noinline))
struct Htab_Node *htab_get(const char *key)
{
	for (
			struct Htab_Node *node = htab.buckets[htab_bucket_for(key)];
			node != NULL;
			node = node->next
	) {
		if (strcmp(node->key, key) == 0)
			return node;
	}
	return NULL;
}


void htab_init(size_t num_buckets)
{
	htab.num_buckets = num_buckets;
	htab.buckets = calloc(htab.num_buckets, sizeof(*htab.buckets));
	ASSERT(htab.buckets);
	htab.num_vals = 0;
}

void htab_add(const char *word)
{
	struct Htab_Node *existing = htab_get(word);
	if (existing) {
		existing->count++;
		return;
	}
	
	struct Htab_Node *node = calloc(1, sizeof(*node));
	ASSERT(node);
	node->count = 1;
	node->key = strdup(word);
	node->next = NULL;
	htab_add_node(node);
}

int htab_count(const char *word)
{
	struct Htab_Node *node = htab_get(word);
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
			free(node->key);
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
			printf("\t`%s` x %d (crc %x)\n", node->key, node->count, crc32(node->key));
		}
	}
}
