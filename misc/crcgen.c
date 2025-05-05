// https://github.com/gcc-mirror/gcc/blob/master/libiberty/crc32.c
// With polynomial for Castagnoli CRC32

#include <stdio.h>

int main ()
{
	unsigned int i, j;
	unsigned int c;
	int table[256];

	for (i = 0; i < 256; i++)
	{
		for (c = i << 24, j = 8; j > 0; --j)
			c = c & 0x80000000 ? (c << 1) ^ 0x1EDC6F41 : (c << 1);
		table[i] = c;
	}

	printf ("static const unsigned int crc32_table[] =\n{\n");
	for (i = 0; i < 256; i += 4)
	{
		printf ("  0x%08x, 0x%08x, 0x%08x, 0x%08x",
				table[i + 0], table[i + 1], table[i + 2], table[i + 3]);
		if (i + 4 < 256)
			putchar (',');
		putchar ('\n');
	}
	printf ("};\n");
	return 0;
}
