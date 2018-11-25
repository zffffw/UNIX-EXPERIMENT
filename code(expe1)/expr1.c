
#include "apue.h"
#include <fcntl.h>
#include <sys/times.h>
#define COMMAND_ARGV_ERROR 1
#define COMMAND_SYNC_ERROR 2
#define FILE_OPEN_ERROR 3
#define LSEEK_ERROR 4
#define CREATE_BUFFER_ERROR 5
#define READ_BUFFER_ERROR 6
#define WRITE_ERROR 7



int main(int argc, char * argv[]) {
    clock_t clockStart, clockEnd; //定义初试时间和结束时间
    struct tms tmsStart, tmsEnd;
    int fw, size;
    long int fileLength;
    char *buffer;//缓存指针
    float userTime, sysTime, clockTime;
    int ticks;    
    /*判断命令行参数正误*/
    if ( argc < 2 || argc > 3 ) {
        printf("command argc error!");
	exit(COMMAND_ARGV_ERROR);        
    } else if ( argc == 3 && strcmp(argv[2], "sync") != 0) {
        printf("command sync error!");
        exit(COMMAND_SYNC_ERROR);
    }
    /*打开或者创建新的输出文件, 并清除旧的东西*/
    if ( argc == 2 ) {
        if (( fw = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, FILE_MODE)) < 0 ) {
            printf("file open error!");
            exit(FILE_OPEN_ERROR);
        }
    } else if ( argc == 3 ){
        if (( fw = open(argv[1], O_RDWR | O_CREAT | O_TRUNC | O_SYNC, FILE_MODE)) < 0) {
           printf("file open error!"); 
           exit(FILE_OPEN_ERROR);
       }
    }

    /*读取文件*/
    if (( fileLength = lseek(STDIN_FILENO, 0, SEEK_END)) < 0 ) {
        printf("lseek error!");
        exit(LSEEK_ERROR);
    }
    /*开辟缓存空间*/
    if (( buffer = (char *) malloc(sizeof(char) * fileLength)) == NULL) {
        printf("create buffer error!");
        exit(CREATE_BUFFER_ERROR);
    }
    /*将指针指向文件开头*/
    if ( lseek(STDIN_FILENO, 0, SEEK_SET) == -1 ) {
        printf("lseek error!");
        exit(LSEEK_ERROR);
    }
    /*将文件内容读到缓存里面*/
    if ( read(STDIN_FILENO, buffer, fileLength) < 0) {
        printf("read to buffer error!");
        exit(READ_BUFFER_ERROR);
    }
    /*获取滴答时间*/
    ticks = sysconf(_SC_CLK_TCK);
    printf("get ticks:%d\n", ticks);
    fflush(stdout);
    printf("|%-8s| %-8s| %-10s| %-9s| %-10s|\n", "BUFFSIZE", "USER CPU", "SYSTEM CPU", "CLOCK CPU", "LOOP TIMES"); 
    fflush(stdout);
    /*开始写入*/
    for (size = 256; size <= 131072; size *= 2) { //131072b = 128K
        if (lseek(fw, 0, SEEK_SET) == -1) {
            printf("lseek error!");
            exit(LSEEK_ERROR);
        }
        int write_times = fileLength / size;
        int i = 0;
        clockStart = times(&tmsStart);
        for ( i = 0; i < write_times; ++i) {
            if( write(fw, buffer + size*i, size) != size) {
                printf("write error!");
                exit(WRITE_ERROR);
            }
        }
        /*文件大小不是size的整数倍，应把剩余文件内容写入*/ 
        if ( i*size < fileLength) {
            int remain = fileLength % (size);
            if( write(fw, buffer + i*size, remain) != remain) {
                printf("write error!");
                exit(WRITE_ERROR);
            }
            write_times += 1;
        }
        clockEnd = times(&tmsEnd);
        userTime = (float)(tmsEnd.tms_utime - tmsStart.tms_utime)/ticks;
        sysTime = (float)(tmsEnd.tms_stime - tmsStart.tms_stime)/ticks;
        clockTime = (float)(clockEnd - clockStart)/ticks; 
        printf("|%8d| %8.2f| %10.2f| %9.2f| %10d|\n", size, userTime, sysTime, clockTime, write_times);
        fflush(stdout);
    }
}
        
        
             
