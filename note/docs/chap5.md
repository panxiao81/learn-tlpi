# Cheaper 5-深入探究文件 I/O

## 词汇速查

原子性：atomicity

竞争状态：race conditions

## 笔记

### 原子性

syscall 是以原子操作方式进行的，内核要保证某个系统调用的所有步骤会作为独立操作而一次性加以执行，期间不会为其他进程或线程中断。

操作共享资源的两个进程或线程的结果取决与一个无法决定的顺序，即进程获得 CPU 使用权的先后相对顺序。

当同时指定 `O_EXCL` 和 `O_CREAT` 标志位时，若打开的文件已存在，则程序会返回一个错误。这使检查文件是否存在和创建文件属于同一原子操作，保证了
当前进程是文件的创建者，因为若在判断文件存在之后再次打开文件，若系统此时判断此进程的时间片已用完，将 CPU 时间分配给了其他进程并创建了该文件，则
两个进程都将认为此文件为自己创建的。

同样的，如果多个进程同时指向一个文件，并向结尾写入数据，此时也许会使用如下代码：

```c
if (lseek(fd, 0, SEEK_END) == -1)
    errExit("lseek");
if (write(fd, buf, len) != len)
    fatal("Partial/failed Write");
```

相似的，若第一个进程执行完 `lseek()` 后消耗完了时间片，被执行类似代码的另一个进程中断，则两个进程会在写入数据前将偏移量设置为相同位置，则可以
看出，此时当原进程再次被调度时，会覆盖第二个进程已经写入的数据，此时又一次出现了竞争状态。要规避这个问题，应在打开文件时加入 `O_APPEND` 标志位
，但对不支持 `O_APPEND` 的文件系统，内核会忽略 `O_APPEND`，按照传统的方式移动偏移量，因此仍可能导致脏写入问题。

### 文件控制 fcntl()

```c
#include <fcntl.h>

int fcntl(int fd, int cmd ...);
```

cmd 支持的操作很多，第三个参数可以设置为不同类型或省略，内核会根据 `cmd` 的值来确定后续参数的数据类型。

用途之一是对一个打开的文件获取或修改访问模式和状态标志。

要获取设置，将 `cmd` 设为 `F_GETFL`

```c
int flags, accessMode;

flags = fcntl(fd, F_GETFL);
if (flags == -1)
    errExit("fnctl");

if(flags & O_SYNC)
    printf("writes are synchronized\n");
accessMode = flags & O_ACCMODE;
if (accessMode == O_WRONLY || accessMode == O_RDWR)
    printf("file is writable\n");
```

判断文件的访问模式需要让 `flags` 与 `O_ACCMODE` 相与，后与常量比对，其余直接相与判断即可。

使用 `F_SETFL` 修改已打开文件的某些状态标志，允许修改的标志有

- O_APPEND
- O_NONBLOCK
- O_NOATIME
- O_ASYNC
- O_DIRECT

其他标志位不允许修改，将被系统忽略。

当文件不是被调用的程序打开时，或文件描述符的获取是由 `open()` 以外的方式获取的时候，这显得格外有用。

先使用 `F_GETFL` 获取文件当前的状态，变更需要修改的比特位后再调用 `fcntl()` 的 `F_SETFL` 更新状态。

例如，要增加 `O_APPEND` 使用如下代码

```c
int flags;

flags = fcntl(fd, F_GETFL);
if (flags == -1)
    errExit("fcntl");
flags |= O_APPEND;
if (fcntl(fd, F_SETFL, flags) == -1)
    errExit("fcntl");
```

### 文件描述符和打开文件的关系

文件描述符和打开的文件并非一一对应的关系，多个文件描述符可能指向同一文件，这些文件描述符可能在相同或不同的进程中打开。

内核维护三个数据结构

- 进程级的文件描述符表
- 系统级的打开文件表
- 文件系统的 i-node 表

对于每个进程，内核为其维护打开的文件描述符 (open file descriptor) 表，表的每一项都记录了单个文件描述符的相关信息。

- 控制文件描述符的一组标志
- 对打开文件句柄的引用

对打开的文件，系统维护一个系统级的描述表格 (open file description table)，将表中的条目称为打开文件句柄 (open file handle)

一个打开文件句柄存储了一个与打开文件相关的所有信息

