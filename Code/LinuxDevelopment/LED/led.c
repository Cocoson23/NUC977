#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

#define LEDON _IO('L', 1)
#define LEDOFF _IO('L' ,0)

int main(int argc, char* argv[])
{
	if(argc < 2) {
		printf("error command, usage: ./led ledx\n");
		return -1;
	}
	// 打开字符设备 => 调用对应的led_open()函数
    int fd = open(argv[1],O_RDWR);
	if(-1 == fd) {
        perror("Open LED");
        return -1;
    }

    while(1) {
		ioctl(fd, LEDON, 0);
		sleep(1);
		ioctl(fd, LEDOFF, 0);
		sleep(1);
		ioctl(fd, LEDON, 1);
		sleep(1);
		ioctl(fd, LEDOFF, 1);
		sleep(1);
    }

    // 关闭字符设备 => 调用led_off()函数
	close(fd);

    return 0;
}

