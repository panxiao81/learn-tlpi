# Cheapear 4 文件 I/O：通用的 I/O 模型

## 词汇速查

文件描述符：file descriptor
文件空洞：file holes

## 笔记

文件 I/O 操作使用文件描述符标记，是一个非负整数。

多数程序期望使用 3 个标准文件描述符：

| 文件描述符 | 用途     | POSIX 名称      | stdio 流 |
| ---------- | -------- | --------------- | -------- |
| 0          | 标准输入 | `STDIN_FILENO`  | stdin    |
| 1          | 标准输出 | `STDOUT_FILENO` | stdout   |
| 2          | 标准错误 | `STDERR_FILENO` | stderr   |

在 Shell 中这三个描述符是打开的，并且指向 Shell 当前运行的终端。程序会继承 Shell 文件描述符的副本，若在运行程序时指定了重定向，则 Shell 会修改文件描述符后再启动程序。

`freopen()` 函数可将 `stdin`, `stdout`，`stderr` 变量指向不同的文件描述符，即无法保证调用后以上三个变量的值仍对应标准值。

指代文件描述符可用数字，但最好用 `unistd.h` 中的 POSIX 标准名称。

由于 UNIX 的一切皆文件特性，可以使用相同的文件 I/O 方式对所有文件进行操作。一旦需要访问文件系统或设备的专有特性时，可使用 `ioctl()` 实现。

### open

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int open(const char *pathname, int flags, /* mode_t mode */);
```

打开的文件由参数 pathname 标识，若 pathname 为符号链接则先解引用。

若调用成功，`open()` 将返回一文件标识符，若发生错误，则返回 `-1` 并将 `errno` 置为对应的错误标志。

`flags` 为位掩码，可选择文件访问方式中的常量。

`flags` 需要在下列常量中选择其一

| 访问模式 | 描述         |
| -------- | ------------ |
| O_RDONLY | 只读方式打开 |
| O_WRONLY | 只写方式打开 |
| O_RDWR   | 读写方式打开 |

除此之外还可用按位或的方式附加其他参数。

`mode` 标识权限，在 `O_CREAT` 存在时需指定，其他时候可忽略，若不指定会将新文件的权限设为栈中的某个随机值。

`mode_t` 可以用八进制数字方式指定，但更为可取的做法是对 0 个或多个表示权限的位掩码进行按位或操作。

根据 SUSv3 的规定，调用 `open()` 成功后返回的文件描述符应为未使用的数值中的最小值。

### read

```c
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t count);
```

`count` 参数对应最多读取的字节数，(size_t 属于 unsigned int)，`buf` 提供用来存放输入数据的内存缓冲区地址，其至少应有 `count` 个字节。

系统调用通常不会分配内存缓冲区，需要预先分配大小合适的内存缓冲区并将缓冲区指针传递给系统调用，而对于库函数则可能分配内存缓冲区。

若调用成功，则返回实际读取的字节数，若遇到 `EOF` 则返回 0，若出现错误则返回 `-1`，`ssize_t` 属于 signed int 类型。

一次读取的字节数可以小于请求的字节数，对于普通文件而言可能是因为读取开始的位置靠近文档尾部。

当 `read()` 用于管道，FIFO，终端或套接字时，也可能小于 `count` 大小。

显然若读取的内容需要输出，若需要表示终止的空字符，则需要显式的追加。

EOF 虽然表示为 0，但实际在文件中并不存在终止标识。

### write

```c
#include <unistd.h>

ssize_t write(int fd, void *buf, size_t count);
```

将数据写入一个已经打开的文件。

若调用成功，则返回实际写入的字节数，该返回值可能小于 count 参数值，可能因为磁盘已满或进程资源对文件大小的限制。

### close

```c
#include <unistd.h>

