#ifndef _INCLUDE_R_TYPES_H_
#define _INCLUDE_R_TYPES_H_

/* basic types */

#define u64 unsigned long long
#define u32 unsigned long
#define u16 unsigned short
#define u8  unsigned char

/* operating system */

#undef __BSD__
#undef __UNIX__
#undef __WINDOWS__

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
  #define __BSD__ 1
#endif

#if __WIN32__ || __CYGWIN__ || MINGW32
  #define __addr_t_defined
  #include <windows.h>
  #ifdef USE_SOCKETS
  #include <winsock.h>
  #undef USE_SOCKETS
#endif

  #define __WINDOWS__ 1
#else
  #define __UNIX__ 1
#endif

#endif