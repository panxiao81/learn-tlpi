# Cheaper 3 系统编程概念

## 词汇速查

系统调用 System Calls

外壳函数 wrapper function

例程 routine 指在程序运行时被反复调用和运行的代码

可移植的 portable

可移植性 portabilty


## 笔记

无论何时，只要执行了系统调用或库函数，都应检查调用的返回状态确定调用是否成功，这是编程铁率

要执行内核态代码，程序需要将系统调用复制到寄存器，然后调用机器中断 `int 0x80` 使 CPU 切换到内核态，并执行寄存器指向的系统调用的代码

若出错，则外壳函数使用这个值设定全局变量 `errno` 并返回主调，并返回一个整形值用于判断系统调用是否成功

通常对于 Linux 来说，当系统调用失败，系统调用例程会对相应的 `errno` 取反，返回一个负值，外壳函数对其再次取反，并将结果附给 `errno`，同时返回 `-1`

有些函数不遵守这个惯例，需要注意特殊情况

系统调用存在开销，频繁的系统调用虽然不慢，但比纯粹的用户态代码要慢的多。

库函数有些会使用系统调用，是系统调用的二次封装

可以通过直接运行 glibc 库文件来确定 glibc 版本。

```shell
$ ldd /bin/ls
	linux-vdso.so.1 (0x00007ffc39cee000)
	libselinux.so.1 => /lib/x86_64-linux-gnu/libselinux.so.1 (0x00007f4f4f7bb000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f4f4f593000)
	libpcre2-8.so.0 => /lib/x86_64-linux-gnu/libpcre2-8.so.0 (0x00007f4f4f4fc000)
	/lib64/ld-linux-x86-64.so.2 (0x00007f4f4f81e000)
$ /lib/x86_64-linux-gnu/libc.so.6 
GNU C Library (Ubuntu GLIBC 2.35-0ubuntu3) stable release version 2.35.
Copyright (C) 2022 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.
Compiled by GNU CC version 11.2.0.
libc ABIs: UNIQUE IFUNC ABSOLUTE
For bug reporting instructions, please see:
<https://bugs.launchpad.net/ubuntu/+source/glibc/+bugs>.
```
要在编译期确定 GLIBC 版本，可使用 `__GLIBC__` 和 `__GLIBC_MINOR__` 两个定义的常量，第一个返回大版本号，第二个返回小版本号

在程序运行时，可调用 `gnu_get_libc_version()` 函数，该函数包含在 `gnu/libc-version.h` 中，返回一个指向版本号字符串的指针

### 错误检查

#### 检查系统调用的错误

每个函数的手册里都有 ERROR 部分，可查看其返回值哪些表示错误。如

```c
fd = open(pathname, flags, mode);
if (fd == -1)
{
    /* code to handle the error */
}
if (close(fd) == -1)
{
    /* code to handle the error */
}
```

系统调用失败时会将全局变量 `errno` 设为一个正值用来标识具体的错误，在 `errno.h` 中包含了 `errno` 的声明，以及一组错误编号定义的常量。如

```c
cnt = read(fd, buf, numbytes);
if (cnt == -1) {
    if (errno == EINTR)
        fprintf(stderr, "read was interrupted by a signal\n");
    else {
        /* Some other error occurred */
    }
}
```

如果系统调用或函数成功，则 `errno` 不会被重置为 0，因此如果 `errno` 当前不为 0 可能是上次调用失败造成的。

另外，根据 SUSv3，允许函数调用成功时将 `errno` 设为非 0 值，标准的做法是先检查函数的返回值是否返回错误，之后再通过 `errno` 判断错误原因。

有些系统调用在调用成功后也返回 `-1` 这些特殊的例子中，应在调用这些函数前将 `errno` 初始化为 0，并在调用后对其进行检查。

常用的操作有，根据 `errno` 值打印错误原因，标准库提供 `perror()` 函数和 `strerror()` 函数。

`perror()` 函数会打印传入的指针指向的字符串，并跟上当前 `errno` 值对应的错误信息，

```c
#include <stdio.h>

void perror(const char *msg);
```

