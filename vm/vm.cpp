//
// Created by 123456 on 2023/2/22.
//

#include <vm.h>
#include <mem.h>
#include <SystemHandlerFactory.h>

#include <array>
#include <unordered_map>
#include <functional>
#include <cstdio>
#include <iostream>

namespace {
    ///LC-3有10个寄存器, R0-R7为通用寄存器，PC为程序计数器，COND为条件标志寄存器
    enum RegIndex : uint16_t {
        R0 = 0,
        R1,
        R2,
        R3,
        R4,
        R5,
        R6,
        R7,      /* 用于保存子程序返回后的执行位置 */
        PC,
        COND,
        RCOUNT
    };

    ///LC-3支持的指令集
    enum InstructionOps : uint16_t {
        BR = 0, /* branch */
        ADD,    /* add  */
        LD,     /* load */
        ST,     /* store */
        JSR,    /* jump register */
        AND,    /* bitwise and */
        LDR,    /* load register */
        STR,    /* store register */
        RTI,    /* unused */
        NOT,    /* bitwise not */
        LDI,    /* load indirect */
        STI,    /* store indirect */
        JMP,    /* jump */
        RES,    /* reserved (unused) */
        LEA,    /* load effective address */
        TRAP    /* execute trap */
    };

    ///条件状态寄存器的可取值: 其记录了最近一次计算的结果，即当写寄存器时更新标志
    enum CondFlag : uint16_t
    {
        POS = 1 << 0,   //正数
        ZRO = 1 << 1,   //零
        NEG = 1 << 2    //负数
    };

    ///LC-3文档规定的支持的6种中断类型
    enum struct TrapOps : uint16_t {
        GETC = 0x20,
        OUT = 0x21,
        PUTS = 0x22,
        IN = 0x23,
        PUTSP = 0x24,
        HALT = 0x25
    };

    ///字节序：大端或小端
    enum struct ByteOrder : uint16_t {
        BigEndian = 0,
        SmallEndian
    };
}

struct VmImpl {
    std::array<uint16_t, RCOUNT> Reg;
    std::unique_ptr<VmLC3::Memory> memory;
    std::unique_ptr<SystemHandler> systemHandler;
    std::unordered_map<uint16_t, std::function<void(struct VmImpl*, uint16_t)>> instructionFunc;
    bool runing;
};

namespace {
    //const uint16_t RegNumMax = static_cast<const uint16_t>(RegIndex::RCOUNT);

    ///测试当前运行vm的体系结构的字节序是大端还是小端
    ByteOrder testByteOrderOfThisMechine()
    {
        int x = 1;
        char* c = reinterpret_cast<char*>(&x);

        //数据低位存放在内存低字节则为小端字节序
        if(*c == 1)
        {
            return ByteOrder::SmallEndian;
        }
        return ByteOrder::BigEndian;
    }

    ///改变uint16_t的字节序
    uint16_t changeByteOrder(uint16_t val)
    {
        return (val << 8) | (val >> 8);
    }

    ///符号拓展，将立即数拓展到16位，并保持其符号和值不变
    uint16_t signExtend(uint16_t val, int bitCount) {
        if((val >> (bitCount - 1)) & 1) {
            val |= (0xffff << bitCount);
        }

        return val;
    }

    ///更新条件标志
    void updateFlags(struct VmImpl* impl, uint16_t regIndex)
    {
        if (impl->Reg[regIndex] == 0)
        {
            impl->Reg[COND] = CondFlag::ZRO;
        }
        else if (impl->Reg[regIndex] >> 15)   //uint16_t的最高位即第15位(从0开始)是符号位，如果为1就是负数
        {
            impl->Reg[COND] = CondFlag::NEG;
        }
        else
        {
            impl->Reg[COND] = CondFlag::POS;
        }
    }

    ///由错误或保留的指令使用：输出错误提示并终止程序
    void abort(struct VmImpl* impl, const char* opName) {
        std::cerr << "invalid op: " << opName << "!" << std::endl;
        impl->runing = false;
    }

    ///条件跳转：如果满足测试的条件就跳转到相对当前PC偏移值为offset的位置执行程序
    void br(struct VmImpl* impl, uint16_t instruction) {
        uint16_t testCond = (instruction >> 9) & 0x7;

        if(testCond & impl->Reg[COND]) {
            uint16_t pcOffset = signExtend(instruction & 0x1ff, 9);
            impl->Reg[PC] += pcOffset;
        }
    }

