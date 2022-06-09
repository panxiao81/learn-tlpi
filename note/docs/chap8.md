# Cheaper 8 用户和组

## 词汇速查

## 笔记

### /etc/passwd

对于每个账户，在系统密码文件即 `/etc/passwd` 中有一列进行描述，他可能长成这个样子

```shell
panxiao81:x:1000:1000:,,,:/home/panxiao81:/bin/bash
```

每个字段用 `:` 分割，这 7 个字段分别为

- 登录名：登录系统时使用的名称，即账号名
- 经过加密的密码：该字段保存用户的加密密码，但现如今的系统一般启用专用的 shadow 密码文件，该字段通常会使用 `x` 字符占位，而实际的密码会加密并保存在 `/etc/shadow` 中。即便启用 shadow 密码，若此处密码为空，则仍允许用户无密码登录系统。
- 用户 ID (UID)：用户的数值型 ID，若 UID=0 则该账号拥有特权，一般该账号只有一个，且登录名为 `root`。早期 Linux 中 UID 为 16 位值，当前 Linux 中 UID 为 32 位。
- 组 ID (GID)：用户属组中首选属组的数值型 ID。进一步关系会在 `/etc/group` 中定义
- 注释：存放用户的描述性文字
- 主目录：用户登录后所处的初始路径，且会以此字段内容设置 HOME 环境变量
- 登录 Shell：用户登录后交给此程序控制。通常该程序是一个 Shell，但也可以是其他程序。若该字段为空，则默认程序为 `/bin/sh`。会以此值来设定 SHELL 环境变量。

在单机系统中用户密码存储在 `/etc/passwd` 中，但若使用了 NIS 或 LDAP 等认证方式则部分信息可能由服务器保存。对程序来说，只要使用标准库函数，则该过程对程序是透明的。

### /etc/shadow

由于程序需要读取用户信息，因此 `/etc/passwd` 不得不对所有用户开放访问权限，若将密码保存在 `/etc/passwd` 中是非常不安全的。因此将密码单独存放至 `/etc/shadow` 文件中，并限制该文件仅允许特权用户访问。

`shadow` 文件通常包含登录名，经过加密的密码，以及其他与安全相关的字段。

SUSv3 并未标准化 `/etc/shadow`。因此并非所有 UNIX 都提供这个特性。且在不同的 UNIX 之间实现也未必相同。

### /etc/group

用户的组由两部分构成，一是密码文件中用户记录的组 ID 字段，二是组文件中列出的用户所属的各个组。

由于早期 UNIX 只允许用户隶属于一个组，在 4.2BSD 中引入了 `并发多属组` 概念，使一个用户可以隶属多个组。且 POSIX.1-1990 对其进行了标准化。这时，组文件会列出用户所属的其他属组。

`/etc/group` 中包含 4 个字段，且每个字段同样使用 `:` 分割。

- 组名：组的名称
- 经过加密处理的密码：组也可以设置密码，但当今已经很少使用组密码特性。在启用 `shadow` 文件的情况下，组密码使用占位，而实际的密码存放在 `/etc/gshadow` 中。
- 组 ID (GID)：该组的数值型 ID。通常对于 GID=0 只定义一个名为 `root` 的组。类似的，GID 当前也为 32 位值。
- 用户列表：属于该组的用户列表，使用逗号分割

### 获取用户和组的信息

```c
#include <pwd.h>

struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);
```

该函数从密码文件中获取记录。

为 `name` 提供一个用户名，或为 `uid` 提供一个 UID，函数返回一个指向如下类型的结构体的指针

```c
struct passwd {
    char *pw_name; /* Login name (username) */
    char *pw_passwd; /* Encrypted passwd */
    uid_t pw_uid; /* User ID */
    gid_t pw_gid; /* Group ID */
    char *pw_gecos; /* Comment (user information) */
    char *pw_dir; /* Initial working (home) directory */
    char *pw_shell; /* Login shell */
};
```

仅当未启用 `shadow` 密码的情况下，`pw_passwd` 才包含有效信息。可在成功调用 `getpwnam()` 后调用 `getspnam()`，并检查是否能够返回 `shadow` 密码记录来判断是否启用 `shadow` 密码。

> `pw_gecos` 字段名字的来源是早期的 UNIX 实现，该字段用于和 GECOS 的计算机通信使用，该名字沿用至今，而用途变为记录用户的信息。

该函数返回的指针指向的数据结构是在内存中静态分配的空间，该函数是不可重入的。

SUSv3 规定若 `passwd` 未发现匹配记录，则函数返回 `NULL` 且不改变 `errno` 值，因此可以使用如下 code snippet 区分出错和未发现匹配记录

