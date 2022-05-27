#include <sys/types.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

int dup(int oldfd)
{
    return fcntl(oldfd, F_DUPFD, 0);
}

int dup2(int oldfd, int newfd)
{
    if ((fcntl(oldfd, F_GETFL)) == -1)
    {
        errno = EBADF;
        return -1;
    }
    if (oldfd == newfd)
    {
        return oldfd;
    }
    if ((fcntl(newfd, F_GETFL)) != -1)
    {
        close(newfd);
    }
    return fcntl(oldfd, F_DUPFD, newfd);
}

int main() {}