int close(int fd);
```

当进程结束时，操作系统会将打开的文件描述符回收，但显式关闭仍是编程的好习惯。

文件描述符为有限资源，因此如果文件描述符关闭失败可能导致一个进程的文件描述符资源消耗殆尽。

对 `close()` 也应进行错误检查。

### lseek

```c
#include <unistd.h>

off_t lseek(int fd, off_t offset, int whence);
```

对于打开的文件，内核会记录其文件偏移量，有时也称其为读写偏移量或指针，指下一次运行 `read()` 或 `write()` 时操作的文件起始位置，以相对文件头部起始点的文件当前位置表示，文件第一个字节的偏移量为 0.

文件打开时会将文件偏移量定为文件开始，每次调用 `read()` 或 `write()` 也会同步调整偏移量，以指向已读或已写数据的下一字节。

`offset` 参数指定一个以字节为单位的数值，通常为 signed int 类型，`whence` 表明应按照哪个基点来解释 `offset` 参数。

- SEEK_SET 将文件偏移量设置为从文件头部起始点开始的 `offset` 个字节
- SEEK_CUR 相对于当前的偏移量，将文件偏移量调整 `offset` 个字节
- SEEK_END 将文件偏移量设置为起始于文件尾部的 `offset` 个字节，即 `offset` 参数应从文件最后一个字节的下一个字节算起。

若 `whence` 为 `SEEK_SET` 则 `offset` 应为正值，其他的情况可以使用负值。

调用成功后会返回新的文件偏差量，因此可用下列方式获取当前的偏差量值

```c
curr = lseek(fd, 0, SEEK_CUR);
```

不能将 `lseek()` 用于管道，FIFO，Socket 或终端，但若合情合理可将 `lseek()` 用于设备，例如在硬盘上查找特定的位置。

### 文件空洞

若文件偏移量已经跨越文件末尾，再执行 I/O 操作的情况下，`read()` 会返回 0，但 `write()` 仍可正常写入数据，从文件末尾到新写入的数据之间的这部分空间称为文件空洞。

文件空洞不占用硬盘空间，只有向文件空洞写入数据才会为其分配磁盘块

### ioctl()

```c
#include <sys/ioctl.h>

int ioctl(int fd, int request, ... /* argp */);
```

`request` 参数指定将在 fd 描述符上执行的控制操作，定义在具体设备的头文件中。

argp 可以传入多个参数，可以是任意数据类型，根据 `request` 的参数值来确定。

## 习题

### tee

code: [c4/tee.c](https://github.com/panxiao81/learn-tlpi/blob/master/src/c4/tee.c)

一道简单的综合应用题目。

做下来基本只有几个要点要注意，整体是很简单的。

- 参数中的判别变量用前应初始化
- `write()` 可能有部分写入成功但依然有出错，应检查 `read()` 读进 buf 的内容数量与 `write()` 的写入量是否一致
- optind 为未判断的第一个参数，让其与 `argc` 比较大小，后直接取 `argv[optind]` 可取到参数中的文件名。

如果有多个文件名要从参数读取的话可用如下的 code snippet

```c
if (optind < argc)
{
    do
    {
        char *file = argv[optind];
        // do something with file
    }
    while ( ++optind < argc);
}
```

以上来源于 GNU grep 源码。

### cp_hole

code: [c4/cp_hole.c](https://github.com/panxiao81/learn-tlpi/blob/master/src/c4/cp_hole.c)

可以用 `dd` 创建一个带有文件空洞的文件用于测试

```shell
$ dd if=/dev/urandom bs=4096 count=2 of=fwh
$ dd if=/dev/urandom seek=7 bs=4096 count=2
$ dd if=/dev/zero bs=4096 count=9 of=fwnh
```

难点在于如何检测文件空洞。

对于一个含有文件空洞的文件，`read()` 函数会正常读到一串 0，但如果直接将其写到另一个文件中，则文件空洞不会被保存下来。

所以，基本思想是在写文件前先检查 buf 中的内容有多少是空字符，将空字符 seek 后再写入。
