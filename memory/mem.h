//
// Created by 123456 on 2023/2/22.
//

#ifndef VMOFLC3_MEM_H
#define VMOFLC3_MEM_H

#include <cstdint>
#include <memory>

struct MemImpl;

namespace VmLC3 {

    class Memory {
    public:
        Memory();

        ~Memory();

        uint16_t memRead(uint16_t address);

        void memWrite(uint16_t address, uint16_t data);

        uint16_t getMemBlockNumMax();

    private:
        std::unique_ptr<struct MemImpl> memImpl;
    };

}


#endif //VMOFLC3_MEM_H
