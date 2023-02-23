//
// Created by 123456 on 2023/2/22.
//

#ifndef VMOFLC3_VM_H
#define VMOFLC3_VM_H

#include <cstdint>
#include <memory>

struct VmImpl;

namespace VmLC3 {

    class Vm {
    public:
        Vm();
        ~Vm();

        int16_t loadImage(const char* imagePath);

        void run();
    private:
        struct VmImpl* getImpl();

        std::unique_ptr<struct VmImpl> vmImpl;
    };
}


#endif //VMOFLC3_VM_H
