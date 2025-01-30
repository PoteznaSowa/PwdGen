# Password generator
This tool generates a random password.
Unlike naive implementations which use a low-quality PRNG like rand(), this program uses cryptographically secure random numbers, read from the CSPRNG in the operating system.