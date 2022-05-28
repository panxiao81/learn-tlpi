#include <stdio.h>
#include <setjmp.h>

jmp_buf env;

void setJmp2()
{
    printf("Enter setJmp2!\n");
    setjmp(env);
    printf("Exiting setJmp2!\n");
}

void setJmp()
{
    printf("Enter setJmp!\n");
    setJmp2();
    printf("Exiting setJmp!\n");
}

void doJmp()
{
    printf("Enter doJmp!\n");
    longjmp(env, 1);
    printf("Exiting doJmp!\n");
}

int main(void)
{
    printf("Entering main!\n");
    setJmp();
    doJmp();
    printf("Exit Main!\n");
    return 0;
}