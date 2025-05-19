#ifndef FT_STRACE_H
# define FT_STRACE_H

#include <sys/syscall.h>

#define INT 0
#define UINT 1
#define LONG 2
#define ULONG 3
#define PTR 4
#define STRUCT 5
#define STR 6

typedef struct s_syscall
{
	const char	*name;
	int			num_args;
	int			arg_types[6];
}	t_syscall;

extern const t_syscall syscalls[];

#endif
