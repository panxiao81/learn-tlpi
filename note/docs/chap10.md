# Cheaper 10 时间

## 词汇速查

- 日历时间：calendar time
- 流逝时间：elapsed time
- 挂钟时间：wall clock
- 国际化：internationalization 

## 笔记

程序会关注两种时间类型：

- 真实时间：该时间又分两个起点，一是从某个标准点计算，二是从进程生命周期内的某个节点。前者即为日历时间，后者称为流逝时间或挂钟时间
- 进程时间：一个进程所使用的 CPU 时间总量

多数计算机中都具有硬件时钟，内核根据内核时钟计算真实时间和进程时间。

### 日历时间

UNIX 系统内部表示时间的方法是以 Epoch 以来的秒数来计算的，即 UTC 时间的 1970 年 1 月 1 日凌晨 0 点。

日历时间使用 `time_t` 类型存储。

对于 32 位系统，`time_t` 是一个 signed int 类型的数，最大的表示范围到 2038 年 1 月 19 号 03:14:07，即 2038 年问题。

```c
#inlcude <sys/time.h>

int gettimeofday(struct timeval *tv, struct timezone *tz);
```

以上 syscall 在 tv 指向的结构体中返回日历时间。

`tv` 是指向如下结构体的一个指针

```c
struct timeval {
    time_t        tv_sec;    /* Seconds since 00:00:00, 1 Jan. 1970 UTC */
    suseconds_t   tv_usec;   /* Additional microseconds (long int) */
}
```

tv_usec 字段可提供微秒级的精度，但精确度取决于平台实现。在现代 x86 平台上，它的确可以提供微秒级精度。

`tz` 参数是历史产物，早期 UNIX 用来获取系统的时区信息，目前已废弃，应设置为 NULL。

`tz` 在 Linux 内核中功能从未恰当的被实现过，保留此参数仅为兼容性考虑。 (详见 gettimeofday(2))

```c
#include <time.h>

time_t time(time_t *timep);
```

该 syscall 返回自 Epoch 以来的秒数，即和 timeval 结构体中 `tv_sec` 的数值相同。

若 timep 参数不为 NULL，还会将该值置于 timep 指向的位置。

该函数只可能因为 timep 参数为一个无效地址时报错，因此往往会使用如下用法且不做错误检查：

```c
t = time(NULL);
```

### 时间转换函数

为将 time_t 转换为可打印的格式，存在标准库函数：

```c
#include <time.h>

char *ctime(const time_t *timep);
```

将一个指向 time_t 类型的指针作为参数传入，返回一个 26 字符的字符串，即为标准格式的日期与时间。该函数转换时会考虑本地时区和夏令时设置，并且返回的字符串指针指向的区域为静态分配的，下一次调用会被覆盖。

SUSv3 规定 `ctime()` 、`gmtime()`、`localtime()` 和 `asctime()` 都可能覆盖其他函数返回且静态分配的空间，换言之即这些函数可以共享同一个静态分配的地址空间。

`gmtime()` 和 `localtime()` 可将 `time_t` 值转换为分解时间，即返回一个指向静态分配的结构体的指针，将分解的时间置于此结构体并返回。

```c
#include <time.h>

struct tm *gmtime(const time_t *timep);
struct tm *localtime(const time_t *timep);
```

`gmtime()` 将日历时间转换为对应 UTC 的分解时间，`localtime()` 则额外考虑时区和夏令时，返回对应本地时间的分解时间。

返回的结构为静态分配。

> `gmtime()` 中的 gm 取自 GMT 即格林威治标准时间

返回的结构体如下

```c
struct tm {
    int tm_sec;      /* Seconds (1-60) */
    int tm_min;      /* Minutes (0-59) */
    int tm_hour;     /* Hours (0-23) */
    int tm_mday;     /* Day of the month (1-31) */
    int tm_mon;      /* Month (1-11) */
    int tm_year;     /* Year since 1900 */
    int tm_wday;     /* Day of the week (Sunday = 0) */
    int tm_yday;     /* Day in the year (0-365; 1 Jan = 0)
    intg tm_isdst;   /* Daylight saving time flag
                          > 0: DST is in effect;
                          = 0: DST is not effect;
                          < 0; DST information not available */
}
```

`tm_sec` 最大设为 60 为了考虑闰秒。

```c
#include <time.h>

time_t mktime(struct tm *timeptr);
```

`mktime()` 将本地的分解时间翻译成 `time_t` 值，将其作为结果返回。

该函数可能会修改结构体，使其能够保证字段合法，且调整会发生在 `mktime()` 返回 `time_t` 的值之前。

该函数还可用于分解计算时间，即溢出的字段会被正常计算，甚至可将字段置为负值。

若 `tm_isdst` 字段为负，则函数会试图判断当前是否执行夏令时，且在调用完成后将字段置为适当值。

#### 分解时间和打印格式间的转换

```c
#include <time.h>

char *asctime(const struct tm *timeptr);
```

参数提供一个指向 tm 结构的指针，该函数返回一个指针，指向一个静态分配的字符串，格式与 `ctime()` 相同。

该函数对 tm 原封不动转换，不受时区影响。

```c
#include <time.h>

size_t strftime(char *outstr, size_t maxsize, const char *format, const struct tm *timeptr);
```

`strftime()` 可以在转换打印格式时作更精细的控制，该函数会将字符串置于 `outstr` 指向的缓冲区中。

`maxsize` 指定了缓冲区的最大长度，且输出的字符串不含有换行符。

