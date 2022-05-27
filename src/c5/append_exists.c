#include <stdlib.h>
#include <stdio.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include "tlpi_hdr.h"

#define FILENAME "test"
#define BUF_SUZE 1024

void help()
{
    usageErr("append_exists: Exerise 5-2 in TLPI\n\
        Usage: appenbd_exists [-h] [-f filename]\n\
        -h\tDisplay this help\n\
        -f\tChoose file.\n");
}

int main(int argc, char *argv[])
{
    int fd, urandomFd, opt;
    char filename[] = FILENAME;
    char buf[BUF_SUZE];

    while ((opt = getopt(argc, argv, "hf::")) != -1)
    {
        switch (opt)
        {
        case 'h':
            help();
            exit(EXIT_SUCCESS);
            break;
        case 'f':
            strcpy(filename, optarg);
            break;
        default:
            usageErr("can't parse arguments\n");
            help();
            exit(EXIT_FAILURE);
        }
    }

    fd = open(filename, O_RDWR | O_APPEND);
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("lseek");
        exit(EXIT_FAILURE);
    }

    urandomFd = open("/dev/urandom", O_RDONLY);
    if (urandomFd == -1)
    {
        perror("urandom");
        exit(EXIT_FAILURE);
    }

    if (read(urandomFd, buf, BUF_SUZE) == -1)
    {
        perror("read urandom");
        exit(EXIT_FAILURE);
    }

    if (write(fd, buf, BUF_SUZE) != BUF_SUZE)
    {
        perror("write");
        exit(EXIT_FAILURE);
    }
    return 0;
}
