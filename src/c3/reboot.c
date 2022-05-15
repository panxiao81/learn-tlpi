/* Compile with _XOPEN_SOURCE >= 500
   (c) Pan Xiao
 */
#include <unistd.h>
#include <errno.h>
#include <sys/reboot.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    int err;
    printf("Syncing Disk...");
    /*
     *  Syncing Disks, otherwise the data which has not be written into the disk will be lost.
     *  reboot(2)
     */
    sync();

    /* for glibc, use "int reboot(int __howto)" */
    err = reboot(RB_AUTOBOOT);
    /* or if you want a direct syscall, use syscall function
    err = syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART);
    Newer version of reboot(2)
    https://man7.org/linux/man-pages/man2/reboot.2.html
    */
    if (err == -1)
    {
        perror("reboot");
    }
    return 0;
}
