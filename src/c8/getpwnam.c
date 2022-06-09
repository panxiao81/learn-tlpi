#include <unistd.h>
#include <pwd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>

struct passwd *getpwnam(const char *name) {
    if (name == NULL)
        return NULL;
    struct passwd *pwd;

    while((pwd = getpwent()) != NULL) {
        if (strcmp(pwd->pw_name, name) == 0) {
            endpwent();
            return pwd;
        }
    }
    return NULL;
}

void help() {
    fprintf(stderr, "Test getpwnam\nUsage: getpwnam [-h] username\n\t-h Display this help\n");
    return;
}

int main(int argc, char *argv[]) {
    struct passwd *pwd;
    int opt;
    char *name;
    while ((opt = getopt(argc, argv, "h")) != -1)
    {
        switch (opt)
        {
            case 'h':
                help();
                exit(EXIT_SUCCESS);
                break;
            default:
                fprintf(stderr, "can't parse arguments\n");
                help();
                exit(EXIT_FAILURE);
        }
    }
    if (optind >= argc) {
        help();
        return EXIT_FAILURE;
    }
    else {
        name = argv[optind];
    }

    pwd = getpwnam(name);
    if(pwd != NULL) {
        printf("UID=%ld\n", (long)pwd->pw_uid);
    }
    else {
        fprintf(stderr, "Error when getting pwd\n");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
