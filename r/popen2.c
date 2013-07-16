/*
 * popen2.c - Run a program with an input and an output pipe
 *
 * Written 2013 by Werner Almesberger <werner@almesberger.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "popen2.h"


int popen2(const char *cmd, FILE *file[2])
{
	int in[2], out[2];
	int error;
	pid_t pid;

	if (pipe(in) < 0)
		return -1;
	if (pipe(out) < 0) {
		error = errno;
		goto fail_pipe_out;
	}

	pid = fork();
	if (pid < 0) {
		error = errno;
		goto fail_fork;
	}

	if (!pid) {
		close(out[0]);
		close(in[1]);
		if (dup2(in[0], 0) < 0) {
			perror("dup2");
			_exit(1);
		}
		if (dup2(out[1], 1) < 0) {
			perror("dup2");
			_exit(1);
		}
		execlp("sh", "sh", "-c", cmd, NULL);
		perror("execl");
		_exit(1);
	}

	close(out[1]);
	close(in[0]);

	file[0] = fdopen(out[0], "r");
	if (!file[0]) {
		error = errno;
		goto fail_file_out;
	}
	file[1] = fdopen(in[1], "w");
	if (!file[1]) {
		error = errno;
		goto fail_file_in;
	}

	return 0;

	fclose(file[1]);

fail_file_in:
	fclose(file[0]);

fail_file_out:
	/*
	 * we'll get EBADF for the pipe ends we've already closed, but that's
	 * okay since there is no risk of this interfering with anything else.
	 */

fail_fork:
	close(out[0]);
	close(out[1]);

fail_pipe_out:
	close(in[0]);
	close(in[1]);

	errno = error;
	return -1;
}
