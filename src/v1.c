#include "common.h"
#include <string.h>

#define MAX_LOAD_FACTOR 20
#define INITIAL_BITS 4

struct Htab_Node {
	char *word;
	int count;
	struct Htab_Node *next;
};

struct Htab {
	struct Htab_Node **buckets;
	int num_bits, num_vals;
};

static const unsigned int crc32_table[] =
{
  0x00000000, 0x1edc6f41, 0x3db8de82, 0x2364b1c3,
  0x7b71bd04, 0x65add245, 0x46c96386, 0x58150cc7,
  0xf6e37a08, 0xe83f1549, 0xcb5ba48a, 0xd587cbcb,
  0x8d92c70c, 0x934ea84d, 0xb02a198e, 0xaef676cf,
  0xf31a9b51, 0xedc6f410, 0xcea245d3, 0xd07e2a92,
  0x886b2655, 0x96b74914, 0xb5d3f8d7, 0xab0f9796,
  0x05f9e159, 0x1b258e18, 0x38413fdb, 0x269d509a,
  0x7e885c5d, 0x6054331c, 0x433082df, 0x5deced9e,
  0xf8e959e3, 0xe63536a2, 0xc5518761, 0xdb8de820,
  0x8398e4e7, 0x9d448ba6, 0xbe203a65, 0xa0fc5524,
  0x0e0a23eb, 0x10d64caa, 0x33b2fd69, 0x2d6e9228,
  0x757b9eef, 0x6ba7f1ae, 0x48c3406d, 0x561f2f2c,
  0x0bf3c2b2, 0x152fadf3, 0x364b1c30, 0x28977371,
  0x70827fb6, 0x6e5e10f7, 0x4d3aa134, 0x53e6ce75,
  0xfd10b8ba, 0xe3ccd7fb, 0xc0a86638, 0xde740979,
  0x866105be, 0x98bd6aff, 0xbbd9db3c, 0xa505b47d,
  0xef0edc87, 0xf1d2b3c6, 0xd2b60205, 0xcc6a6d44,
  0x947f6183, 0x8aa30ec2, 0xa9c7bf01, 0xb71bd040,
  0x19eda68f, 0x0731c9ce, 0x2455780d, 0x3a89174c,
  0x629c1b8b, 0x7c4074ca, 0x5f24c509, 0x41f8aa48,
  0x1c1447d6, 0x02c82897, 0x21ac9954, 0x3f70f615,
  0x6765fad2, 0x79b99593, 0x5add2450, 0x44014b11,
  0xeaf73dde, 0xf42b529f, 0xd74fe35c, 0xc9938c1d,
  0x918680da, 0x8f5aef9b, 0xac3e5e58, 0xb2e23119,
  0x17e78564, 0x093bea25, 0x2a5f5be6, 0x348334a7,
  0x6c963860, 0x724a5721, 0x512ee6e2, 0x4ff289a3,
  0xe104ff6c, 0xffd8902d, 0xdcbc21ee, 0xc2604eaf,
  0x9a754268, 0x84a92d29, 0xa7cd9cea, 0xb911f3ab,
  0xe4fd1e35, 0xfa217174, 0xd945c0b7, 0xc799aff6,
  0x9f8ca331, 0x8150cc70, 0xa2347db3, 0xbce812f2,
  0x121e643d, 0x0cc20b7c, 0x2fa6babf, 0x317ad5fe,
  0x696fd939, 0x77b3b678, 0x54d707bb, 0x4a0b68fa,
  0xc0c1d64f, 0xde1db90e, 0xfd7908cd, 0xe3a5678c,
  0xbbb06b4b, 0xa56c040a, 0x8608b5c9, 0x98d4da88,
  0x3622ac47, 0x28fec306, 0x0b9a72c5, 0x15461d84,
  0x4d531143, 0x538f7e02, 0x70ebcfc1, 0x6e37a080,
  0x33db4d1e, 0x2d07225f, 0x0e63939c, 0x10bffcdd,
  0x48aaf01a, 0x56769f5b, 0x75122e98, 0x6bce41d9,
  0xc5383716, 0xdbe45857, 0xf880e994, 0xe65c86d5,
  0xbe498a12, 0xa095e553, 0x83f15490, 0x9d2d3bd1,
  0x38288fac, 0x26f4e0ed, 0x0590512e, 0x1b4c3e6f,
  0x435932a8, 0x5d855de9, 0x7ee1ec2a, 0x603d836b,
  0xcecbf5a4, 0xd0179ae5, 0xf3732b26, 0xedaf4467,
  0xb5ba48a0, 0xab6627e1, 0x88029622, 0x96def963,
  0xcb3214fd, 0xd5ee7bbc, 0xf68aca7f, 0xe856a53e,
  0xb043a9f9, 0xae9fc6b8, 0x8dfb777b, 0x9327183a,
  0x3dd16ef5, 0x230d01b4, 0x0069b077, 0x1eb5df36,
  0x46a0d3f1, 0x587cbcb0, 0x7b180d73, 0x65c46232,
  0x2fcf0ac8, 0x31136589, 0x1277d44a, 0x0cabbb0b,
  0x54beb7cc, 0x4a62d88d, 0x6906694e, 0x77da060f,
  0xd92c70c0, 0xc7f01f81, 0xe494ae42, 0xfa48c103,
  0xa25dcdc4, 0xbc81a285, 0x9fe51346, 0x81397c07,
  0xdcd59199, 0xc209fed8, 0xe16d4f1b, 0xffb1205a,
  0xa7a42c9d, 0xb97843dc, 0x9a1cf21f, 0x84c09d5e,
  0x2a36eb91, 0x34ea84d0, 0x178e3513, 0x09525a52,
  0x51475695, 0x4f9b39d4, 0x6cff8817, 0x7223e756,
  0xd726532b, 0xc9fa3c6a, 0xea9e8da9, 0xf442e2e8,
  0xac57ee2f, 0xb28b816e, 0x91ef30ad, 0x8f335fec,
  0x21c52923, 0x3f194662, 0x1c7df7a1, 0x02a198e0,
  0x5ab49427, 0x4468fb66, 0x670c4aa5, 0x79d025e4,
  0x243cc87a, 0x3ae0a73b, 0x198416f8, 0x075879b9,
  0x5f4d757e, 0x41911a3f, 0x62f5abfc, 0x7c29c4bd,
  0xd2dfb272, 0xcc03dd33, 0xef676cf0, 0xf1bb03b1,
  0xa9ae0f76, 0xb7726037, 0x9416d1f4, 0x8acabeb5
};

unsigned int crc32(const unsigned char *buf)
{
	unsigned int crc = 0xffffffff;
	while (*buf) {
		crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf) & 255];
		buf++;
	}
	return crc;
}

static struct Htab htab;

#define lower_n_bits(x, n) ((x) & ((1 << (n)) - 1))
#define htab_num_buckets() (1 << (htab.num_bits))
#define htab_bucket_for(x) lower_n_bits(crc32((const unsigned char *) (x)), htab.num_bits)

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


struct Htab_Node *htab_get(const char *word)
{
	for (
			struct Htab_Node *node = htab.buckets[htab_bucket_for(word)];
			node != NULL;
			node = node->next
	) {
		if (strcmp(node->word, word) == 0)
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
	node->word = strdup(word);
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
	for (int i = 0; i < htab_num_buckets(); ++i) {
		for (
				struct Htab_Node *node = htab.buckets[i];
				node != NULL;
		) {
			struct Htab_Node *next = node->next;
			free(node->word);
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
			printf("\t`%s` x %d (crc %x)\n", node->word, node->count, crc32(node->word));
		}
	}
}