若成功，函数返回 `outstr` 指定的字节长度，不包含终止空字节，若字符串长度超过 maxsize 长度，则返回 0 表示出错，且此时 outstr 的内容不确定。

`format` 为一字符串，使用类似 printf 的方式进行格式化，该转换说明符可在 strftime(3) manpage 中查看。

```c
#define _XOPEN_SOURCE
#include <time.h>

char *strptime(const char *str, const char *format, struct tm *timeptr);
```

该函数是 `strftime()` 的逆向，将字符串转换为分解时间。

若成功，该函数返回一指针，指向 str 中下一个未经处理的字符。若无法匹配字符串，则返回 `NULL` 表示错误。

格式规范类似于 `scanf`， 转换说明符可参阅 strptime(3) manpage 手册。

### 时区

时区信息由系统提供，统一维护，通常保存在 `/usr/share/zoneinfo` 中。本地时间由 `/etc/localtime` 定义，通常是到 `/usr/share/zoneinfo` 下一个文件的符号链接。

对运行中的程序指定时区，通过 TZ 环境变量来完成。

获取当前时间时，函数会调用 `tzset(3)`，对下列全局变量进行初始化。

```c
char *tzname[2];    /* Name of timezone and alternate (DST) timezone */
int daylight;       /* Nonzero if there is an alternate (DST) timezone */
long timezone;      /* Seconds difference between UTC and local standard time */
```

若 TZ 环境变量未设置，则采用 `/etc/localtime` 中的默认时区，若无法匹配文件，则采用 UTC 时间。

### 地区

世界各国在对时间的表示法等信息中习俗不同，理想状态中程序应处理这些本地化问题，这即为 internationalization，即 `i18n`

地区信息也统一维护，通常保存在 `/usr/share/local`，有的系统保存在 `/usr/lib/local`，

可使用 `setlocale()` 设置和查询当前地区。

本书不重点讨论 i18n。

### 更新系统时钟

```c
#define _BSD_SOURCE
#include <sys/time.h>

int settimeofday(const struct timeval *tv, const struct timezone *tz);
```

`settimeofday()` 是 `gettimeofday()` 的逆向。

与 `gettimeofday()` 相同的，tz 参数应指定为 NULL。

`settimeofday()` 会导致系统时间突然变化，这可能会影响一些应用，若对时间进行微小调整如调整几秒钟误差，通常使用库函数 `adjtime()`，该函数将系统时间逐步调整到正确时间。

```c
#define _BSD_SORUCE
#include <sys/time.h>

int adjtime(struct timeval *delta, struct timeval *olddelta);
```

`delta` 指向一个结构，指定需要调整的秒和微秒数。

在函数运行时间内可能无法完成时钟调整，在这种情况下剩余的时间会存放在 olddelta 指向的结构体中。如果不关心这个值可以将其置为 NULL。类似的，若不打算矫正时间，可以指定 delta 参数为 NULL。

Linux 提供了更复杂，更通用的 adjtimex() 系统调用，adjtime() 在 Linux 上基于 adjtimex() 实现。

### 软件时钟 jiffies

时间精度在操作系统中受限于系统软件时钟的分辨率，单位为 jiffies，其大小是定义在内核中的常量 HZ。该值可由内核参数调整。

### 进程时间

内核把 CPU 时间分成两部分。

- 用户 CPU 时间，指用户模式下执行花费的时间数量。也称为虚拟时间。对于程序本身即为其得到 CPU 的时间。
- 系统 CPU 时间指在内核模式中执行花费的时间数量，这是内核用于执行系统调用或代表程序执行其他任务的时间

有时进程时间又指处理过程中消耗的总 CPU 时间。

可以使用 time(1) 程序统计应用执行所花费的时间。

```c
#include <sys/times.h>

clock_t times(struct tms *buf);
```

该系统调用检索进程时间信息，并将结果返回给 `buf` 指向的结构体。

该结构体的结构如下：

```c
struct tms {
    clock_t tms_utime;    /* User CPU time used by caller */
    clock_t tms_stime;    /* System CPU time used by caller */
    clock_t tms_cutime;   /* User CPU time of all (waited for) children */
    clock_t tms_cstime;   /* System CPU time of all (waited for) children */
}
```

前两个字段返回调用进程到目前为止使用的用户和系统组件的 CPU 时间，最后两个字段返回父进程执行系统调用 wait() 的所有已经终止的子进程使用的 CPU 时间。

`clock_t` 是 clock tick 为单位的整型值，可以调用 `sysconf(_SC_CLK_TCK)` 获得每秒包含的 tick 数，再用这个数字除以 `clock_t` 转换为秒。

`times()` 返回的是自过去的任意时间点流逝的以 clock tick 为单位的时间，SUSv3 标准特别未定义这个时间点，因此这个返回值唯一的用法是计算一对 times() 调用返回值的差，然而若 clock_t 溢出，则 times() 会重新从 0 开始计算，因此其结果仍是不可靠的。

```c
#include <time.h>

clock_t clock(void)
```

函数 clock() 提供了一个取得进程时间的简单接口，返回调用进程使用的总 CPU 时间。

其计量单位为 `CLOCKS_PER_SEC`，因此必须先除以这个值来获得进程使用的 CPU 时间秒数。

## 练习

### 10-1

对于 times() 的 clock_t，其进入下一个循环的时间为 $2^{32} \div 100$ 即 42949672.96 秒。约等于 497 天，1.36 年。

对于 clock() 的 返回值，进入下一个循环的时间为 $ 2^{32} \div 10000$，即 429496.7296 秒，约为 4.97 天。
