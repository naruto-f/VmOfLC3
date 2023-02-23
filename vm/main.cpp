//
// Created by 123456 on 2023/2/22.
//

#include <iostream>
#include <vm.h>

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cout << "Usage: vm [imagePath]!"<< std::endl;
        return 0;
    }

    VmLC3::Vm vm;

    int16_t res = vm.loadImage(argv[1]);
    if(res == -1)
    {
        std::cerr << "file open filed!" << std::endl;
        return -1;
    }

    vm.run();

    return 0;
}


