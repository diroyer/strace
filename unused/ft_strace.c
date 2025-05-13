#include "ft_strace.h"


/* NOTE: This is a work in progress. */
/* A tracee needs to be attached to a tracer. */
/* The tracer enters the event loop */
/* when the tracer is finished tracing, in can cause the tracee to continue executing in a regular, non-traced mode. */
/* the tracee stops each time a traceable event occurs, these events are: ptrace stops */
/* the tracer is notified using waitpid */
/* that call returns a status value containing information about the tracee's stop */
/* while the tracee is stopped, the tracer can use ptrace commands to inspect and modify the tracee's */
/* the tracer then causes the tracee to continue executing in a traced mode */

/* PTRACE_ATTACH and PTRACE_TRACE_ME are used to attach a tracee to a tracer
 * but they will receive a SIGSTOP signal, and when u are forking ur own process,
 * the SIGSTOP could be conflict with another SIGSTOP signal */

/* PTRACE_SEIZE is used to attach a tracee to a tracer without sending a SIGSTOP signal */
/* PTRACE_INTERRUPT is used to interrupt the tracee replacing the SIGSTOP signal */
/* PTRACE_LISTEN is used to listen for signals from the tracee */
/* PTRACE_SYSCALL is used to continue the tracee after a syscall stop */
/* PTRACE_GETSIGINFO is used to get the signal information */
/* PTRACE_GETREGSET is used to get the register set */
/* PTRACE_SETOPTIONS is used to set the options of the tracee */

/* types of ptrace stops: */
/* - signal delivery stop: just prior to signal delivery */
	/* CONDITIONS: */
	/* - each time a signal is about to be delivered */
	/* - except for SIGKILL which cannot be intercepted */
	/* - the signal is not delivered until the tracer allows it */
	/* - reported bt wait family syscalls with WIGSTOPPED and WSTOPSIG */
	/* ACTIONS: */
	/* - retrieve the signal information with PTRACE_GETSIGINFO */
	/* - the tracer restarts the tracee with PTRACE_restart but in my case I will use PTRACE_SYSCALL ? */
	/* - if sig == 0 then the signal is not delivered */
	/* - if sig != 0 then the signal is delivered */
	/* - injected signal sig can differ from WSTOPSIG */


/* - syscall stop: just after syscall entry or exit */
	/* CONDITIONS: */
	/* - when the tracee is restarted using PTRACE_SYSCALL */
	/* - stops prior to entering the next syscall */
	/* - reported by wait family syscalls with WSTOPSIG == SIGTRAP 
	 * or WSTOPSIG = SIGTRAP | 0x80 if we are using PTRACE_O_TRACESYSGOOD */
	/* easy to distinguish from signal delivery stops */
	/* how to distinguish syscall entry and exit stops ? Keep track of the sequence of ptrace stops
	 * or use PTRACE_GET_SYSCALL_INFO but this is not our case */
	/* Method to obtain syscall information: */
	/* - PTRACE_GETREGS: get the register set old way */
	/* - PTRACE_PEEKUSER: get the register set new way, painfully slow */
	/* - PTRACE_GETREGSET: get the register set new way, but
	 * could be problematic when using different architectures */

	/* to access procces memory we can use PTRACE_PEEKDATA and PTRACE_POKEDATA 
	 * to inspect and modify respectively, we will not use them */

/* group stop: when a tracee receives a signal that stops the entire process group */
	/* CONDITIONS: */
	/* - a stopping signal causes a signal delivery stop */
	/* - after is it injected by the tracer */
	/* - the tracee enters a group stop state */
	/* - reported by wait family syscalls with WSTOPSIG == SIGSTOP */
	/* - PTRACE_GETSIGINFO can be used to tell it from a signal delivery stop */
	/* - when the tracee is attached using PTRACE_SEIZE, then (status >> 16) == PTRACE_EVENT_STOP,
	 * and PTRACE_EVENT_STOP is not needed */
	/* PROBLEMS: */
	/* - the tracee will not run until restarted by the tracer */
	/* - the tracee will not send notifications besides SIGKILL deaths */
	/* - if restarted, the stopping signal is effectively ignored */
	/* - if not restarted future SIGCONT signals will no be reported
	 * and would not have any effect on the tracee */
	/* SOLUTION: */
	/* instead of PTRACE_SYSCALL, use PTRACE_LISTEN to restart the tracee in a way where it does not
	 * execute but wait for new evens (only avaible in PTRACE_SEIZE) */

