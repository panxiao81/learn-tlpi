# Cheaper 7 内存分配

## 词汇速查

## 笔记

堆是一段长度可变的虚拟内存。

在 program break 的位置抬升后，程序就可以访问新分配的区域内的任何内存。此时物理内存页会在程序首次试图访问新分配的内存时又内核分配新的物理内存页。

UNIX 中使用 `brk()` 和 `sbrk()` 系统调用来操作 program break。

```c
#include <unistd.h>

int brk(void *end_data_segment);

void *sbrk(intptr_t increment);
```

syscall `brk()` 会将 program break 设定到 `end_data_segment` 指定的位置。且实际会四舍五入到下一个内存页的边界。

若将 program break 设定为低于初始值的大小，可能会导致无法预知的行为。

`sbrk()` 将 program break 在原有地址基础上增加 `increment` 参数个大小。`intptr_t` 是一个整数数据类型，若调用成功则返回前一个 program break 的地址，即若 program break 增加，返回值是指向新分配内存起始位置的指针。

`sbrk(0)` 调用将返回 program break 的当前位置。

通常我们使用 `malloc()` 函数包来在堆上分配和释放内存，比起 syscall 有很多优势。

```c
#include <stdlib.h>

void *malloc(size_t stze);
```

`malloc()` 分配 `size` 大小的内存，且分配的内存区域未初始化。返回指向新分配内存起始位置的指针。若分配失败则返回 `NULL`。

由于返回值为 `void*` 所以可以将返回的指针转换赋给任何类型的指针。返回的内存块是字节对齐的。

调用 `malloc(0)` 要么返回 `NULL` 要么返回一小块可以用 `free()` 释放的内存，Linux 遵循后者。

```c
#include <stdlib.h>

void free(void *ptr)
```

`free()` 释放 `ptr` 指向的内存块，该参数应是由 `malloc()` 分配的，或其他间接使用其在堆上分配的内存的地址的指针。

一般，`free()` 并不会降低 program break 的位置，而是将内存添加到空闲内存列表中，可供后续的 `malloc()` 使用。优势在于

- 被释放的内存块可能位于堆的中间，而非顶部，此时不能降低 program break
- 减少了 syscall 次数
- 多数时候立即降低 program break 并不会带来很多收益

`free()` 可以接受一个空指针，函数不会产生错误。

对指向已经释放的内存的指针再次调用，如再次传递给 `free()` 可能产生不可预知的后果。

当进程终止时，使用的所有内存都会被回收，包括在堆中使用 `malloc()` 分配的内存。对于在程序运行时就分配并且在程序结束后才释放的程序来说可以省略 `free()`。但最好还是显式指定释放内存。尤其对于 `malloc()` 调试库而言可能将未显式释放的内存认为是内存泄漏。

#### malloc() 和 free() 的实现原理

`malloc()` 会先扫描由 `free()` 释放的空闲内存块列表，若找到尺寸大于或等于要求的一块空闲的内存，若完全相等，则直接返回给调用者，若较大，则会先对内存进行分割，将大小相当的内存返回，剩余的部分保留。

若找不到足够大的内存，则 `malloc()` 调用 `sbrk()` 分配一段内存，为了减少 syscall 调用，`malloc()` 会一次性分配更多内存。

在 `malloc()` 分配内存时会额外分配几个字节用来存放这段内存大小的整数值，该区域位于分配的地址头部，实际返回的指针则指向这个字节之后。

空闲内存列表是一个双向链表的数据结构，当内存块处在空闲内存列表时，`free()` 使用内存块本身的空间来存放链表指针，将自身添加到链表中。

当内存不断的释放和重新分配，空闲列表中的空闲内存和已经分配的内存会混合在一起。由于 C 可以使指针指向任意位置，若程序无意间修改或覆盖了内存分配的长度值，则 `free()` 也会在内存列表中按照错误的方式记录，这造成了排查困难的错误。

### 使用 calloc() 和 realloc()

```c
#include <stdlib.h>

void *calloc(size_t numitems, size_t size)
```

`calloc()` 用于给一组相同对象分配内存。参数 `numitems` 指定分配数量，`size` 指定对象大小。并返回指向该内存块的指针。若出错则返回 `NULL`。`calloc()` 会将分配的内存初始化为 0。

```c
#include <stdlib.h>

void *realloc(void *ptr, size_t size)
```

`realloc()` 用来调整一块内存的大小，此内存应是由 `malloc()` 函数包分配的。 `ptr` 为指向需要调整大小的内存块的指针，`size` 为要调整到的大小值。

若成功，则返回指向大小调整后的内存块的指针，该指针指向的内存可能与之前的不同。若发生错误则返回 `NULL`，此时对 `ptr` 指向的内存块原封不动。若增加了内存，对增加的部分不会进行初始化。

尽可能不使用 `realloc()` 调整内存，由于最常见的情况是无法就原有的内存块扩展，此时需要分配新的内存块并将原数据复制到新内存，这种行为会带来大量 CPU 损耗。

且对后续内存的引用应使用 `realloc()` 的返回内存。并由于 `realloc()` 可能失败，且失败时会返回 `NULL`，在调用 `realloc()` 时不应直接将原指针赋予新的值，这可能会当 `realloc()` 执行失败时无法再访问原有的内存块。应使用如下的 code snippets

```c
nptr = realloc(ptr, newsize);
if (nptr == NULL)
{
    /* Handle Error */;
} else {  /* realloc() succeeded */
    ptr = nptr;
}
```

### 在栈上分配内存

```c
#include <alloca.h>

void *alloca(size_t size);
```

可以使用 `alloca()` 在栈上动态分配内存。`size` 指定分配的字节数。并返回指向分配的内存块的指针。

不需也绝不能调用 `free()` 释放 `alloca()` 分配的内存。也不可能调用 `realloc()` 修改 `alloca()` 分配的内存大小。

若 `alloca()` 调用后发生堆栈溢出，则程序行为不可预知，此时一般程序会收到 `SIGSEGV` 信号。

不能在函数的参数列表里调用 `alloca()`，而应在调用函数前分配内存并传递指针。否则分配的堆栈空间会出现在函数参数的空间内。

```c
func(x, alloca(size), z); /* Wrong! */

/* Right */
void *y;
y = alloca(size);
func(x, y, z);
```

在栈上分配内存比起在堆上的内存有一定优势

- 速度快于 `malloc()` 函数包，且不需要维护空闲内存块列表
- 内存会随栈帧的移除而自动释放

## 练习

### 7-1

code: c7/free_and_sbrk.c

```shell
./free_and_sbrk 10 102400

Initial program break:          0x55e470df6000
Allocating 10*102400 bytes
Times 0: Program break is now:           0x55e470df6000
Times 1: Program break is now:           0x55e470e28000
Times 2: Program break is now:           0x55e470e28000
Times 3: Program break is now:           0x55e470e5a000
Times 4: Program break is now:           0x55e470e5a000
Times 5: Program break is now:           0x55e470e8c000
Times 6: Program break is now:           0x55e470e8c000
Times 7: Program break is now:           0x55e470ebe000
Times 8: Program break is now:           0x55e470ebe000
Times 9: Program break is now:           0x55e470ef0000
Program break is now:           0x55e470ef0000
Freeing blocks from 1 to 10 in steps of 1
After free(), program break is: 0x55e470df6000
```

### 7-2

这个就饶了我吧，CS：APP 的时候会做的，一定会做的。