#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

#include <os.h>
#define GAP (1 << 20)

static char buff[64];

int main(void)
{
    int i, j;
    int fd = sys_fopen("2.txt", O_RDWR);

    // write 'hello world!' * 10
    for (i = 0; i < 9; i++)
    {  
        sys_fwrite(fd, "hello world!\n", 13);
        sys_lseek(fd, GAP, 1);
    }

    // read
    sys_move_cursor(1,1);
    sys_lseek(fd, 0, 0);
    for (i = 0; i < 9; i++)
    {
        sys_fread(fd, buff, 13);
        for (j = 0; j < 13; j++)
        {
            printf("%c", buff[j]);
        }
        sys_lseek(fd, GAP, 1);
    }

    sys_close(fd);
}