#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <unistd.h>

#define FILENAME "test"
#define MAGIC_NUMBER 114514

int main(int argc, const char *argv[])
{
    char *filename = FILENAME;
    int fd, fd2, accmode1, accmode2, seek1, seek2;
    fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }
    fd2 = dup(fd);

    accmode1 = fcntl(fd, F_GETFL);
    accmode2 = fcntl(fd2, F_GETFL);

    if (accmode1 == -1 || accmode2 == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    if (accmode1 == accmode2)
    {
        printf("Access Mode is the same\n");
    }
    else
    {
        printf("Access mode is not the same.\n");
    }

    if (lseek(fd, MAGIC_NUMBER, SEEK_SET) == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }

    seek1 = lseek(fd, 0, SEEK_CUR);
    seek2 = lseek(fd2, 0, SEEK_CUR);

    if (seek1 == -1 || seek2 == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }

    if (seek1 == seek2)
    {
        printf("Seek is the same.\n");
    }
    else
    {
        printf("Seek is not the same.\n");
    }

    return EXIT_SUCCESS;
}