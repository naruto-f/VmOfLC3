//
// Created by 123456 on 2023/2/22.
//

#include <mem.h>
#include <SystemHandlerFactory.h>

#include <array>
#include <cstdio>

///c++11新特性: 匿名命名空间，在其中定义和声明的变量和函数的作用与static关键字修饰的作用相同，没有外部链接，但与static关键字不同的是可以在其中声明和定义自定义数据类型，而不限于内置数据类型。
namespace {
    const uint16_t MemBlocksLen = 1 << 15;   //65535 memory blocks

    ///键盘输入相关的寄存器：时内存映射寄存器，存放在特定内存位置中，访问这些寄存器需要读写内存
    enum KBR : uint16_t
    {
        KBSR = 0xFE00, /* keyboard status */
        KBDR = 0xFE02  /* keyboard data */
    };
}

struct MemImpl {
    std::array<uint16_t, MemBlocksLen> mem;
    std::unique_ptr<SystemHandler> systemHandler;
};

VmLC3::Memory::Memory() {
    memImpl = std::make_unique<struct MemImpl>();
    std::unique_ptr<SystemHandlerFactory> factory = std::make_unique<SystemHandlerFactory>();
    memImpl->systemHandler = factory->getSystemHandler();
}

uint16_t VmLC3::Memory::memRead(uint16_t address) {
    if (address == KBR::KBSR)
    {
        if (memImpl->systemHandler->checkKey())
        {
            memImpl->mem[KBSR] = (1 << 15);
            memImpl->mem[KBDR] = getchar();
        }
        else
        {
            memImpl->mem[KBSR] = 0;
        }
    }

    return memImpl->mem[address];
}

void VmLC3::Memory::memWrite(uint16_t address, uint16_t val) {
    memImpl->mem[address] = val;
}

uint16_t VmLC3::Memory::getMemBlockNumMax() {
    return MemBlocksLen;
}

VmLC3::Memory::~Memory() {

}


