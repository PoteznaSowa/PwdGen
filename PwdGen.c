#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__x86_64__)
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#include <immintrin.h>
#endif
#endif

#include <stdio.h>
#include "osrng.h"

/*
 * Divides a 64-bit integer dividend in place.
 * @param	n	Dividend
 * @param	m	Divisor
 * @param	o	Modulo offset
 * @return	Remainder plus offset
 */
static int PullModulo(unsigned long long* n, int m, int o) {
	int r = *n % m + o;
	*n /= m;
	return r;
}

/*
 * Swaps two array elements.
 * @param	s	Array to swap elements in
 * @param	i1	First element index
 * @param	i2	Second element index
 */
static void Swap(char s[], int i1, int i2) {
	char c = s[i1];
	s[i1] = s[i2];
	s[i2] = c;
}

typedef union {
	unsigned long long u64[2];
	unsigned u32[4];
} PwdData;

static void FillRand(PwdData* buffer) {
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__x86_64__)

	int max_cpuid;
	int rdrand_available = 0;
	int rdseed_available = 0;

#ifdef _MSC_VER
	int regs[4];
	__cpuid(regs, 0);
	max_cpuid = regs[0];

	if (max_cpuid >= 1) {
		__cpuid(regs, 1);
		rdrand_available = (regs[2] >> 30) & 1;
	}
	if (max_cpuid >= 7) {
		__cpuid(regs, 7);
		rdseed_available = (regs[1] >> 18) & 1;
	}
#else
	int eax, ebx, ecx, edx;
	__cpuid(0, eax, ebx, ecx, edx);
	max_cpuid = eax;

	if (max_cpuid >= 1) {
		__cpuid(1, eax, ebx, ecx, edx);
		rdrand_available = !!(ecx & bit_RDRND);
	}
	if (max_cpuid >= 7) {
		__cpuid(7, eax, ebx, ecx, edx);
		rdseed_available = !!(ebx & bit_RDSEED);
	}
#endif

	if (rdseed_available) {
#if defined(_M_AMD64) || defined(__x86_64__)
		_rdseed64_step(&buffer->u64[0]);
		_rdseed64_step(&buffer->u64[1]);
#else
		_rdseed32_step(&buffer->u32[0]);
		_rdseed32_step(&buffer->u32[1]);
		_rdseed32_step(&buffer->u32[2]);
		_rdseed32_step(&buffer->u32[3]);
#endif
		return;
	}
	if (rdrand_available) {
#if defined(_M_AMD64) || defined(__x86_64__)
		_rdrand64_step(&buffer->u64[0]);
		_rdrand64_step(&buffer->u64[1]);
#else
		_rdrand32_step(&buffer->u32[0]);
		_rdrand32_step(&buffer->u32[1]);
		_rdrand32_step(&buffer->u32[2]);
		_rdrand32_step(&buffer->u32[3]);
#endif
		return;
	}
#endif	// x86
	OsRng(buffer, sizeof(*buffer));
}

int main() {
	/*
	 * Generate and show random text which consists of:
	 * - 16 ASCII characters;
	 * - 6 decimal digits;
	 * - 32 hexadecimal digits.
	 */

	unsigned long long rnum;
	PwdData rnum2;
	char pwd[16];

	FillRand(&rnum2);

	rnum = rnum2.u64[0];

	/*
	 * It is guaranteed that the password contains at least one:
	 * - digit;
	 * - upper letter;
	 * - lower letter;
	 * - punctuation symbol.
	 */
	const char puncts[32] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
	pwd[0] = PullModulo(&rnum, 10, '0');
	pwd[1] = PullModulo(&rnum, 26, 'A');
	pwd[2] = PullModulo(&rnum, 26, 'a');
	pwd[3] = puncts[PullModulo(&rnum, 32, 0)];

	for (int i = 4; i < 10; i++) {
		pwd[i] = PullModulo(&rnum, 94, '!');
	}

	rnum = rnum2.u64[1];

	for (int i = 10; i < 16; i++) {
		pwd[i] = PullModulo(&rnum, 94, '!');
	}

	Swap(pwd, 0, PullModulo(&rnum, 15, 0));
	Swap(pwd, 1, PullModulo(&rnum, 14, 1));
	Swap(pwd, 2, PullModulo(&rnum, 13, 2));
	Swap(pwd, 3, PullModulo(&rnum, 12, 3));

	printf("%.16s %.6d %016llx%016llx\n", pwd, (int)(rnum2.u64[0] % 1000000), rnum2.u64[0], rnum2.u64[1]);
	return 0;
}
