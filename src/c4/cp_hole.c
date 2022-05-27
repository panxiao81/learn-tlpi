#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include "tlpi_hdr.h"

#define BUF_SIZE 1024

void help()
{
    usageErr(
        "cp_hole [-h] filename_1 filename_2\n",
        "\t Copy a file with file hole\n",
        "\t-h\t display this help");
}

int main(int argc, char const *argv[])
{
    int opt, fd1, fd2, numRead;
    char buf[BUF_SIZE];

    while ((opt = getopt(argc, argv, "h")) != -1)
    {
        switch (opt)
        {
        case 'h':
            help();
            exit(EXIT_SUCCESS);
            break;

        default:
            usageErr("Can't not parse arguments\n");
            help();
            exit(EXIT_FAILURE);
            break;
        }
    }

    if (optind < argc)
    {
        if (optind + 1 >= argc)
        {
            usageErr("Too few arguments!\n");
            help();
            exit(EXIT_FAILURE);
        }
        fd1 = open(argv[optind], O_RDONLY);
        if (fd1 == -1)
        {
            perror("open");
            exit(EXIT_FAILURE);
        }
        fd2 = open(argv[++optind], O_RDWR | O_CREAT | O_TRUNC, 00644);
        if (fd2 == -1)
        {
            perror("open file 2");
            exit(EXIT_FAILURE);
        }
    }

    while ((numRead = read(fd1, buf, BUF_SIZE)) > 0)
    {

        int i = 0, j;
        while (i < numRead)
        {
            /* Find non-zero bytes, find the start point of the hole. */
            for (j = i; j < numRead; j++)
            {
                if (buf[j] == '\0')
                {
                    break;
                }
            }
            size_t nonZeroBytes = (size_t)j - i;
            if (write(fd2, &buf[i], nonZeroBytes) != nonZeroBytes)
            {
                perror("write");
                exit(EXIT_FAILURE);
            }
            /* Count how many non-zero bytes in there. */
            for (i = j; i < numRead; i++)
            {
                if (buf[i] != '\0')
                {
                    break;
                }
            }
            off_t zeroBytes = (off_t)i - j;
            /* Seek bytes */
            if (lseek(fd2, zeroBytes, SEEK_CUR) == -1)
            {
                perror("seek");
                exit(EXIT_FAILURE);
            }
            /* Recursion */
        }
    }
    if (numRead == -1)
    {
        perror(read);
        exit(EXIT_FAILURE);
    }
    if (close(fd1) == -1)
    {
        perror("close file 1");
    }
    if (close(fd2) == -1)
    {
        perror("close file 2");
    }
    exit(EXIT_SUCCESS);
    return 0;
}