- 当前文件的偏移量
- 打开文件时所使用的状态标志
- 文件访问模式
- 与信号驱动 I/O 相关的配置
- 对该文件 i-node 对象的引用

文件系统会为其上的文件维护 i-node 表，信息一般包括

- 文件类型
- 一个指向该文件所持有的锁的列表的指针
- 包括文件大小以及不同类型操作相关的时间戳的各种文件属性

两个不同的文件描述符，若指向同一个打开文件句柄，会共享同一个文件偏移量，而文件描述符标志为进程和文件描述符私有。

何时需要复制文件描述符？考虑 Shell 的重定向语法 `2>&1`，将标准错误重定向至标准输出，因此考虑下列命令，将标准输出和标准错误写入文件中

```shell
./myscript > result.log 2>&1
```

注意 Shell 从左至右处理重定向语句。

Shell 在此处将文件描述符 1 复制到文件描述符 2 实现了文件描述符的重定向，此时文件描述符 2 和 1 指向同一个文件句柄。

在这种场景下，不能简单的打开两次文件，因两次打开生成两个不同的文件句柄，不会共享同一文件偏移量指针，导致可能互相覆盖对方的输出，其次打开的文件
不一定是磁盘文件，也可能是管道或其他的文件。

```c
#include <unistd.h>

int dup(int oldfd);
```

`dup()` 调用一个已经打开的文件描述符 oldfd，并返回一个新的描述符，二者指向同一个打开的文件句柄。系统保证新编号一定是编号最低的未用文件描述符。

若假设当前已经打开 `0`，`1`，`2` 三个文件描述符，此时调用 `dup()` 会返回文件描述符 `3`。

若希望返回 `2`，则可先关闭 `2`，再复制文件描述符。也可使用 `dup2()` 简化操作

```c
#include <unistd.h>

int dup2(int oldfd, int newfd);
```

`dup2()` 复制 `oldfd` 参数指定的文件描述符，编号为 `newfd`。若 `newfd` 所指定的文件描述符已经打开，则 `dup2()` 会先关闭该文件描述符。

`dup2()` 会忽略 `newfd` 关闭期间发生的错误，最好提前显式关闭 `newfd`。

`oldfd` 若不存在，则 `dup2()` 失败并返回错误 `EBADF`，并且不关闭 `newfd`，若 `oldfd` 有效且与 `newfd` 相等，则 `dup2()` 什么都不做，
且不关闭 `newfd`，并将其作为结果返回。

也可使用 `fcntl()` 的 `F_DUPFD` 复制文件描述符

```c
newfd = fcntl(oldfd, F_DUPFD, startfd);
```

将复制 `oldfd`，并将大于等于 `startfd` 的最小未用的值作为描述符编号。此调用可保证新的描述符落在特定的区域范围内。

新的文件描述符有其自己的文件描述符标志，可使用 `dup3()` 在复制同时控制新文件描述符的文件描述符标志

```c
#define _GNU_SOURCE
#include <unistd.h>

int dup3(int oldfd, int newfd, int flags);
```

`dup3()` 为 Linux 特有

### 在文件特定偏移量的 I/O

系统调用 `pread()` 和 `pwrite()` 完成与 `read()` 和 `write()` 相同的工作，只是可以指定文件偏移量，而非在当前偏移量处做 I/O 操作，且不
改变当前偏移量。

```c
#include <unistd.h>

ssize_t pread(int fd, void *buf, size_t count, off_t offset);

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
```

好处是，一方面，能够保证移动偏移量和读写操作是原子性的，另外比起先移动偏移量再写入要省去一部分系统调用带来的可忽略的性能提升。(但显然通常 I/O 
的性能瓶颈都在设备 I/O 而非系统调用，I/O 的开销远大于系统调用)。

对 `pread()` 和 `pwrite()` 而言，fd 指代的文件必须可定位，即允许对文件描述符执行 `lseek()` 操作。

尤其在多线程编程时，此种方法为保证系统调用的原子性显然非常有帮助。

### 分散输入和集中输出

```c
#include <sys/uio.h>

ssize_t readv(int fd, const struct iovec *iov, int iovcnt);

ssize_t writev(int fd, const struct iovec *iov, int iovcnt);
```

这些函数可一次传输多个缓冲区的数据。

