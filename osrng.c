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

#else
#ifdef __linux
#include <sys/random.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#endif

void OsRng(void* buffer, unsigned length) {
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
	 * the output of SystemPrng in the CNG driver. We access the driver
	 * directly.
	 * Moreover, the aforementioned documented functions are handled by a
	 * user-mode CSPRNG, which is relatively easier to compromise than a
	 * kernel-mode one.
	 * 
	 * References:
	 * https://aka.ms/win10rng
	 * https://github.com/gtworek/PSBits/blob/master/Misc/IOCTL_KSEC_RNG.c
	 * https://learn.microsoft.com/en-us/windows/security/security-foundations/certification/fips-140-validation
	 */

	static volatile HANDLE dev = NULL;
	static volatile LONG dev_flag = 0;

	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	if (dev == NULL) {
		if (!InterlockedExchange(&dev_flag, 1)) {
			UNICODE_STRING path = RTL_CONSTANT_STRING(L"\\Device\\CNG");
			OBJECT_ATTRIBUTES oa;
			InitializeObjectAttributes(&oa, &path, 0, NULL, NULL);
			status = NtOpenFile(&dev, FILE_READ_DATA, &oa, &iosb, FILE_SHARE_READ, 0);
			if (!NT_SUCCESS(status)) {
				dev = NULL;
			}
			InterlockedExchange(&dev_flag, 0);
		} else {
			do {
				SwitchToThread();
			} while (InterlockedCompareExchange(&dev_flag, 0, 0));
		}
	}

	ULONG ioctl = length < 16384 ? IOCTL_KSEC_RNG : IOCTL_KSEC_RNG_REKEY;

	status = NtDeviceIoControlFile(dev, NULL, NULL, NULL, &iosb, ioctl, NULL, length, buffer, length);

	//NtClose(dev);
	//dev = NULL;
#else
	unsigned char* _b = buffer;
#ifdef __linux
	// Use a system call.
	do {
		ssize_t r = getrandom(_b, length, 0);
		if (r == -1) {
			break;
		}
		_b += r;
		length -= r;
	} while (length);
	if (!length) {
		return;
	}
#endif
	// Use a device file.
	int f = open("/dev/urandom", O_RDONLY);
	do {
		ssize_t r = read(f, _b, length);
		if (r == -1) {
			break;
		}
		_b += r;
		length -= r;
	} while (length);
	close(f);
	//if (!length) {
	//	return;
	//}
#endif
}
