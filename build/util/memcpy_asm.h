#ifndef __CPU_ASM_H__
#define __CPU_ASM_H__

#include <cover_base.h>

EXTERN_SYMBOL DLL_SYMBOL void * CDECL_SYMBOL cover_fast_memcpy_3dn(void * to, const void * from, size_t len);
EXTERN_SYMBOL DLL_SYMBOL void * CDECL_SYMBOL cover_fast_memcpy_mmx(void * to, const void * from, size_t len);
EXTERN_SYMBOL DLL_SYMBOL void * CDECL_SYMBOL cover_fast_memcpy_mmxext(void * to, const void * from, size_t len);
//EXTERN_SYMBOL DLL_SYMBOL void * CDECL_SYMBOL cover_fast_memcpy(void * to, const void * from, size_t len);

#endif
