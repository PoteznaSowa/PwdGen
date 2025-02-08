#ifdef _WIN32
#include <Windows.h>
#include <winternl.h>
#include <winioctl.h>

#pragma comment(lib, "NTDLL.Lib")

#define RTL_CONSTANT_STRING(s) { sizeof(s) - sizeof((s)[0]), sizeof(s), s }

#ifndef IOCTL_KSEC_RNG		 // ntddksec.h, 0x390004
#define IOCTL_KSEC_RNG		 CTL_CODE(FILE_DEVICE_KSEC, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#ifndef IOCTL_KSEC_RNG_REKEY // ntddksec.h, 0x390008
#define IOCTL_KSEC_RNG_REKEY CTL_CODE(FILE_DEVICE_KSEC, 2, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

#else // _WIN32
#include <sys/random.h>
#endif

#include <stdio.h>

/*
* Fill a buffer with random bytes from the operating system.
* @param	buffer	A pointer to the buffer
* @param	length	The length of the buffer
*/
static void GetRandom(void* buffer, int length) {
#ifdef _WIN32
	/*
	 * Get randomness from the kernel-mode CNG driver.
	 * Yes, we could use documented functions below:
	 * - CryptGenRandom;
	 * - SystemFunction036 a.k.a. RtlGenRandom;
	 * - rand_s;
	 * - BCryptGenRandom;
	 * - ProcessPrng,
	 * but all these functions in turn produce random numbers derived from
	 * the output of SystemPrng in the CNG driver.
	 * So we access the driver directly.
	 * https://github.com/gtworek/PSBits/blob/master/Misc/IOCTL_KSEC_RNG.c
	 */

	HANDLE dev;
	IO_STATUS_BLOCK iosb;
	UNICODE_STRING path = RTL_CONSTANT_STRING(L"\\Device\\CNG");
	OBJECT_ATTRIBUTES oa;

	InitializeObjectAttributes(&oa, &path, 0, NULL, NULL);
	NtOpenFile(&dev, FILE_READ_DATA, &oa, &iosb, FILE_SHARE_READ, 0);
	NtDeviceIoControlFile(dev, NULL, NULL, NULL, &iosb, IOCTL_KSEC_RNG_REKEY, NULL, 0, buffer, length);
	NtClose(dev);
#else	// Linux
	// Use a device file.
	//FILE* f = fopen("/dev/random", "rb");
	//fread(buffer, 1, length, f);
	//fclose(f);

	// Use a system call.
	getrandom(buffer, length, GRND_RANDOM);

	// Use an RDRAND instruction.
	//unsigned long long* p = buffer;
	//__builtin_ia32_rdrand64_step(p);
	//__builtin_ia32_rdrand64_step(p + 1);
#endif
}

/*
* Divides a 64-bit integer and returns a remainder plus some offset.
* @param	n	Dividend
* @param	m	Divisor
* @param	o	Modulo offset
*/
static int PullModulo(unsigned long long* n, int m, int o) {
	int r = *n % m + o;
	*n /= m;
	return r;
}

/*
* Swaps two array elements.
* @param	s	Array
* @param	i1	First element index
* @param	i2	Second element index
*/
static void Swap(char s[], int i1, int i2) {
	char c = s[i1];
	s[i1] = s[i2];
	s[i2] = c;
}

int
#ifdef _WIN32
__cdecl
#endif
main() {
	/*
	* Generate and show random text which consists of:
	* - 16 ASCII characters;
	* - 6 decimal digits;
	* - 32 hexadecimal digits.
	*/

	unsigned long long rnum;
	unsigned long long rnum2[2];
	char pwd[16];

	GetRandom(rnum2, sizeof(rnum2));

	rnum = rnum2[0];

	/*
	* It is guaranteed that the password contains at least one of these:
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
	Swap(pwd, 1, PullModulo(&rnum, 15, 0));
	Swap(pwd, 2, PullModulo(&rnum, 14, 0));
	Swap(pwd, 3, PullModulo(&rnum, 13, 0));

	printf("%.16s %.6d %016llx%016llx\n", pwd, (int)(rnum2[0] % 1000000), rnum2[0], rnum2[1]);
	return 0;
}