`iov` 定义了一组用来传输数据的缓冲区，整形数指定了 `iov` 的成员个数，`iov` 中的每个成员的数据结构如下

```c
struct iovec {
    void *iov_base;
    size_t iov_len;
};
```

`readv()` 实现了分散输入，从 `fd` 读一串连续的字节，然后将其散置于 `iov` 指定的缓冲区中。

`readv()` 的特点依然是保证原子性。

调用 `readv()` 返回成功读区的字节数，若文件结束返回 `0`，调用者需对其返回值进行检查，若数据不足填满缓冲区，则只会占用部分缓冲区，最后一个缓冲
区可能只有部分数据。

`writev()` 实现了集中输出。将 `iov` 中的内容拼接起来，以连续的字节序列写入文件描述符 `fd` 指代的文件中。

显然 `writev()` 的操作是原子性的。

`writev()` 同样可能存在部分写的问题。

现代 Linux 提供了 `preadv()` 和 `pwritev()` 可在特定偏移量处进行分散输入/集中输出的 I/O，虽然并非标准，但 BSD 支持该调用。

```c
#define _BSD_SOURCE
#include <sys/uio.h>

ssize_t preadv(int fd, const struct iovec *iov, int iovcnt, off_t offset);

ssize_t pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
```

### 截断文件

```c
#include <unistd.h>

int truncate(const char *pathname, off_t length);

int ftruncate(int fd, off_t length);
```

若当前文件长度大于 `length`，则丢弃超出的部分，若小于 `length`，则在文件结尾添加空字节或文件空洞。

### 非阻塞 I/O

若打开文件时指定 `O_NUNBLOCK`，则

- 若 `open()` 未立即打开文件，则返回错误，而非陷入阻塞
- 调用 `open()` 成功后的 I/O 操作也是非阻塞的，若 I/O 操作未立即完成，则可能只写入部分数据，并返回 `EAGAIN` 或 `EWOULDBLOCK` 错误。

管道，FIFO，套接字，设备都支持非阻塞模式。

由于内核缓冲区保证普通文件 I/O 不会阻塞，因此打开普通文件通常忽略 `O_NONBLOCK` 标志，但若使用强制文件锁时，该标志对普通文件也可用。

### 大文件 I/O

通常 `off_t` 为 signed long 类型，但这样将文件大小限制在了 $2^31-1$ 即 2GB 大小。

可使用支持大文件操作的备选 API 或将 `_FILE_OFFSET_BITS` 宏的值设为 64。

若使用备选的过渡 API，则可使用原文件 I/O 函数的 64 位版本。即 `open64()`，`lseek64()` 等。

更好的做法是在编译时将 `_FILE_OFFSET_BITS` 设为 64，此时所有 32 位函数和数据类型都将转换为 64 位版本。

在 64 位系统上，`open64()` 和 `open()` 为同一个实现，且对应的函数已经为 64 位版本。

对于 `printf()`,应将 `off_t` 强制转换为 `long long`，后使用 `%lld` 修饰符。

### /dev/fd 目录

对于每个进程，系统提供一个虚拟的目录 `/dev/fd`，该目录包含 `/dev/fd/n` 形式的文件名，其中 `n` 是打开的文件描述符相对应的编号。

早期版本的 Linux 将该文件链接至 `/proc/self/fd` 中，现代 Linux 直接将其链接到文件本身。

系统还提供了 3 个符号链接 `/dev/stdin`，`/dev/stdout` 和 `/dev/stderr`。早期链接到 `/dev/fd`，现在链接到 `/proc/self/fd/`

### 创建临时文件

```c
#include <stdlib.h>

int mkstemp(char *template);
```

`mkstemp()` 生成一个唯一的文件名并打开该文件，返回一个文件描述符。

参数 `template` 采用路径名形式，其中最后 6 个字符必须是 `XXXXXX`,这 6 个字符会被替换，且修改后的文件名会通过 `template` 传回，因此 
`template` 必须是一个字符数组，而不能是一个字符常量。

文件拥有者对 `mkstemp()` 建立的文件拥有读写权限，其他用户没有任何权限，且打开文件使用 `O_EXCL` 参数，保证调用者独占访问文件。

通常，在创建临时文件后，程序就应使用 `unlink()` 将其删除。

```c
#include <stdio.h>

FILE *tmpfile(void);
```

