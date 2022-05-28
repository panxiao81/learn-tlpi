#include <string.h>
#include <errno.h>
#include <stdlib.h>

int unsetenv(const char *name)
{
    if (name == NULL || strlen(name) == 0 || strchr(name, '=') != NULL)
    {
        errno = EINVAL;
        return -1;
    }

    extern char **environ;
    size_t len = strlen(name);

    for (char **ep = environ; *ep != NULL;)
    {
        if (strncmp(name, *ep, len) == 0 && *ep[len] == '=')
        {
            for (char **sp = ep; *sp != NULL; sp++)
            {
                *sp = *(sp + 1);
            }
        }
        else
        {
            ep++;
        }
    }
    return 0;
}

int setenv(const char *name, const char *value, int overwrite)
{
    if (name == NULL || value == NULL || strlen(name) == 0 || strchr(name, '=') != NULL)
    {
        errno = EINVAL;
        return -1;
    }

    extern char **environ;

    if (getenv(name) != NULL && overwrite == 0)
    {
        return 0;
    }

    unsetenv(name);

    char *env = malloc(strlen(name) + strlen(value) + 2);
    /* +2 is '=' and '\0' */
    if (env == NULL)
    {
        return -1;
    }

    strcpy(env, name);
    strcat(env, "=");
    strcat(env, value);

    return (putenv(env) == 0) ? 0 : -1;
}

int main() {}