/* PTRACE_EVENT stop: when a tracee stops due to a ptrace event */

/* Following forks before Linux 2.5.46 */
/* - new process had to be created using clone() syscall with CLONE_PTRACE flag */
/* - on entering clone(), fork() or vfork() the tracer had to change them to clone() with appropriate flags */
/* - strace had to turn vfork() into fork() by stripping the CLONE_VFORK flag */
/* There is new options nowadays like PTRACE_O_TRACECLONE, PTRACE_O_TRACEFORK, PTRACE_O_TRACEVFORK 
 * but we will not use them */

/* Following execs */
/* - all other threads in the process are terminated */
/* - the thread ID of the thread that invoked execve() is set to the thread group ID */
/* - at completion of the system call, it appears as though the execve() occured
 * in the thread group leader, regardless of which thread did the execve() */
/* - this resetting of the thread ID looks very confusing to the tracer */

/* Deatchin a tracee */
/* - PTRACE_DETACH is used to detach a tracee from a tracer but we will not use it */
/* Traditional approach: */
/* - the tracer sends a SIGSTOP signal to the tracee */
/* - then it waits for tracee so stop in a signal delivery stop state for SIGSTOP */
/* - and finally, detach the tracee, supressing the SIGSTOP signal */
/* Problem: */
/* - this can race with concurrent SIGSTOPs */
/* - the tracee may enter other ptrace stops and needs to be restarted and waited for again, until
 * SIGSTOP is seen */
/* Modern approach: */
/* - the tracer forces the tracee to enter a ptrace stop using PTRACE_INTERRUPT */
/* - dependending on the circumstances, its either a signal delivery stop or a syscall stop or PTRACE_EVENT stop */
/* - a this point it's safe to detach the tracee using PTRACE_DETACH */


