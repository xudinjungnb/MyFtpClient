#ifndef GETCH_H
#define GETCH_H

#include <termios.h>
#include <unistd.h>
#include <stdio.h>

//　修改终端的控制方式，1取消回显、确认　２获取数据　3还原
static int getch(void)
{
    // 用于记录终端的配置信息
    struct termios old;
    // 获取终端的配置信息
    tcgetattr(STDIN_FILENO,&old);
    // 设置新的终端配置   
    struct termios new_1 = old;
    // 取消确认、回显
    new_1.c_lflag &= ~(ICANON|ECHO);
    // 设置终端配置信息
    tcsetattr(STDIN_FILENO,TCSANOW,&new_1);

    // 在新模式下获取数据   
    int key_val = 0;
    do{
        key_val += getchar();
    }while(stdin->_IO_read_end - stdin->_IO_read_ptr);

    // 还原配置信息
    tcsetattr(STDIN_FILENO,TCSANOW,&old);
    // 返回新模式下获取的数据
    return key_val;
}

#endif//GETCH_H
