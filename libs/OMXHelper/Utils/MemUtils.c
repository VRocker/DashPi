#include "MemUtils.h"
#include <stdlib.h>

#define _ALIGN( value, alignment ) ((( value ) + ( alignment - 1 ) ) &~ ( alignment - 1 ) )

void* _aligned_malloc(size_t s, size_t alignTo)
{
	char* pFull = (char*)malloc(s + alignTo + sizeof(char*));
	char* pAligned = (char*)_ALIGN(((unsigned long)pFull + sizeof(char*)), alignTo);

	*(char **)(pAligned - sizeof(char*)) = pFull;

	return pAligned;
}

void _aligned_free(void* p)
{
	if (!p)
		return;

	char* pFull = *(char**)(((char*)p) - sizeof(char*));
	free(pFull);

	p = 0;
}