    ///加法运算：两个寄存器内的值或一个寄存器内的值和一个立即数做加法，并将结果存入目的寄存器
    void add(struct VmImpl* impl, uint16_t instruction) {
        uint16_t destReg = (instruction >> 9) & 0x7;
        uint16_t op1Reg = (instruction >> 6) & 0x7;

        uint16_t op1 = impl->Reg[op1Reg];
        uint16_t op2;

        bool immFlag = (instruction >> 5) & 0x1;        /* 标志第二个操作数是否是立即数 */

        if(immFlag) {
            op2 = signExtend(instruction & 0x1f, 5);
        }
        else {
            uint16_t op2Reg = instruction & 0x7;
            op2 = impl->Reg[op2Reg];
        }

        impl->Reg[destReg] = op1 + op2;
        updateFlags(impl, destReg);
    }

    ///以当前PC值为基址，加上偏移后为数据在内存中的地址，再从内存中读取数据到目的寄存器
    void ld(struct VmImpl* impl, uint16_t instruction) {
        uint16_t destReg = (instruction >> 9) & 0x7;
        uint16_t pcOffset = signExtend(instruction & 0x1ff, 9);   //目的地址相对于当前PC的偏移，可正可负，所以需要做符号拓展

        uint16_t address = impl->Reg[PC] + pcOffset;
        impl->Reg[destReg] = impl->memory->memRead(address);

        updateFlags(impl, destReg);
    }

    void st(struct VmImpl* impl, uint16_t instruction) {
        uint16_t srcReg = (instruction >> 9) & 0x7;
        uint16_t pcOffset = signExtend(instruction & 0x1ff, 9);   //目的地址相对于当前PC的偏移，可正可负，所以需要做符号拓展

        uint16_t address = impl->Reg[PC] + pcOffset;
        impl->memory->memWrite(address, impl->Reg[srcReg]);
    }

    ///跳转到子例程: 跳转前将返回地址存入R7
    void jsr(struct VmImpl* impl, uint16_t instruction) {
        impl->Reg[R7] = impl->Reg[PC];
        bool immFlag = (instruction >> 11) & 0x1;
        uint16_t address = 0;

        if(immFlag) {
            uint16_t pcOffset = signExtend(instruction & 0x7ff, 11);
            address = impl->Reg[PC] + pcOffset;
        }
        else {
            uint16_t baseReg = (instruction >> 6) & 0x7;
            address = impl->Reg[baseReg];
        }

        impl->Reg[PC] = address;
    }

    ///按位与：两个寄存器内的值或一个寄存器内的值和一个立即数按位与，并将结果存入目的寄存器
    void bitAnd(struct VmImpl* impl, uint16_t instruction) {
        uint16_t destReg = (instruction >> 9) & 0x7;
        uint16_t op1Reg = (instruction >> 6) & 0x7;

        uint16_t op1 = impl->Reg[op1Reg];
        uint16_t op2;

        bool immFlag = (instruction >> 5) & 0x1;        /* 标志第二个操作数是否是立即数 */

        if(immFlag)
        {
            op2 = signExtend(instruction & 0x1f, 5);
        }
        else {
            uint16_t op2Reg = instruction & 0x7;
            op2 = impl->Reg[op2Reg];
        }

        impl->Reg[destReg] = op1 & op2;
        updateFlags(impl, destReg);
    }

    ///以指定寄存器中的值为基址，加上偏移后为数据在内存中的地址，再从内存中读取数据到目的寄存器
    void ldr(struct VmImpl* impl, uint16_t instruction) {
        uint16_t destReg = (instruction >> 9) & 0x7;
        uint16_t baseReg = (instruction >> 6) & 0x7;
        uint16_t pcOffset = signExtend(instruction & 0x3f, 6);   //目的地址相对于当前PC的偏移，可正可负，所以需要做符号拓展

        uint16_t address = impl->Reg[baseReg] + pcOffset;
        impl->Reg[destReg] = impl->memory->memRead(address);

        updateFlags(impl, destReg);
    }

    void str(struct VmImpl* impl, uint16_t instruction) {
        uint16_t srcReg = (instruction >> 9) & 0x7;
        uint16_t baseReg = (instruction >> 6) & 0x7;
        uint16_t pcOffset = signExtend(instruction & 0x3f, 6);   //目的地址相对于当前PC的偏移，可正可负，所以需要做符号拓展

        uint16_t address = impl->Reg[baseReg] + pcOffset;
        impl->memory->memWrite(address, impl->Reg[srcReg]);
    }

    void rti(struct VmImpl* impl, uint16_t instruction) {
        abort(impl, "RTI");
    }

