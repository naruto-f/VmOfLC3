/*
 * Created by 123456 on 2023/2/23.
 * 由于vm运行可能需要接受用户的外部输入，所以需要将原先的缓冲区暂存，vm进程结束后再恢复原先的缓冲状态
 * 处理键盘输入，缓冲等平台相关操作的纯虚工具类，不同系统需要继承这个类并实现这些纯虚函数。
 */

#ifndef VMOFLC3_SYSTEMHANDLER_H
#define VMOFLC3_SYSTEMHANDLER_H

#include <cstdint>

class SystemHandler {
public:
    virtual void disableAndStoreCurBuffer() = 0;

    virtual uint16_t checkKey() = 0;
};

#endif //VMOFLC3_SYSTEMHANDLER_H
