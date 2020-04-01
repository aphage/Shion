#include <stddef.h>

void* aqua_memset(void* dest, int val, size_t len) {
    unsigned char* ptr = dest;
    while (len-- > 0)
        *ptr++ = (unsigned char)val;
    return dest;
}
void*
aqua_memmove(void* dest, const void* src, size_t len) {
	char* d = dest;
	const char* s = src;
	if (d < s)
		while (len--)
			*d++ = *s++;
	else
	{
		char* lasts = (char*)s + (len - 1);
		char* lastd = d + (len - 1);
		while (len--)
			*lastd-- = *lasts--;
	}
	return dest;
}

__int64
aqua_abs64(__int64 i) {
	return i < 0 ? -i : i;
}