#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "tlpi_hdr.h"

#define BUF_SIZE 1024

void help()
{
    usageErr(
        "tee [-a] [-h] filename\n",
        "\t Copy from stdin, then outout to the file and stdout\n",
        "\t-a\tappend to the end of file, not overwrite it."
        "\t-h\t display this help");
}

int main(int argc, char const *argv[])
{
    int opt, append = 0, fd, numRead;
    void *buf;
    if ((buf = malloc(BUF_SIZE)) == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    /* Argument */
    while ((opt = getopt(argc, argv, "ah")) != -1)
    {
        switch (opt)
        {
        case 'a':
            append = 1;
            break;
        case 'h':
            help();
            exit(EXIT_SUCCESS);
            break;
        default:
            usageErr("Can't parse the arguments\n");
            help();
            exit(EXIT_FAILURE);
            break;
        }
    }

    /* Get the filename */
    if (optind < argc)
    {
        const char *file = argv[optind];
        /* printf("file=%s\noptind=%d\nargc=%d\n", file, optind, argc); */

        /* Open file */
        if (append)
        {
            fd = open(file, O_RDWR | O_CREAT | O_APPEND, 00644);
        }
        else
        {
            fd = open(file, O_RDWR | O_CREAT | O_TRUNC, 00644);
        }
        if (fd == -1)
        {
            perror("open");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        usageErr("Need a filename!\n");
        exit(EXIT_FAILURE);
    }

    while ((numRead = read(STDIN_FILENO, buf, BUF_SIZE)) > 0)
    {
        if (write(fd, buf, BUF_SIZE) != numRead)
        {
            perror("write file");
            if (close(fd) == 0)
            {
                perror("close file");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_FAILURE);
        }
        if (write(STDOUT_FILENO, buf, BUF_SIZE) != numRead)
        {
            perror("write stdout");
            exit(EXIT_FAILURE);
        }
    }
    if (close(fd) == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    free(buf);
    exit(EXIT_SUCCESS);
    return 0;
}
