# Cheaper 9 进程凭证

## 词汇速查

- 特权级进程: privileged process
- 辅助组: supplementary group

## 笔记

**每个进程**都有一套 UID 和 GID，这些 ID 被称为进程凭证。

包括如下：

- 实际用户 ID (real user ID) 和 实际组 ID (real group ID)
- 有效用户 ID (effective user ID) 和 有效组 ID (effective group ID)
- 保存的 set-user-id (saved set-user-id) 和 保存的 set-group-id (saved set-group-id)
- 文件系统用户 ID (file-system user ID) 和 文件系统组 ID (file-system group id) (此为 Linux 专有特性)
- 辅助组 ID

### 实际 UID 和 实际 GID

实际用户和组 ID 即进程所属的用户和组。当用户登录时，登录 Shell 从密码文件中读取相应的用户密码记录的第三子段和第四子段 (即 UID 子段和 GID 子段)，置为实际用户 ID 和组 ID。当创建新进程时将从父进程中继承该值。

### 有效 UID 和有效 GID

当进程执行各种操作时，将结合有效用户 ID，有效组 ID 和辅助组 ID 来共同确定赋予进程的权限。

有效 ID 为 0 的进程拥有超级用户的所有权限，这种进程称为特权级进程。某些系统调用仅支持特权用户运行。

通常有效用户和组 ID 与实际用户和组 ID 相等，但可以通过 syscall 或 set-user-ID 和 set-group-ID 来改变。

### set-user-id 和 set-group-id

set-user-ID (即 setuid) 将把进程的有效用户 ID 置为可执行文件的 UID，从而获得常规情况下不具有的权限，set-group-ID (即 setGID) 将进程的有效 GID 置为可执行文件的 GID。

每个文件都具有额外的 setUID 和 setGID 位，可通过 chmod(1) 来设置。非特权用户仅能对用户本身拥有的程序设置，特权用户或具有特定权限的用户 (CAP_FOWNER) 能对任何文件进行设置。

```sh
chmod u+s prog
chmod g+s prog
```

在 Linux 中 setUID 和 setGID 对 Shell 脚本无效。

常见的 setUID 程序有 `passwd(1)` 用于修改用户密码，`mount(8)` 和 `umount(8)` 用于挂载和卸载文件系统，`su(1)` 允许用户以另一用户的身份运行 Shell，以及 `sudo(1)` 实用程序，用于将命令临时提权执行。

常见的 setGID 程序有 `wall(1)`，用于向 tty 组下的所有终端发送消息。

setUID 特性和 setGID 特性若使用不当可能造成安全隐患。

### 保存 set-user-id 和 保存 set-group-id

该设计用于和 setUID 和 setGID 结合使用。当执行程序时会发生以下事件：

- 若可执行文件的 setUID 或 setGID 权限已开启，则将进程的有效 UID 或有效 GID 置为可执行文件的所属。若未设置 setUID 或 setGID 则保持不变
- 保存 setUID 或 保存 setGID 的值由有效 ID 复制而来，无论程序是否指定 setUID 或 setGID 该复制都将进行。

即：

假设某进程的实际 UID，有效 UID 和 setUID 均为 1000，当其运行了 root 用户拥有的 setUID 程序后，进程的 UID 如下

```sh
real=1000, effective=0, saved=0
```

该特性使程序具有了在拥有特权的用户和不具有特权的用户间来回切换的能力，因为存在转换状态的 syscall。

### 文件系统 UID 和 GID

在 Linux 中执行打开文件，改变文件所属，修改文件权限之类的操作时，决定权限的时文件系统 UID 和文件系统 GID。

通常来说，修改有效 UID 和 有效 GID 时，文件系统 UID 和文件系统 GID 也会跟随绑定变化，仅在使用 Linux 特有的 `setfsuid()` 和 `setfsgid()` 时，才有所不同。

该设计是解决一些历史问题而存在的，现如今已经几乎没有存在价值。多数时候无需对此进行判断。

### 辅助组 ID

辅助组 ID 用于标识进程所属的若干附加组。

### 获取和修改进程凭证

Linux 提供了一系列 syscall，SUSv3 仅对其中一部分做了规范，还有一些在 UNIX 中得到广泛应用，还有一些是 Linux 特有的。

可通过 `/proc/PID/status` 检查其中的 `Uid`，`Gid`，`Groups` 的检查获取进程凭证。

#### 获取和修改实际、有效和保存 setUID

