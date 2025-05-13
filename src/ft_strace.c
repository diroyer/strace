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

void trace_pid(pid_t tracee)
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

	if (ptrace(PTRACE_SETOPTIONS, tracee, 0, PTRACE_O_TRACESYSGOOD) == -1)
		handle_error("ptrace setoptions");



	while (1) {
		if (ptrace(PTRACE_SYSCALL, tracee, NULL, NULL) == -1)
			handle_error("ptrace syscall");

		if (waitpid(tracee, &status, 0) == -1)
			handle_error("waitpid");

		if (WIFEXITED(status)) {
			printf("Tracee exited with status %d\n", WEXITSTATUS(status));
			break;
		}

		if (WIFSIGNALED(status)) {
			printf("Tracee killed by signal %d\n", WTERMSIG(status));
			break;
		}

		if (WIFSTOPPED(status)) {

			if (ptrace(PTRACE_GETSIGINFO, tracee, NULL, &siginfo) == -1)
				handle_error("ptrace getsiginfo");

			if ((status >> 8) == (SIGTRAP | 0x80)) {

				get_regs(tracee, &regs);

				if (!in_syscall) {
					printf("Syscall: %lld, ", regs.orig_rax);
				} else {
					printf("exit: %#llx\n", regs.rax);
				}

				in_syscall = !in_syscall;


			} else if (WSTOPSIG(status) != SIGTRAP) {
				printf("Signal stop: %s\n", strsignal(WSTOPSIG(status)));
			}
		}
	}
}

int	main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
		return (1);
	}

	pid_t pid = fork();

	if (pid == -1)
		handle_error("fork");

	if (pid == 0) {

		usleep(50000);
		execvp(argv[1], &argv[1]);
		handle_error("execvp");
	} else {
		trace_pid(pid);
	}
	return (0);
}
