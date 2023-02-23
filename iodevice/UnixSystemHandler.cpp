//
// Created by 123456 on 2023/2/23.
//

#if defined(__linux__) || defined(__unix__)

#include <UnixSystemHandler.h>

#include <cstdio>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/mman.h>

namespace {
    struct termios original_tio;

    void disable_input_buffering()
    {
        tcgetattr(STDIN_FILENO, &original_tio);
        struct termios new_tio = original_tio;
        new_tio.c_lflag &= ~ICANON & ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    }

    void restore_input_buffering()
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_tio);
    }

    void handle_interrupt(int signal)
    {
        restore_input_buffering();
        printf("\n");
        exit(-2);
    }
}

void UnixSystemHandler::disableAndStoreCurBuffer() {
    disable_input_buffering();
    signal(SIGINT, handle_interrupt);   //当vm程序结束时发出SIGINT信号，信号处理程序会恢复还原的buffer状态
}

uint16_t UnixSystemHandler::checkKey() {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    return select(1, &readfds, nullptr, nullptr, &timeout) != 0;
}

#endif

