The environment what I use is:

- Ubuntu 22.04 LTS (GCC 11.2 with GLIBC 2.35)
- macOS 12.3.1 (Apple clang 12.0.5)
- CentOS 6.10 (RedHat GCC 4.4.7 with GLIBC 2.12)

The source code which come from upstream is not being included, for the author is not provided a Git repository, and the code is still being actively update.
you should download it from the [author's website](https://man7.org/tlpi/code/index.html) by yourself.

```shell
wget https://man7.org/tlpi/code/download/tlpi-220505-dist.tar.gz
tar -xf tlpi-220505-dist.tar.gz && rm -rf tlpi-220505-dist.tar.gz
```

You need to unarchive it in UNIX-Like system since there's some symbolic link, and I also use symbolic link to link the lib folder to my source code.
and also, you should compile and run these code in Linux, for these code is the LINUX PROGRAMMING, of course.

Download the source code from author's website. According to the README file, we need to install some header file package.

For Debian and Ubuntu, run:

```sh
sudo apt install build-essential libcap-dev libacl1-dev libselinux1-dev libseccomp-dev gcc-multilib
```

For CentOS, run:

```sh
sudo yum install libcap-devel libacl-devel libselinux-devel libseccomp-devel "@Development tools"
```

Then try to compile the source code which from the author.

```sh
cd tlpi-dist
make all
```

If success, you are ready to go.

The source code what I write is placed at `/src`, and all the source file appeared in the note will use `/src` as the root directory.
