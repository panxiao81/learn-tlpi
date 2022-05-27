#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/fcntl.h>

struct iovec
{
    void *iov_base;
    size_t iov_len;
};

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    size_t spaceSize = 0, p = 0;
    ssize_t numRead;
    void *space;
    for (int i = 0; i < iovcnt; i++)
    {
        spaceSize += (iov + i)->iov_len;
    }

    space = malloc(spaceSize);
    if (space == NULL)
    {
        return -1;
    }

    memset(space, 0, spaceSize);

    if ((numRead = read(fd, space, spaceSize)) == -1)
    {
        free(space);
        return -1;
    }
    else if (numRead == 0)
    {
        free(space);
        return 0;
    }

    for (int i = 0; i < iovcnt; i++)
    {
        if (numRead - p < (iov + i)->iov_len)
        {
            memcpy((iov + i)->iov_base, space + p, numRead - p);
            break;
        }
        memcpy((iov + i)->iov_base, space + p, (iov + i)->iov_len);
        p += (iov + i)->iov_len;
    }
    free(space);
    return numRead;
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    size_t spaceSize = 0, p = 0;
    ssize_t numWrite;
    void *space;
    for (int i = 0; i < iovcnt; i++)
    {
        spaceSize += (iov + i)->iov_len;
    }
    space = malloc(spaceSize);
    if (space == NULL)
    {
        return -1;
    }

    for (int i = 0; i < iovcnt; i++)
    {
        memcpy(space + p, (iov + i)->iov_base, (iov + i)->iov_len);
        p += (iov + i)->iov_len;
    }

    numWrite = write(fd, space, spaceSize);
    free(space);
    return numWrite;
}

int main() {}