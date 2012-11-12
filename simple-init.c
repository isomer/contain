#include <sys/types.h>
#include <sys/wait.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	switch(fork()) {
		case 0:
			execvp(argv[1], &argv[1]);
			err(1, "Failed to execve(%s)", argv[1]);
		case -1:
			err(1, "Failed to fork()");
		default:
			break;
	}

	/* TODO: If we get a SIGTERM, we should try and clean up our children.
	 * By sending SIGTERM, SIGKILL etc
	 */
			
	/* Wait for all children to exit */
	while (wait(NULL) == -1 && errno != ECHILD)
		;

	return 0;
}