例如

```c
fd = open(pathname, flags, mode);
if (fd == -1) {
    perror("open");
    exit(EXIT_FAILURE);
}
```

`strerror()` 对参数 `errnum` 传入的错误号，返回一个指向相应的错误的字符串的指针。

```c
#include <string,h>

char *strerror(int errnum);
```

这个返回的指针指向的字符串可能是静态分配的，后续再次调用可能会覆盖该字符串

#### 对库函数的错误处理

不同的库函数在发生错误时返回的值和表现都不相同

有些库函数返回错误的方式与系统调用完全相同，有些返回的值不同，但对 `errno` 的对待方式相同，而有些函数则完全不使用 `errno` 进行错误处理

### 可移植性问题

系统调用和库函数 API 的特性受标准影响，这些有一部分是 SUS 标准规定的，还有一些是 BSD 和 SVR4 的事实标准

在程序中可以使用特定的宏来使函数使用特定标准的定义，可在包含任何头文件之前先定义特定的特性测试宏 (Feature Test Nacros), 例如

```c
#define _BSD_SOURCE 1
```

也可以在编译时定义

```shell
$ cc -D_BSD_SOURCE prog.c
```

在函数文档中通常有关于使用何种定义的描述。在 `feature_test_macro(7)` 和 `features.h` 中有更多描述

由于数据类型在不同的 Unix 版本甚至是同个系统的不同版本中可能长度不同，为降低移植难度，SUSv3 规范了标准系统数据类型，其使用 `typedef` 定义，例如表示进程号

```c
typedef int pid_t
```

多数类型都定义在 `sys/types.h` 中，其余则定义在其他头文件中。SUSv3 要求所有类型均为运算类型 (arithmetic type)，因此其类型要么为整型，要么为浮点数类型。

由于 `printf()` 在解释传入的数据时不会判断其类型，因此在使用时通常先强制转换为 `long` 类型，再使用 `%ld` 描述符。但有时 `off_t` 类型为 `long long`，因此要强制转换为 `long long` 再使用 `%lld` 描述符。

每种 Unix 实现都明确定义了一些标准的结构体，但顺序是没有明确定义的，且有时还会包含标准之外的特殊的字段，因此不要在初始化时赋值，而是初始化后给结构体中的元素单独赋值才是保险的。

有些宏是没有在所有 Unix 实现都有定义的，因此可以使用 `#ifdef` 检查并使用。

过去曾规定在使用规范的函数前必须包含 `sys/types.h`，但现代 Unix 实现中一般不需要特地包含，现代 Unix 标准中也删去了这个规定。

### 习题

code: `c3/reboot.c`

根据 `reboot(2)` 可以看到 reboot syscall 需要两个 Magic Number，第一个参数需要 `LINUX_REBOOT_MAGIC1`, 他的值为 `0xfee1dead`，这非常的极客笑话。

第二个 Magic Number 可以取三个值，这三个值转换成 16 进制后是 Linus 和三位千金的生日的样子。

但 `reboot(2)` 的函数好想并不能正常工作的样子，由于现代 Linux 已经不再使用 `libc5` 了。

Glibc 给函数做了封装，不再需要提供两个 Magic Number，因此我的 `reboot` 程序代码 (`c3/reboot.c`) 也使用了新的外壳函数。

但在 Glibc 的源码中 (`glibc/sysdeps/unix/sysv/linux/reboot.c`) 可以看到他确确实实使用了 Magic Number，且进行了系统调用，我想是出于跨平台移植的考虑吧。Glibc 的 `reboot()` 和 Apple macOS 的 `reboot(2)` 文档给出的用法是一致的，而 macOS 的 `reboot(2)` 是来源于 BSD 的。

现代的 Linux 的 `/sbin/reboot` 是指向 `/sbin/systemctl` 的一个符号链接.

根据 BSD 的 `src/sbin/reboot/reboot.c` 重启系统还需要给 `init` 发送信号使其结束, 即真正的重启是需要 init 配合的，只给内核发送信号是不能真正重启的，事实也是这样的。