#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

int main(void)
{
    while(1) {
		// 打开字符设备 => 调用对应的led_open()函数
        int fd = open("led",O_RDWR);
		sleep(1);
		if(-1 == fd) {
                	perror("Open LED");
                	return -1;
        }
        // 关闭字符设备 => 调用led_off()函数
		close(fd);
		sleep(1);
    }
    return 0;
}

