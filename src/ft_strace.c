#include "ft_strace.h"

// PTRACE_SEIZE not using PTRACE_ME

Err_value	ft_strace(char **argv)
{
	pid_t	pid;

	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid == 0) {
		_debug("Child process\n");
		pause();
		_debug("Child process done\n");
		exit(EXIT_SUCCESS);
	} else {
		int status;

		if (ptrace(PTRACE_SEIZE, pid, NULL, NULL) == -1) {
			perror("ptrace");
			exit(EXIT_FAILURE);
		}

		_debug("PTRACE_SEIZE\n");

		if (ptrace(PTRACE_INTERRUPT, pid, NULL, NULL) == -1) {
			perror("ptrace");
			exit(EXIT_FAILURE);
		}

		if (waitpid(pid, &status, 0) == -1) {
			perror("waitpid");
			exit(EXIT_FAILURE);
		}
		if (WIFSTOPPED(status)) {
			_print("Child stopped\n");
		}
	}

	return EXIT_SUCCESS;
}

int	main(int argc, char **argv)
{
	if (argc < 2) {
		_error("Usage: %s [PROGRAM] [ARGS]\n", argv[0]);
		return EXIT_FAILURE;
	}

	ft_strace(argv + 1);
}