    ///将指定寄存器中的值按位取反，并存入目的寄存器
    void bitNot(struct VmImpl* impl, uint16_t instruction) {
        uint16_t destReg = (instruction >> 9) & 0x7;
        uint16_t opReg = (instruction >> 6) & 0x7;

        uint16_t val = ~impl->Reg[opReg];
        impl->Reg[destReg] = val;

        updateFlags(impl, destReg);
    }

    ///间接加载：类似于c语言中的二级指针间接寻址，第一次读内存取数据所在的地址，第二次读内存取数据，并存入目的寄存器
    void ldi(struct VmImpl* impl, uint16_t instruction) {
        uint16_t destReg = (instruction >> 9) & 0x7;
        uint16_t pcOffset = signExtend(instruction & 0x1ff, 9);   //目的地址相对于当前PC的偏移，可正可负，所以需要做符号拓展

        uint16_t address = impl->memory->memRead(impl->Reg[PC] + pcOffset);
        impl->Reg[destReg] = impl->memory->memRead(address);

        updateFlags(impl, destReg);
    }

    void sti(struct VmImpl* impl, uint16_t instruction) {
        uint16_t srcReg = (instruction >> 9) & 0x7;
        uint16_t pcOffset = signExtend(instruction & 0x1ff, 9);   //目的地址相对于当前PC的偏移，可正可负，所以需要做符号拓展

        uint16_t address = impl->memory->memRead(impl->Reg[PC] + pcOffset);
        impl->memory->memWrite(address, impl->Reg[srcReg]);
    }

    ///RET指令是JMP指令的一个特例，其跳转到子程序返回后继续执行的地址，LC-3里由R7保存继续执行的位置
    void jmp(struct VmImpl* impl, uint16_t instruction) {
        uint16_t srcReg = (instruction >> 6) & 0x7;

        impl->Reg[PC] = impl->Reg[srcReg];
    }

    ///计算相对PC偏移offset的地址，并存入目的寄存器
    void lea(struct VmImpl* impl, uint16_t instruction) {
        uint16_t destReg = (instruction >> 9) & 0x7;
        uint16_t pcOffset = signExtend(instruction & 0x1ff, 9);

        uint16_t address = impl->Reg[PC] + pcOffset;
        impl->Reg[destReg] = address;
        updateFlags(impl, destReg);
    }

    ///未使用指令
    void res(struct VmImpl* impl, uint16_t instruction) {
        abort(impl, "RES");
    }

    ///获取单个用户输入ASCII字符，并存入R0
    void trapGetc(struct VmImpl* impl) {
        char c;
        std::cin >> c;
        impl->Reg[R0] = static_cast<uint16_t>(c);
        updateFlags(impl, R0);
    }

    ///将预先存入R0的ASCII字符输出到标准输出
    void trapOut(struct VmImpl* impl) {
        char c = static_cast<char>(impl->Reg[R0]);
        std::cout << c << std::flush;
    }

    ///输出提示文字让用户输入一个字符
    void trapIn(struct VmImpl* impl) {
        char c;
        std::cout << "Enter a character: ";
        std::cin >> c;
        std::cout << c;

        impl->Reg[R0] = static_cast<uint16_t>(c);
        updateFlags(impl, R0);
    }

    ///请注意，与 C 字符串不同，字符不是存储在单个字节中，而是存储在单个内存位置中。LC-3 中的内存位置为 16 位，因此字符串中的每个字符的宽度为 16 位。要使用 C 函数显示它，我们需要将每个值转换为 char 并单独输出它们。
    void trapPuts(struct VmImpl* impl) {
        //每个word(16bit)存放一个字符
        uint16_t address = impl->Reg[R0];
        uint16_t data = 0;
        while(data = impl->memory->memRead(address++)) {
            std::cout << static_cast<char>(data);
        }
        std::cout << std::flush;
    }

    ///输出程序停止字符串，并将程序运行标志设为0
    void trapHalt(struct VmImpl* impl) {
        std::cout << "HALT" << std::flush;
        impl->runing = false;
    }

    ///将内存地址存放在R0中的字符串输出
    void trapPutSp(struct VmImpl* impl) {
        //每个word(16bit)存放一个字符
        uint16_t address = impl->Reg[R0];
        uint16_t data = 0;
        ByteOrder order = testByteOrderOfThisMechine();

        while(data = impl->memory->memRead(address++)) {
            if(order == ByteOrder::SmallEndian) {
                data = changeByteOrder(data);
            }

            std::cout << static_cast<char>(data >> 8);
            if(static_cast<char>(data & 0xff)) {
                std::cout << static_cast<char>(data & 0xff);
            }
        }
        std::cout << std::flush;
    }

