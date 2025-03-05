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

int main() {
	/*
	 * Generate and show random text which consists of:
	 * - 16 ASCII characters;
	 * - 6 decimal digits;
	 * - 32 hexadecimal digits.
	 */

	unsigned long long rnum;
	unsigned long long rnum2[2];
	char pwd[16];

	OsRng(rnum2, sizeof(rnum2));

	rnum = rnum2[0];

	/*
	 * It is guaranteed that the password contains at least one:
	 * - digit;
	 * - upper letter;
	 * - lower letter;
	 * - punctuation symbol.
	 */
	const char puncts[] = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
	pwd[0] = PullModulo(&rnum, 10, '0');
	pwd[1] = PullModulo(&rnum, 26, 'A');
	pwd[2] = PullModulo(&rnum, 26, 'a');
	pwd[3] = puncts[PullModulo(&rnum, 32, 0)];
	
	for (int i = 4; i < 10; i++) {
		pwd[i] = PullModulo(&rnum, 94, '!');
	}

	rnum = rnum2[1];

	for (int i = 10; i < 16; i++) {
		pwd[i] = PullModulo(&rnum, 94, '!');
	}

	Swap(pwd, 0, PullModulo(&rnum, 16, 0));
	Swap(pwd, 1, PullModulo(&rnum, 15, 1));
	Swap(pwd, 2, PullModulo(&rnum, 14, 2));
	Swap(pwd, 3, PullModulo(&rnum, 13, 3));

	printf("%.16s %.6d %016llx%016llx\n", pwd, (int)(rnum2[0] % 1000000), rnum2[0], rnum2[1]);
	return 0;
}
