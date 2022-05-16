# Learn TLPI (The Linux Programming Interface)

This is the source code and the note of my solution for TLPI (The Linux Programming Interface)

Still work in progress.

Since the book which I am reading is mainly focused on the Chinese version, My note is also mainly written in Chinese.

The original example source code and header file could be downloaded at [auther's website](https://man7.org/tlpi/code/index.html), and I also provide a scripts for this job.

## Build

```shell
# Install the dependency first.
$ make all

# Or build my own code only
$ make build
```

Since some code needs the library which comes from upstream, the makefile is automatically running the scripts to download the newest version of source code from the website.