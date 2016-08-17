#ifndef _KENDIAN_H_
#define _KENDIAN_H_

#include <stdint.h>

#ifndef K_BIG_ENDIAN
    #ifdef _BIG_ENDIAN_
        #if _BIG_ENDIAN_
            #define K_BIG_ENDIAN 1
        #endif
    #endif
    #ifndef K_BIG_ENDIAN
        #if defined(__hppa__) || \
            defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
            (defined(__MIPS__) && defined(__MISPEB__)) || \
            defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
            defined(__sparc__) || defined(__powerpc__) || \
            defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
            #define K_BIG_ENDIAN 1
        #endif
    #endif
    #ifndef K_BIG_ENDIAN
        #define K_BIG_ENDIAN  0
    #endif
#endif


template<typename T> T kEndianScalar(T t) {
  #if K_BIG_ENDIAN
    #if defined(_MSC_VER)
      #pragma push_macro("__builtin_bswap16")
      #pragma push_macro("__builtin_bswap32")
      #pragma push_macro("__builtin_bswap64")
      #define __builtin_bswap16 _byteswap_ushort
      #define __builtin_bswap32 _byteswap_ulong
      #define __builtin_bswap64 _byteswap_uint64
    #endif
    // If you're on the few remaining big endian platforms, we make the bold
    // assumption you're also on gcc/clang, and thus have bswap intrinsics:
    if (sizeof(T) == 1) {   // Compile-time if-then's.
      return t;
    } else if (sizeof(T) == 2) {
      uint16_t r = __builtin_bswap16(*reinterpret_cast<uint16_t *>(&t));
      return *reinterpret_cast<T *>(&r);
    } else if (sizeof(T) == 4) {
      uint32_t r = __builtin_bswap32(*reinterpret_cast<uint32_t *>(&t));
      return *reinterpret_cast<T *>(&r);
    } else if (sizeof(T) == 8) {
      uint64_t r = __builtin_bswap64(*reinterpret_cast<uint64_t *>(&t));
      return *reinterpret_cast<T *>(&r);
    } else {
      assert(0);
    }
    #if defined(_MSC_VER)
      #pragma pop_macro("__builtin_bswap16")
      #pragma pop_macro("__builtin_bswap32")
      #pragma pop_macro("__builtin_bswap64")
    #endif
  #else
	return t;
  #endif
}

template<typename T> T kReadScalar(const void *p) {
  return kEndianScalar(*reinterpret_cast<const T *>(p));
}

template<typename T> void kWriteScalar(void *p, T t) {
  *reinterpret_cast<T *>(p) = kEndianScalar(t);
}

#endif
