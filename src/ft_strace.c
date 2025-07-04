/* Impl of ft_strace */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <signal.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdbool.h>

#include "syscalls.h"
#include "ft_strace.h"

#define NT_PRSTATUS 1

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

void	format_arg(int type, void *arg)
{
	switch (type) {
		case INT:
			fprintf(stderr, "%d", *(int *)arg);
			break;
		case UINT:
			fprintf(stderr, "%u", *(unsigned int *)arg);
			break;
		case LONG:
			fprintf(stderr, "%ld", *(long *)arg);
			break;
		case ULONG:
			fprintf(stderr, "%lu", *(unsigned long *)arg);
			break;
		case PTR:
			if (!*(void **)arg)
				fprintf(stderr, "NULL");
			else
				fprintf(stderr, "%p", *(void **)arg);
			break;
		case STR:
			if (!*(char **)arg)
				fprintf(stderr, "NULL");
			else {
				fprintf(stderr, "...");
			}
			break;
		case STRUCT:
			fprintf(stderr, "{...}");
			break;
		default:
			fprintf(stderr, "unknown");
			break;
	}
}

void format_arg_32(int type, void *arg)
{
    switch (type) {
        case INT:
            fprintf(stderr, "%d", *(int32_t *)arg);
            break;
        case UINT:
            fprintf(stderr, "%u", *(uint32_t *)arg);
            break;
        case LONG:
            fprintf(stderr, "%d", *(int32_t *)arg); // long = 32 bits sur 32 bits
            break;
        case ULONG:
            fprintf(stderr, "%u", *(uint32_t *)arg); // unsigned long = 32 bits
            break;
        case PTR:
            if (*(uint32_t *)arg == 0)
                fprintf(stderr, "NULL");
            else
                fprintf(stderr, "0x%x", *(uint32_t *)arg);
            break;
        case STR:
            if (*(uint32_t *)arg == 0)
                fprintf(stderr, "NULL");
            else
                fprintf(stderr, "...");
            break;
        case STRUCT:
            fprintf(stderr, "{...}");
            break;
        default:
            fprintf(stderr, "unknown");
            break;
    }
}

int handle_64(struct user_regs_struct_64 regs, t_state *state) {

	if (regs.orig_rax == __NR_execve) {
		if (!state->started) {
			state->started = true;
		}
	} else if (!state->started) {
		return 0;
	}

	if (!state->in_syscall) {

		unsigned long long *args[] = {
			&regs.rdi, &regs.rsi, &regs.rdx,
			&regs.r10, &regs.r8, &regs.r9
		};

		fprintf(stderr, "%s(", syscalls_64[regs.orig_rax].name);
		for (int i = 0; i < syscalls_64[regs.orig_rax].num_args; i++) {
			if (i > 0)
				fprintf(stderr, ", ");
			format_arg(syscalls_64[regs.orig_rax].arg_types[i], (void *)args[i]);
		}
		fprintf(stderr, ")");

	} else {
		fprintf(stderr, " = 0x%llx\n", regs.rax);
		fflush(stderr);
	}

	return 0;
}

int handle_32(struct user_regs_struct_32 regs, t_state *state) {

	if (!state->in_syscall) {

		int *args[] = {
			&regs.ebx, &regs.ecx, &regs.edx,
			&regs.esi, &regs.edi, &regs.ebp
		};

		fprintf(stderr, "%s(", syscalls_32[regs.orig_eax].name);
		for (int i = 0; i < syscalls_32[regs.orig_eax].num_args; i++) {
			if (i > 0)
				fprintf(stderr, ", ");
			format_arg_32(syscalls_32[regs.orig_eax].arg_types[i], (void *)args[i]);
		}
		fprintf(stderr, ")");

	} else {
		fprintf(stderr, " = 0x%x\n", regs.eax);
		fflush(stderr);
	}

	return 0;
}