    void trapHandle(struct VmImpl* impl, TrapOps ops) {
        switch (ops) {
            case TrapOps::GETC:
                trapGetc(impl);
                break;
            case TrapOps::OUT:
                trapOut(impl);
                break;
            case TrapOps::PUTS:
                trapPuts(impl);
                break;
            case TrapOps::IN:
                trapIn(impl);
                break;
            case TrapOps::PUTSP:
                trapPutSp(impl);
                break;
            case TrapOps::HALT:
                trapHalt(impl);
                break;
            default:
                break;
        }
    }

    ///中断处理
    void trap(struct VmImpl* impl, uint16_t instruction) {
        impl->Reg[R7] = impl->Reg[PC];
        TrapOps trapOps = static_cast<TrapOps>(instruction & 0xff);

        trapHandle(impl, trapOps);
    }

    ///获取指令的操作码
    uint16_t getOp(uint16_t instruction)
    {
        return instruction >> 12;
    }

    ///实际执行将映像文件装载到内存的操作
    void loadImageToMem(VmLC3::Memory* mem, FILE* imageFile, ByteOrder order)
    {
        uint16_t origin;     //指示从内存的哪个地址开始装载程序
        fread(&origin, sizeof(origin), 1, imageFile);

        //因为编译出来的LC-3体系结构的二进制代码是大端字节序的，所以如果vm主机是小端就要转换字节序才能在vm运行
        if(order == ByteOrder::SmallEndian)
        {
            origin = changeByteOrder(origin);
        }

        uint16_t memBlockNumsMax = 1 << 15;
        uint16_t tmp[memBlockNumsMax - origin];
        size_t readNums = fread(tmp, sizeof(uint16_t), memBlockNumsMax - origin, imageFile);

        if(order == ByteOrder::SmallEndian)
        {
            for(int i = 0; i < readNums; ++i)
            {
                tmp[i] = changeByteOrder(tmp[i]);
            }
        }

        for(int i = 0; i < readNums; ++i)
        {
            mem->memWrite(origin + i, tmp[i]);
        }
    }
}

VmLC3::Vm::Vm() : vmImpl(std::make_unique<::VmImpl>()) {
    //std::unique_ptr<SystemHandlerFactory> factory = std::make_unique<SystemHandlerFactory>();
    vmImpl->systemHandler = SystemHandlerFactory::getSystemHandler();
    vmImpl->systemHandler->disableAndStoreCurBuffer();

    vmImpl->memory = std::make_unique<VmLC3::Memory>();

    vmImpl->instructionFunc[BR] = br;
    vmImpl->instructionFunc[ADD] = add;
    vmImpl->instructionFunc[LD] = ld;
    vmImpl->instructionFunc[ST] = st;
    vmImpl->instructionFunc[JSR] = jsr;
    vmImpl->instructionFunc[AND] = bitAnd;
    vmImpl->instructionFunc[LDR] = ldr;
    vmImpl->instructionFunc[STR] = str;
    vmImpl->instructionFunc[RTI] = rti;
    vmImpl->instructionFunc[NOT] = bitNot;
    vmImpl->instructionFunc[LDI] = ldi;
    vmImpl->instructionFunc[STI] = sti;
    vmImpl->instructionFunc[JMP] = jmp;
    vmImpl->instructionFunc[RES] = res;
    vmImpl->instructionFunc[LEA] = lea;
    vmImpl->instructionFunc[TRAP] = trap;
}


int16_t VmLC3::Vm::loadImage(const char *imagePath) {
    //LC-3规定程序代码从内存地址0x3000开始执行
    vmImpl->Reg[PC] = 0x3000;

    FILE* imageFile = fopen(imagePath, "rb");
    if(!imageFile)
    {
        return -1;
    }

    ByteOrder order = testByteOrderOfThisMechine();
    loadImageToMem(vmImpl->memory.get(), imageFile, order);
    fclose(imageFile);

    return 0;
}

void VmLC3::Vm::run() {
    vmImpl->runing = true;

    while (vmImpl->runing) {
        uint16_t instruction = vmImpl->memory->memRead(vmImpl->Reg[PC]++);   //获取指令后PC指向下一条指令地址
        uint16_t op = getOp(instruction);
        vmImpl->instructionFunc[op](getImpl(), instruction);
    }
}

struct VmImpl* VmLC3::Vm::getImpl() {
    return vmImpl.get();
}

VmLC3::Vm::~Vm() {

}



