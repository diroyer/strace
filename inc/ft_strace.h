#ifndef FT_STRACE_H

# define FT_STRACE_H

# include <unistd.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <sys/ptrace.h>
# include <sys/types.h>
# include <sys/wait.h>


typedef int64_t Err_value;

# define _error(...) { \
	dprintf(STDERR_FILENO, __VA_ARGS__); \
}

# define _print(...) { \
	dprintf(STDOUT_FILENO, __VA_ARGS__); \
}

# ifndef DEBUG
#  define _debug(...) { \
		dprintf(STDERR_FILENO, __VA_ARGS__); \
	}
# else
#  define _debug(...) { }
# endif

#endif
