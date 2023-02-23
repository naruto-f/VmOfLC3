//
// Created by 123456 on 2023/2/23.
//

#include <WinSystemHandler.h>

#if defined(_WIN32) || defined(_WIN64)

#include <stdio.h>
#include <stdint.h>
#include <signal.h>
/* windows only */
#include <Windows.h>
#include <conio.h>  // _kbhit

namespace {
    HANDLE hStdin = INVALID_HANDLE_VALUE;
    DWORD fdwMode, fdwOldMode;

    void disable_input_buffering()
    {
        hStdin = GetStdHandle(STD_INPUT_HANDLE);
        GetConsoleMode(hStdin, &fdwOldMode); /* save old mode */
        fdwMode = fdwOldMode
                ^ ENABLE_ECHO_INPUT  /* no input echo */
                ^ ENABLE_LINE_INPUT; /* return when one or
                                         more characters are available */
        SetConsoleMode(hStdin, fdwMode); /* set new mode */
        FlushConsoleInputBuffer(hStdin); /* clear buffer */
    }

    void restore_input_buffering()
    {
         SetConsoleMode(hStdin, fdwOldMode);
    }

    void handle_interrupt(int signal)
    {
        restore_input_buffering();
        printf("\n");
        exit(-2);
    }
}


void WinSystemHandler::disableAndStoreCurBuffer() {
    disable_input_buffering();
    signal(SIGINT, handle_interrupt);   //当vm程序结束时发出SIGINT信号，信号处理程序会恢复还原的buffer状态
}

uint16_t WinSystemHandler::checkKey() {
    return WaitForSingleObject(hStdin, 1000) == WAIT_OBJECT_0 && _kbhit();
}

#endif