```c
struct passwd *pwd;

errno = 0;
pwd = getpwnam(name);

if (pwd == NULL) {
    if (errno == 0)
        /* not found */;
    else 
        /* Error */;
}
```

但实际实现中，不少实现并未遵守标准，若要区分这两种情况，保险期间需要查阅当前系统的 man 手册。

```c
#include <pwd.h>

struct group *getgrnam(const char *name);
struct group *getgrgid(gid_t gid);
```

该函数从组文件中获取记录。类似的，两个函数通过组名或组 ID 来获取信息，返回一个指向下列结构体的指针

```c
struct group {
    char *gr_name;  /* Group name */
    char *gr_passwd;  /* Encrypted passwd */
    gid_t gr_gid;   /* Group ID */
    char **gr_mem;  /* NULL-terminated array of pointers to names of members listed in /etc/group */
};
```

该函数同样不可重入。

若未发现匹配记录，行为与前述函数类似。

### 遍历 passwd 文件

```c
#include <pwd.h>

struct passwd *getpwent(void);

void setpwent(void);
void endpwent(void);
```

`getpwent()` 函数从密码文件逐条返回记录，当出错或到文件末端时返回 NULL。`getpwent()` 调用后会打开密码文件，当处理完毕后需调用 `endpwent()` 关闭文件。

可使用如下 code snippet 遍历密码文件，并打印用户名和用户 ID

```c
struct passwd *pwd;

while ((pwd = getpwent()) != NULL) {
    printf("%-8s %5ld \n", pwd->pw_name, (long)pwd->pw_uid);
}

endpwent();
```

可以使用 `setpwent()` 函数返回文件起始处。

`getgrent()`, `setgrent()` 和 `endgrent()` 与上述类似但对组密码文件进行类似操作。

### 存取 shadow 文件

```c
#include <shadow.h>

struct spwd *getspnam(const char *name);

struct spwd *getspent(void);
void setspent(void);
void endspent(void);
```

以上函数使用方式与 `passwd` 一套类似。

`spwd` 结构体的结构如下：

```c
struct spwd {
    char *sp_namp;  /* Login name */
    char *sp_pwdp;  /* Encrypted passwd */
    /* 以下用于密码有效期等设定 */
    long sp_lstchg; /* Time of last password change (days since 1 Jan. 1970) */
    long sp_min;    /* Min. number of days between password changes */
    long sp_max;    /* Max. number of days before change required */
    long sp_warn;   /* Number of days beforehand that user is warned of upcoming password expiration */
    long sp_inact;  /* Number of days after expiration that account is considered inactive and locked */
    long sp_expire; /* Date when account expires (days since 1 Jan. 1970) */
    unsigned long sp_flag; /* Reserved for future use */
};
```

### 用户加密与用户认证

若程序需要读取 shadow 密码，由于安全原因，UNIX 使用单向加密算法对密码进行加密。因此验证密码的唯一方法是将密码的明文字符串使用同一算法加密，并与加密后的字符串与 `shadow` 中存储的密码进行比较。

加密算法封装在 `crypt()` 函数中。

```c
#define _XOPEN_SOURCE
#include <unistd.h>

char *crypt(const char *key, const char *salt);
```

`crypt()` 接受一个密码，和一个算法的变体。`salt` 指向一个至少两字符的字符串，用来改变算法使用的加密强度。该函数返回一个指向字符串的指针，内容为加密后的密码。

可将从 `shadow` 中获取的密码直接作为 `salt` 参数。

在使用 `crypt` 函数时需在编译时开启 `-lcrypt` 选项，链接 `crypt` 库。

可使用 `getpass()` 函数从终端获取密码。

```c
#define _BSD_SOURCE
#include <unistd.h>

char *getpass(const char *prompt);
```

该函数先屏蔽回显功能，并停止对终端特殊字符串的处理，之后打印 `prompt` 参数指向的字符串，读取一行输入并返回该字符串。(不含结尾的换行符)
返回前，该函数将终端设置复原。

该函数不可重入。

为安全起见，在使用完明文密码后应立即将明文从内存中抹去，否则程序崩溃时可从内核转储中读取明文密码。

### 练习

#### 8-1

两次 `getowuid()` 调用均发生在 `printf()` 构建字符串之前，而 `getpwuid()` 函数是不可重入的，即第二次调用时结果被覆盖了。

#### 8-2

code: c8/getpwnam.c

非常简单。

姑且还是做了空指针判断，但其实不判也没什么大问题。

姑且写了 Test，可以运行程序查看用法。

