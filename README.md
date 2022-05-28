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

<a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-sa/4.0/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/">Creative Commons Attribution-ShareAlike 4.0 International License</a>.