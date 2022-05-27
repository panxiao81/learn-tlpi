#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include "tlpi_hdr.h"

void help()
{
    fprintf(stderr, "atomic_append, Exerise 5-3 in TLPI.\n(c)Pan Xiao\n");
    usageErr("atomic_append [-h] filename num-bytes [x]\n-h\tDisplay this Help.\n");
}

int main(int argc, char *argv[])
{
    int opt, numBytes, x = 0, fd;
    /* Opt */
    while ((opt = getopt(argc, argv, "h")) != -1)
    {
        switch (opt)
        {
        case 'h':
            help();
            exit(EXIT_SUCCESS);
            break;
        default:
            fprintf(stderr, "Can't parse arguments!\n");
            help();
            exit(EXIT_FAILURE);
        }
    }
    if (argc - optind > 3)
    {
        fprintf(stderr, "Too many arguments!\n");
        help();
        exit(EXIT_FAILURE);
    }
    else if (argc - optind < 2)
    {
        fprintf(stderr, "Too few arguments!\n");
        help();
        exit(EXIT_FAILURE);
    }
    char *filename = argv[optind];
    numBytes = atoi(argv[optind + 1]);
    if (optind + 2 < argc)
    {
        x = 1;
        printf("Open file without O_APPEND\n");
    }
    /* printf("%s, %d, %d", filename, numBytes, x); */

    if (x)
    {
        fd = open(filename, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR);
    }
    else
    {
        fd = open(filename, O_RDWR | O_APPEND | O_CREAT, S_IWUSR | S_IRUSR);
    }
    if (fd == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numBytes; i++)
    {
        if (x)
        {
            if (lseek(fd, 0, SEEK_END) == -1)
            {
                perror("lseek");
                exit(EXIT_FAILURE);
            }
        }
        if (write(fd, "t", 1) != 1)
        {
            perror("write");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}