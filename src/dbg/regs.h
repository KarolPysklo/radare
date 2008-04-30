#ifndef _INCLUDE_REGS_H_
#define _INCLUDE_REGS_H_
#include "../main.h"
#include "thread.h"
#include "wp.h"

#ifndef OFF_FMT
#define OFF_FMT "0x%08x"
#define OFF_FMTx "%x"
#define OFF_FMTd "%d"
#endif

#if __WINDOWS__
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4

#include <windows.h>
#include <winbase.h>

#define regs_t CONTEXT

#else

#include "signal.h"

#if __mips__ && __linux__
	#include <sys/ucontext.h>
	#include <sys/user.h>
	//typedef unsigned long mips64_regs_t [4096];
	//  #define regs_t mips64_regs_t
	#define regs_t mcontext_t
#endif

#if __x86_64__ && __linux__
	/* linux 64 bits */
	#include <sys/user.h>
	#define regs_t struct user_regs_struct
#endif

#if __i386__ && __linux__
	/* linux 32 bits */
	#include <sys/user.h>
	#define regs_t struct user_regs_struct
#endif
 
#if __APPLE__
	#include <sys/ucontext.h>
	#include <mach/i386/_structs.h>
 	#include <mach/mach_types.h>
	#define regs_t _STRUCT_X86_THREAD_STATE32
#endif

/* BSD */
#if __NetBSD__ || __FreeBSD__ || __OpenBSD__
	/* bsd 32 bits */
	#include <machine/reg.h>
	#define regs_t struct reg
#endif

#if __arm__ && __linux__
	#include <sys/ucontext.h>
	#define regs_t elf_gregset_t
#endif
#endif


  /* portable ptrace */
#if __linux__
#define PTRACE_PC 0
#else
#define PTRACE_PC arch_pc()
#endif

// BSD compatibility
#if __Solaris__
#define PTRACE_PEEKTEXT PTRACE_READTEXT
#define PTRACE_POKETEXT PTRACE_WRITETEXT
#if 0
        PTRACE_ATTACH                    /* 10, attach to an existing process */
        PTRACE_DETACH                    /* 11, detach from a process */
        PTRACE_GETREGS                   /* 12, get all registers */
        PTRACE_SETREGS                   /* 13, set all registers */
        PTRACE_GETFPREGS                 /* 14, get all floating point regs */
        PTRACE_SETFPREGS                 /* 15, set all floating point regs */
        PTRACE_READDATA                  /* 16, read data segment */
        PTRACE_WRITEDATA                 /* 17, write data segment */
        PTRACE_READTEXT                  /* 18, read text segment */
        PTRACE_WRITETEXT                 /* 19, write text segment */
        PTRACE_GETFPAREGS                /* 20, get all fpa regs */
        PTRACE_SETFPAREGS                /* 21, set all fpa regs */
        PTRACE_GETWINDOW                 /* 22, get register window n */
        PTRACE_SETWINDOW                 /* 23, set register window n */
        PTRACE_22                        /* 22, filler */
        PTRACE_23                        /* 23, filler */
        PTRACE_SYSCALL                   /* 24, trap next sys call */

        PTRACE_DUMPCORE                  /* 25, dump process core */
        PTRACE_SETWRBKPT                 /* 26, set write breakpoint */
        PTRACE_SETACBKPT                 /* 27, set access breakpoint */
        PTRACE_CLRDR7                    /* 28, clear debug register 7 */
        PTRACE_26                        /* 26, filler */
        PTRACE_27                        /* 27, filler */
        PTRACE_28                        /* 28, filler */
        PTRACE_GETUCODE                  /* 29, get u.u_code */ 
     // ^- this is the trap reason
#endif
#endif

#if __FreeBSD__ || __NetBSD__ || __OpenBSD__ || __APPLE__
#define PTRACE_KILL PT_KILL
#define PTRACE_DETACH PT_DETACH
#define PTRACE_TRACEME PT_TRACE_ME
#define PTRACE_ATTACH PT_ATTACH
#define PTRACE_SYSCALL PT_SYSCALL
#define PTRACE_CONT PT_CONTINUE
#define PTRACE_PEEKTEXT PT_READ_D
#define PTRACE_POKEDATA PT_WRITE_D
#define PTRACE_GETREGS PT_GETREGS
#define PTRACE_SETREGS PT_SETREGS
#define PTRACE_GETFPREGS PT_GETFPREGS
#define PTRACE_SETFPREGS PT_SETFPREGS
#define PTRACE_SINGLESTEP PT_STEP
#endif

#if __FreeBSD__ || __NetBSD__ || __OpenBSD__
  #include <machine/reg.h>
  #define regs_t struct reg
#endif


#if __x86_64__ && __linux__
	/* linux 64 bits */
	#include <sys/user.h>
	#define regs_t struct user_regs_struct
#endif

#define MAX_BPS	128

#define WS(w) (ps.ws.w)
#define WS_SI(f) (ps.ws.si.f)

#ifndef regs_t
#error No regs_t defined?!
#endif

//#ifndef regs_t
#if __arm__
  #define regs_t elf_gregset_t
//#else
//  #define regs_t struct user_regs_struct
#endif
//#endif
#if __WINDOWS__
#define siginfo_t int
#else
#include <signal.h>
#endif

struct wait_state {
	int event;
	struct bp_t *bp;
	regs_t regs;
	siginfo_t si;
};


#endif
