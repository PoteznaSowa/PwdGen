# Password generator
This tool generates a random password.
Unlike naïve implementations which use a low-quality PRNG like rand(), this program uses cryptographically secure random numbers, read from the CSPRNG in the operating system.

The program reads 128 random bits to make a password with a length of 15 characters. It also generates a 6-digit PIN.