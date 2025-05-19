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

#include "ft_strace.h"

#define NT_PRSTATUS 1

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

void get_regs(pid_t tracee, struct user_regs_struct *regs)
{
	struct iovec iov;

	iov.iov_base = regs;
	iov.iov_len = sizeof(*regs);

	if (ptrace(PTRACE_GETREGSET, tracee, NT_PRSTATUS, &iov) == -1)
		handle_error("ptrace getregset");
}

void escape_str(char *str)
{
	char *escaped_str = malloc(strlen(str) * 2 + 1);
	if (!escaped_str)
		handle_error("malloc");

	char *p = escaped_str;
	while (*str) {
		if (*str == '\\' || *str == '"')
			*p++ = '\\';
		*p++ = *str++;
	}
	*p = '\0';

	fprintf(stderr, "\"%s\"", escaped_str);
	free(escaped_str);
}


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

void block_sig(pid_t pid)
{
	sigset_t set;
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
	sigaddset(&set, SIGHUP);
	//sigaddset(&set, SIGINT);
	sigaddset(&set, SIGQUIT);
	sigaddset(&set, SIGPIPE);
	sigaddset(&set, SIGTERM);
	sigprocmask(SIG_BLOCK, &set, NULL);
}

int trace_pid(pid_t tracee)
{
	int status;
	struct user_regs_struct regs;
	struct iovec iov;
	siginfo_t siginfo;
	int in_syscall = 0;


	if (ptrace(PTRACE_SEIZE, tracee, NULL, NULL) == -1)
		handle_error("ptrace seize");

	if (ptrace(PTRACE_INTERRUPT, tracee, NULL, NULL) == -1)
		handle_error("ptrace interrupt");

	waitpid(tracee, &status, 0);

	block_sig(tracee);

	if (ptrace(PTRACE_SETOPTIONS, tracee, 0, PTRACE_O_TRACESYSGOOD) == -1)
		handle_error("ptrace setoptions");

	while (1) {
		if (ptrace(PTRACE_SYSCALL, tracee, NULL, NULL) == -1)
			handle_error("ptrace syscall");

		if (waitpid(tracee, &status, 0) == -1)
			handle_error("waitpid");

		if (ptrace(PTRACE_GETSIGINFO, tracee, NULL, &siginfo) == -1)
			handle_error("ptrace getsiginfo");

		if (siginfo.si_signo == SIGTRAP) {

			get_regs(tracee, &regs);

			if (!in_syscall) {

				unsigned long long *args[] = {
					&regs.rdi, &regs.rsi, &regs.rdx,
					&regs.r10, &regs.r8, &regs.r9
				};

				fprintf(stderr, "%s(", syscalls[regs.orig_rax].name);
				for (int i = 0; i < syscalls[regs.orig_rax].num_args; i++) {
					if (i > 0)
						fprintf(stderr, ", ");
					format_arg(syscalls[regs.orig_rax].arg_types[i], (void *)args[i]);
				}
				fprintf(stderr, ")");
			} else {
				fprintf(stderr, " = %#llx\n", regs.rax);
			}	

			in_syscall = !in_syscall;

		} else {
			fprintf(stderr, "--- %s ---\n", strsignal(siginfo.si_signo));
			return (siginfo.si_signo);
		}
	}
}


int	main(int argc, char **argv, char **envp)
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
		return (1);
	}

	pid_t pid = fork();
	int status;

	if (pid == -1)
		handle_error("fork");

	if (pid == 0) {
		//raise(SIGSTOP);
		execve(argv[1], argv + 1, envp);
		handle_error("execvp");
	} else {
		//block_sig(pid);
		status = trace_pid(pid);
		if (WIFSIGNALED(status)) {
			fprintf(stderr, "+++ killed by %s +++\n", strsignal(WTERMSIG(status)));
			kill(getppid(), WTERMSIG(status));
		} else
			fprintf(stderr, "+++ exited with %d +++\n", WEXITSTATUS(status));
	}
	return (WEXITSTATUS(status));
}
