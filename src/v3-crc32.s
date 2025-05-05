.text
.globl crc32

///
/// unsigned crc32(__m256i string)
///
/// Compute CRC32 of string stored in YMM0,
/// returning result in RAX.
///

crc32:	// The idea is as follows
	//
	// - String is in ymm0
	// - Compute string length
	// - Use a duff device-like jump table to perform crc32-ing
	//

	push %rbp
	mov %rsp, %rbp

	//==== Part 0: store the string on stack

	// Align the stack to 32 bytes
	and $0xFFFFFFFFFFFFFF80, %rsp

	// Get space for string
	sub $32, %rsp

	// Write the string
	vmovdqa %ymm0, (%rsp)

	//==== Part 1: compute string length

	// Zero ymm7
	vpxor %ymm7, %ymm7, %ymm7
	
	// Compare bytes in string to zeros
	vpcmpeqb %ymm0, %ymm7, %ymm1

	// Move result into integer mask
	vpmovmskb %ymm1, %ecx

	// Count number of first nonzero bytes in the string
	// Now %eax is strlen
	tzcnt %ecx, %ecx

	//==== Part 2: the crc32
	// String can be up to 32 bytes in length
	
	// RSI will be pointer to current position in string
	mov %rsp, %rsi

	// RDI will be the resulting hash
	mov $0xffffffff, %rdi

	// Most of the strings are small, so here is a shortcut
	cmp $8, %ecx
	jb lt8

	cmp $32, %ecx
	jb lt32
	crc32q (%rsi), %rdi
	sub $8, %ecx
	add $8, %rsi

lt32:	cmp $24, %ecx
	jb lt24
	crc32q (%rsi), %rdi
	sub $8, %ecx
	add $8, %rsi

lt24:	cmp $16, %ecx
	jb lt16
	crc32q (%rsi), %rdi
	sub $8, %ecx
	add $8, %rsi

lt16:	cmp $8, %ecx
	jb lt8
	crc32q (%rsi), %rdi
	sub $8, %ecx
	add $8, %rsi

lt8:	cmp $4, %ecx
	jb lt4
	crc32l (%rsi), %edi
	sub $4, %ecx
	add $4, %rsi

lt4:	cmp $2, %ecx
	jb lt2
	crc32w (%rsi), %edi
	sub $2, %ecx
	add $2, %rsi

lt2:	test %ecx, %ecx
	jz lt1
	crc32b (%rsi), %edi

lt1:	mov %rdi, %rax
	leave
	ret
