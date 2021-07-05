#include "singleton.h"
#include <cstdio>
#include <cstdlib>

#include <fcntl.h>
#include <string>


#ifdef _WIN32
#include <Windows.h>
#define MUTEX_NAME	L"Local\\KIM_DUMPHELPER_MUTEX"
#else
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#endif // _WIN32



namespace dump_helper {

#ifdef _WIN32
	int proc_is_exist()
	{
		HANDLE hMutex = NULL;
		hMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
		if (hMutex != NULL)
		{
			if (ERROR_ALREADY_EXISTS == GetLastError())
			{
				ReleaseMutex(hMutex);
				return 1;
			}
		}
		return 0;
	}

#else
	static int lockfile(int fd)
	{
		struct flock fl;

		fl.l_type = F_WRLCK;
		fl.l_start = 0;
		fl.l_whence = SEEK_SET;
		fl.l_len = 0;

		return(fcntl(fd, F_SETLK, &fl));
	}


	int proc_is_exist()
	{
		int  fd;
		char buf[16];
		char filename[100];

		sprintf(filename, "/var/run/%s.pid", "kim_dumpHelper");

		fd = open(filename, O_RDWR | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
		if (fd < 0) {
			return 1;
		}

		if (lockfile(fd) == -1) {
			close(fd);
			return 1;
		}
		else {
			ftruncate(fd, 0);
			sprintf(buf, "%ld", (long)getpid());
			write(fd, buf, strlen(buf) + 1);
			return 0;
		}
	}
#endif // _WIN32
}




