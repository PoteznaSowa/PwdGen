#pragma once

#ifdef __cplusplus
extern "C"
#endif
/*
 * Fill a buffer with random bytes from the operating system.
 * @param	buffer	A pointer to the buffer
 * @param	length	The length of the buffer
 */
void OsRng(void* buffer, unsigned length);