void block_sig(pid_t pid)
{
	sigset_t set;
	int status;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
	waitpid(pid, &status, 0);
	sigaddset(&set, SIGHUP);
	//sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGPIPE);
	sigaddset(&set, SIGTERM);
	sigprocmask(SIG_BLOCK, &set, NULL);
}

int handle_syscall(pid_t tracee, t_state *state) {

	union {
		struct user_regs_struct_64 x86_64;
		struct user_regs_struct_32 x86_32;
	} regs;

	struct iovec iov;
	iov.iov_base = &regs;
	iov.iov_len = sizeof(regs);

	if (ptrace(PTRACE_GETREGSET, tracee, NT_PRSTATUS, &iov) == -1)
		handle_error("ptrace getregset");

	if (iov.iov_len == sizeof(struct user_regs_struct_32)) {
		handle_32(regs.x86_32, state);
	} else {
		handle_64(regs.x86_64, state);
	}

	return 0;
}

int trace_pid(pid_t tracee)
{
	int status;
	siginfo_t siginfo;
	t_state state = {0};


	if (ptrace(PTRACE_SEIZE, tracee, NULL, NULL) == -1)
		handle_error("ptrace seize");

	if (ptrace(PTRACE_INTERRUPT, tracee, NULL, NULL) == -1)
		handle_error("ptrace interrupt");

	block_sig(tracee);

	if (ptrace(PTRACE_SETOPTIONS, tracee, 0, PTRACE_O_TRACESYSGOOD) == -1)
		handle_error("ptrace setoptions");

	while (1) {
		if (ptrace(PTRACE_SYSCALL, tracee, NULL, NULL) == -1)
			handle_error("ptrace syscall");

		if (waitpid(tracee, &status, 0) == -1)
			handle_error("waitpid");

		if (WIFEXITED(status)) {
			if (state.in_syscall)
				fprintf(stderr, "\n");
			return (WEXITSTATUS(status));
		}

		if (ptrace(PTRACE_GETSIGINFO, tracee, NULL, &siginfo) == -1)
			handle_error("ptrace getsiginfo");

		if (siginfo.si_signo == SIGTRAP) {

			handle_syscall(tracee, &state);
			state.in_syscall = !state.in_syscall;
	
		} else {
			fprintf(stderr, "--- %s ---\n", strsignal(siginfo.si_signo));
			return (siginfo.si_signo);
		}
	}
}

char *find_executable(char *cmd)
{
	static char full_path[1024] = {0};
	char *path = getenv("PATH");
	if (!path) {
		fprintf(stderr, "PATH environment variable not set\n");
		return NULL;
	}

	char *token = strtok(path, ":");
	while (token) {
		//snprintf(full_path, sizeof(full_path), "%s/%s", token, cmd);

		strncpy(full_path, token, sizeof(full_path) - 1);
		strncat(full_path, "/", sizeof(full_path) - strlen(full_path) - 1);
		strncat(full_path, cmd, sizeof(full_path) - strlen(full_path) - 1);
		if (access(full_path, R_OK | X_OK) == 0) {
			return full_path;
		}
		token = strtok(NULL, ":");
	}
	return NULL;
}


int	main(int argc, char **argv, char **envp)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
		return (1);
	}

	if (argv[1][0] != '.' && argv[1][0] != '/') {
		char *executable = find_executable(argv[1]);
		if (!executable) {
			fprintf(stderr, "Executable '%s' not found in PATH\n", argv[1]);
			return (1);
		}
		argv[1] = executable;
	} 

	pid_t pid = fork();
	int status = 0;

	if (pid == -1)
		handle_error("fork");

	if (pid == 0) {
		execve(argv[1], argv + 1, envp);
		handle_error("execve");
	} else {
		status = trace_pid(pid);
		if (WIFSIGNALED(status)) {
			fprintf(stderr, "+++ killed by %s +++\n", strsignal(WTERMSIG(status)));
			kill(getpid(), WTERMSIG(status));
		} else
			fprintf(stderr, "+++ exited with %d +++\n", WEXITSTATUS(status));
	}
	return (WEXITSTATUS(status));
}