`tmpfile()` 创建一个名称唯一的临时文件，并以读写方式打开，成功将返回一个文件流供 `stdio` 使用，文件流关闭后将自动删除临时文件，为达到该目的
，`tmpfile()` 函数将在打开文件后立即调用 `unlink()` 来删除文件。

## 习题

### 5-1

code: c5/large_file.c

就是改代码，没什么好说的。

### 5-2

code: c5/append_exists.c

在使用 `O_APPEND` 方式打开文件时，尽管设置偏移量为 `0`，仍然会向文件中追加写入。

### 5-3

code: c5/atomic_append.c

运行结果如下：

```shell
$ ls -l
总用量 3164
-rwxrwxr-x 1 panxiao81 panxiao81   31072  5月 27 19:12 append_exists
-rw-rw-r-- 1 panxiao81 panxiao81    1492  5月 27 18:58 append_exists.c
-rwxrwxr-x 1 panxiao81 panxiao81   31992  5月 27 19:41 atomic_append
-rw-rw-r-- 1 panxiao81 panxiao81    1843  5月 27 19:41 atomic_append.c
-rw------- 1 panxiao81 panxiao81 2000000  5月 27 19:42 f1
-rw------- 1 panxiao81 panxiao81 1121998  5月 27 19:42 f2
-rwxrwxr-x 1 panxiao81 panxiao81   29552  5月 27 19:13 large_file
-rw-rw-r-- 1 panxiao81 panxiao81    1375  5月 25 02:15 large_file.c
-rw-rw-r-- 1 panxiao81 panxiao81     298  5月 27 18:46 Makefile
```

可以看到 f1 明显比 f2 大的多。

由于不能保证无 `O_APPEND` 时操作的原子性，因此在两个进程同时写入时，对于 f2 的情况写入的数据互相覆盖，导致了出现此情况。

### 5-4

code: c5/dup.c

对于 `dup()` 可以使用 `fnctl(oldfd, F_DUPFD, 0)`，`dup2()` 可以改写为 `fcntl(oldfd, F_DUPFD, newfd)`，剩下的只需要做好错误处理就可以了。

注意，根据 dup(2) 所说:

> The  steps  of  closing and reusing the file descriptor newfd are performed atomically.

也就是说，使用系统调用 `dup2()` 可以保证关闭 `newfd` 和再次打开 `newfd` 是有原子性的，这点在我们自己实现的版本中是无法做到的。

源码中的 `main()` 只是为了通过编译而已，我没有写测试，因为我觉得他大概是能正常工作的。

### 5-5

code: c5/check_descriptor.c

由于题目说明非常简略，代码也写的非常简略。

由于只是要查看是否共享，我只进行了简单的判断是否相等。

### 5-6

可见，fd2 由 fd1 复制而来，共享同一偏移量，fd3 是另一份打开的文件，单独维护一份偏移量

第一次：`Hello,`

第二次：`Hello,world` 由于共享同一偏移量，且文件偏移量随 `write()` 后移

第三次：由于 `lseek()` 将偏移量移动回文件开头，再次写入覆盖了过去的文字，现在内容为 `HELLO,world`

第四次：fd3 的文件偏移量一直在开头，因此覆盖了原有文字，内容为 `Gidday world`

### 5-7

code: c5/readv_writev.c

为保证原子性，对于读入的操作，需要得知总的缓冲区大小，分配一段临时的 buffer 将信息一次性读入，再分开写入给定的缓冲区。对写入操作也同理，需要先将给定的缓冲区写入到临时分配的大缓冲区，之后一次性对整个大缓冲区进行写入操作。

对于内存内容的拷贝操作，很容易想到可以使用 `memcpy()` 来实现。大缓冲区的操作也只需要根据 `iov_len` 计算偏移量即可。

这里需要注意的是，对于读入缓冲区，需要判断读入的字节数中尚未写入的大小与当前块的缓冲区大小，若尚未写入的数据大小小于当前缓冲区的大小，则可判断写完当前剩余的数据即可结束并返回。

`readv()` 不会保证当读到的数据未占满缓冲区时剩余的缓冲区内的数据，因此这种实现是完全合理的。

这段代码我同样没有经过完整测试，`main()` 函数存在仍同样只是为了能通过编译。