能完成此操作的系统调用有多种，其中有些功能重叠，由于其源于不同的 UNIX 实现。

```c
#include <unistd.h>

uid_t getuid(void); /* return real user ID of calling process */
uid_t geteuid(void); /* return effective user ID of calling process */
gid_t getgid(void);
gid_t getegid(void);
```

`getuid()` 和 `getgid()` 返回调用进程的实际 UID 和 GID，而 `geteuid()` 和 `getegid()` 返回调用进程的有效 UID 和 GID。

以上函数总是成功。

```c
#include <unistd.h>

int setuid(uid_t uid);
int setgid (gid_t gid);
```

调用该函数时，对进程是否具有特权，规则如下：

- 若非特权用户调用 `setuid()`，仅能修改进程的有效用户 ID，且仅能将有效用户 ID 修改为相应的实际用户 ID 或保存 UID。即对非特权用户而言仅在运行 setUID 程序时才能使用该函数。
- 当特权用户用非 0 参数调用 `setuid()` 时，实际 UID，有效 UID 和保存 UID 都将被置为 uid 参数指定的值，即特权用户若调用该函数修改了 UID，则会丢失所有特权，且之后也不能使用 `setuid()` 将权限设为 0。

例如，若要让 setUID 程序放弃 root 权限，则可以使用下列代码：

```c
if (setuid(getuid()) == -1) {
    errExit("setuid);
}
```

若 setUID 的程序属主非 root 用户，则可使用上述调用在实际用户 ID 和保存 setUID 间切换。

```c
#include <unistd.h>

int seteuid(uid_t euid);
int setegid(gid_t egid);
```

使用 seteuid() 和 setegid() 可使进程的有效 UID 或 GID 修改为参数 euid 或 egid 指定的值。

其遵循以下规则：

- 非特权用户仅能将有效 UID 和 GID 修改为相应的实际 ID 或保存 set ID 值。即等效于 setuid() 调用。
- 特权用户可将有效 ID 修改为任意值，若特权进程将有效 UID 修改为非特权，即非 0 值，则当前进程不再具有特权，但可通过规则 1 恢复特权。

即：

```c
euid = geteuid();  /* Save initial effective UID which is the same as saved setUID */

if (seteuid(getuid()) == -1) {  /* Drop privileges */
    errExit("seteuid");
}

if (seteuid(euid) == -1) {  /* Regain privileges */
    errExit("seteuid);
}
```

```c
#include <unistd.h>

int setreuid(uid_t ruid, uid_t euid);
int setregid(gid_t rgid, gid_t egid);
```

以上系统调用允许调用进程独立修改其实际和有效 UID 和 GID。其中 `ruid` 指新的实际 UID，`euid` 指新的有效 UID。若想只修改其中一个，可将另一参数指定为 `-1`。

规则如下：

- 非特权进程只能将实际 UID 设置为当前用户的实际 UID (即保持不变) 或有效 UID 值，只能将有效 UID 设置为当前用户的实际 UID，有效 UID (即保持不变)，或保存 setUID
- 特权进程能够将实际 UID 和有效 UID 设置为任何值。
- 无论进程是否拥有特权，只要下列条件有一条成立，则保存 setUID 会被设置为有效 UID 的值
  - `ruid` 不为 `-1` （即实际 UID 需要有一个设置，即便是设为当前值)
  - 对有效 UID 设置的值不同于系统调用之前的实际 UID

即，若进程使用 `setreuid()` 只将有效 UID 修改为实际 UID 的当前值，则保存 setUID 的值将保持不变。同时由于规则 3，若进程想放弃特权，则可以执行：

```c
setreuid(getuid(), getuid());
```

若 setUID root 的进程需要将进程凭证改为其他任意值，则需要先修改 GID 后再修改 UID，否则由于修改 UID 后进程已经丧失特权，后续调用 `setregid()` 将由于没有特权而失败。

在多数 UNIX 实现中，进程不能直接获取保存 setUID 和 setGID，但 Linux 提供了两个非标准的系统调用实现该功能。

```c
#define _GNU_SOURCE
#include <unistd.h>

int getresuid(uid_t *ruid, uid_t *euid, uid_t *suid);
int getresgid(gid_t *rgid, uid_t *egid, uid_t *sgid);
```

该函数将实际 UID，有效 UID 和保存 setUID 返回给指定 3 个参数所指定的位置，若成功返回 0，否则返回 -1.

