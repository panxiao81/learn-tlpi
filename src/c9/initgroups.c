#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int initgroups(const char *user, gid_t group)
{
    if (user == NULL) {
        return -1;
    }
    
    gid_t groupslist[NGROUPS_MAX + 1] = {0};
    size_t groupsSize = 0;

    /* Traverse /etc/group and find if the user is belongs to that group */
    /* set errno = 0 to know that if getgrent() throws an error. */
    struct group *structgroup;
    for (errno = 0;structgroup = getgrent();)
    {
        if (errno != 0)
        {
            return -1;
        }

        char **gr = structgroup->gr_mem;
        if (gr == NULL)
        {
            return -1;
        }

        for(int i = 0;gr[i] != NULL;i++)
        {
            if (strcmp(gr[i], user) == 0)
            {
                groupslist[groupsSize] = structgroup->gr_gid;
                groupsSize++;
            }
        }
    }

    /* Add group from function */
    groupslist[groupsSize] = group;
    groupsSize++;

    /* Set supplementary group. */
    if (setgroups(groupsSize, groupslist) == -1)
    {
        return -1;
    }

    return 0;
}

int main(void)
{
    /* Test, use uid=1000, get the supplementary groups list with UID=1000's group */
    /* This program should be run as root. */
    if(geteuid() != 0) {
        fprintf(stderr, "Need to run as root\n");
        return EXIT_FAILURE;
    }
    struct passwd *user = getpwuid(1000);
    if(user == NULL) {
        perror("getpwuid");
        return EXIT_FAILURE;
    }
    if (initgroups(user->pw_name, user->pw_gid) == -1) {
        perror("self initgroups");
        return EXIT_FAILURE;
    }

    gid_t grouplists[NGROUPS_MAX + 1];
    int num;
    if((num = getgroups(NGROUPS_MAX, grouplists)) == -1) {
        perror("getgroups");
        return EXIT_FAILURE;
    }

    for(int i = 0;i < num;i++) {
        printf("%d, ", grouplists[i]);
    }
    printf("\n");
    return 0;




}