#ifndef FT_STRACE_H
# define FT_STRACE_H
#include <stdbool.h>

struct user_regs_struct_64
{
  unsigned long long int r15;
  unsigned long long int r14;
  unsigned long long int r13;
  unsigned long long int r12;
  unsigned long long int rbp;
  unsigned long long int rbx;
  unsigned long long int r11;
  unsigned long long int r10;
  unsigned long long int r9;
  unsigned long long int r8;
  unsigned long long int rax;
  unsigned long long int rcx;
  unsigned long long int rdx;
  unsigned long long int rsi;
  unsigned long long int rdi;
  unsigned long long int orig_rax;
  unsigned long long int rip;
  unsigned long long int cs;
  unsigned long long int eflags;
  unsigned long long int rsp;
  unsigned long long int ss;
  unsigned long long int fs_base;
  unsigned long long int gs_base;
  unsigned long long int ds;
  unsigned long long int es;
  unsigned long long int fs;
  unsigned long long int gs;
};

struct user_regs_struct_32
{
  int ebx;
  int ecx;
  int edx;
  int esi;
  int edi;
  int ebp;
  int eax;
  int xds;
  int xes;
  int xfs;
  int xgs;
  int orig_eax;
  int eip;
  int xcs;
  int eflags;
  int esp;
  int xss;
};

extern const t_syscall syscalls_64[];
extern const t_syscall syscalls_32[];

typedef struct s_state {
	bool		in_syscall;
	bool		started;
} t_state;

#endif