```c
#define _GNU_SOURCE
#include <unistd.h>

int setresuid(uid_t ruid, uid_t euid, uid_t suid);
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);
```

以上系统调用允许进程独立修改 3 个 UID 的值，若不想同时修改所有 ID，则将对应不修改的子段设为 -1。

该系统调用的规则如下：

- 非特权用户能够将实际用户 ID，有效用户 ID 和保存 setUID 中的任一 ID 设置为当前值。
- 特权用户可以设置为任何值
- 不管系统调用是否对其他 ID 做了任何改动，总是将文件系统 ID 设置为与有效 ID 相同的值。

该调用要么全部成功，要么全部失败。

```c
#include <sys/fsuid.h>

int setfsuid(uid_t fsuid);
int setfsgid(gid_t fsgid);
```

以上系统调用可以独立修改文件系统 ID。规则如下：

- 非特权进程能将文件系统 ID 设置为实际 ID，有效 ID，文件系统 ID (即保持不变) 或 set ID 的当前值
- 特权用户可以设定为任何值

该函数永远成功，他缺少错误检查，函数返回修改过的文件系统 ID，即如果将文件系统 ID 设置成一个非法的值，他会返回之前的文件系统 ID。由于缺少获取文件系统 ID 的系统调用，因此这是一种获取当前文件系统 ID 的方法。

```c
#include <unistd.h>

int getgroups(int gidsetsize, gid_t grouplist[]);
```

调用程序需要负责为该系统调用中的 `grouplist[]` 数组分配空间，并在 `gidsetsize` 中指定长度。若调用成功，函数返回置于数组中的 GID 数量。

若进程属组的数量超过 `gidseisize`，则该系统调用返回错误 `EINVAL`，`grouplist[]` 的大小可定义为常量 `NGROUPS_MAX+1`，该常量定义了进程属组的最大数量，其定义在 `limits.h` 中。

```c
gid_t grouplist[NGROUPS_MAX + 1];
```

另外，可将 `gidsetsize` 指定为 0，这样一来数组未做修改，但函数会返回进程属组的数量。

```c
#define _BSD_SOURCE
#include <grp.h>

int setgroups(size_t gidsetsize, const gid_t *grouplist);
int initgroups(const char *user, gid_t group);
```

特权级应用可通过上述系统调用修改辅助组 ID 集合。

`setgroups()` 用 `grouplist` 所指数组替换调用进程的辅助组 ID，`gidsetsize` 指定了数组中 GID 的数量。

`initgroups()` 会扫描 `/etc/groups` 文件，为 user 创建属组列表，以此来初始化调用进程的辅助组 ID，同时也会将 `group` 参数指定的组加入到辅助组 ID 的集合中。

## 习题

### 9-1

- a: real=2000, effective=2000, saved=2000, file-system=2000
- b: real=1000, effective=2000, saved=2000, file-system=2000
- c: real=1000, effective=2000, saved=0 file-system=2000
- d: real=1000, effective=0, saved=0, file-system=2000
- e: real=1000, effective=2000, saved=3000, file-system=2000

### 9-2

该进程当前没有特权，但可以通过 `seteuid()` 系统调用重新获取特权。

### 9-3

code: c9/initgroups.c

由于 setgroups() 需要特权，此程序需要 root 运行。

程序的 main() 写死了判断 UID=1000，运行程序前确认系统上存在 UID=1000 的账号。

### 9-4

参考书后答案。

```c
e = geteuid();

setuid(getuid()); /* Suspend privileges */
setuid(e); /* Resume privileges */
/* Can't permanently drop the setUID with setuid() */

seteuid(getuid()); /* Suspend privileges */
seteuid(e); /* Resume privileges */
/* Can't permanently drop the setUID with seteuid() */

setreuid(-1, getuid());
setreuid(-1, e);
setreuid(getuid(), getuid());

setresuid(-1, getuid(), -1);
setresuid(-1, e, -1);
setresuid(getuid(), getuid(), getuid());
```

### 9-5

同参考书后答案

```c
e = geteuid();

/* Can't suspend and resume privileges with setuid() */
setuid(getuid());

seteuid(getuid()); /* Suspend privileges */
seteuid(e); /* Resume privileges */
/* Can't permanently drop the setUID with seteuid() */

setreuid(-1, getuid());
setreuid(-1, e);
setreuid(getuid(), getuid());

setresuid(-1, getuid(), -1);
setresuid(-1, e, -1);
setresuid(getuid(), getuid(), getuid());
```