int	perr_exit(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

char* get_signal_value(int signal) {
	switch (signal) {
		case SIGHUP:
			return "SIGHUP";
		case SIGINT:
			return "SIGINT";
		case SIGQUIT:
			return "SIGQUIT";
		case SIGILL:
			return "SIGILL";
		case SIGTRAP:
			return "SIGTRAP";
		case SIGABRT:
			return "SIGABRT";
		case SIGBUS:
			return "SIGBUS";
		case SIGFPE:
			return "SIGFPE";
		case SIGKILL:
			return "SIGKILL";
		case SIGUSR1:
			return "SIGUSR1";
		case SIGSEGV:
			return "SIGSEGV";
		case SIGUSR2:
			return "SIGUSR2";
		case SIGPIPE:
			return "SIGPIPE";
		case SIGALRM:
			return "SIGALRM";
		case SIGTERM:
			return "SIGTERM";
		case SIGSTKFLT:
			return "SIGSTKFLT";
		case SIGCHLD:
			return "SIGCHLD";
		case SIGCONT:
			return "SIGCONT";
		case SIGSTOP:
			return "SIGSTOP";
		case SIGTSTP:
			return "SIGTSTP";
		case SIGTTIN:
			return "SIGTTIN";
		case SIGTTOU:
			return "SIGTTOU";
		case SIGURG:
			return "SIGURG";
		case SIGXCPU:
			return "SIGXCPU";
		case SIGXFSZ:
			return "SIGXFSZ";
		case SIGVTALRM:
			return "SIGVTALRM";
		case SIGPROF:
			return "SIGPROF";
		case SIGWINCH:
			return "SIGWINCH";
		case SIGIO:
			return "SIGIO";
		case SIGPWR:
			return "SIGPWR";
		case SIGSYS:
			return "SIGSYS";
		default:
			return "UNKNOWN";
	}
}

int ft_tracer(pid_t tracee)
{
	int status;
	struct user_regs_struct regs;
	siginfo_t siginfo;
	struct iovec iov;

	if (ptrace(PTRACE_SEIZE, tracee, NULL, NULL) == -1)
		perr_exit("ptrace seize");
	if (ptrace(PTRACE_INTERRUPT, tracee, NULL, NULL) == -1)
		perr_exit("ptrace interrupt");

	if (waitpid(tracee, &status, 0) == -1)
		perr_exit("waitpid");
	while (1) {

		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			printf("Tracee exited\n");
			return 0;
		}

		if (WIFSTOPPED(status)) {
			int stop_signal = WSTOPSIG(status);
			if (stop_signal == SIGTRAP) {
				iov.iov_base = &regs;
				iov.iov_len = sizeof(regs);


				if (ptrace(PTRACE_GETREGSET, tracee, NT_PRSTATUS, &iov) == -1)
					perr_exit("ptrace getregset");

				printf("Syscall stop: sycall number: %lli\n", regs.orig_rax);

				printf("RIP: %llx, RAX: %lli, RDI: %llx, RSI: %llx, RDX: %llx, R10: %llx, R8: %llx, R9: %llx\n",
						regs.rip, regs.rax, regs.rdi, regs.rsi, regs.rdx, regs.r10, regs.r8, regs.r9);

				if (ptrace(PTRACE_SYSCALL, tracee, NULL, NULL) == -1)
					perr_exit("ptrace syscall");
			} else if (stop_signal == SIGSTOP) {
				printf("Group stop detected restarting tracee with listen\n");

				if (ptrace(PTRACE_LISTEN, tracee, NULL, NULL) == -1)
					perr_exit("ptrace listen");
			} else {
				printf("Signal stop: %s\n", get_signal_value(stop_signal));

				if (ptrace(PTRACE_GETSIGINFO, tracee, NULL, &siginfo) == -1)
					perr_exit("ptrace getsiginfo");

				printf("Signal number: %d, Signal code: %d, Signal errno: %d\n",
						siginfo.si_signo, siginfo.si_code, siginfo.si_errno);

				if (ptrace(PTRACE_SYSCALL, tracee, NULL, NULL) == -1)
					perr_exit("ptrace syscall");
			}
		}
	}

	return 0;
}

	//iov.iov_base = &regs;
	//iov.iov_len = sizeof(regs);
	//
	//if (ptrace(PTRACE_GETREGSET, tracee, NT_PRSTATUS, &iov) == -1)
	//	perr_exit("ptrace getregset");
	//
	//printf("RIP: %llx, RAX: %llx, RDI: %llx, RSI: %llx, RDX: %llx, R10: %llx, R8: %llx, R9: %llx\n",
	//		regs.rip, regs.rax, regs.rdi, regs.rsi, regs.rdx, regs.r10, regs.r8, regs.r9);
	//
	//if (ptrace(PTRACE_SYSCALL, tracee, NULL, NULL) == -1)
	//	perr_exit("ptrace syscall");
	//
	//if (ptrace(PTRACE_LISTEN, tracee, NULL, NULL) == -1)
	//	perr_exit("ptrace listen");
	//
	//goto loop;



int	main(int argc, char **argv, char **envp)
{
	if (argc < 2) {
		_error("Usage: %s [PROGRAM] [ARGS]\n", argv[0]);
		return EXIT_FAILURE;
	}

	pid_t tracee = fork();
	if (tracee == -1) perr_exit("fork tracee");

	if (tracee == 0) {
		//raise(SIGSTOP);
		execve(argv[1], argv + 1, envp);
		perr_exit("execve");

	} else {
		ft_tracer(tracee);
	}

	//pid_t tracee = fork();
	//if (tracee == -1) perr_exit("fork tracee");
	//
	//if (tracee == 0) {
	//	while (1)
	//		pause();
	//}
	//
	//pid_t tracer = fork();
	//if (tracer == -1) perr_exit("fork tracer");
	//
	//if (tracer == 0) {
	//	ft_tracer(tracee);
	//}
}

