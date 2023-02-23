//
// Created by 123456 on 2023/2/23.
//

#ifndef VMOFLC3_WINSYSTEMHANDLER_H
#define VMOFLC3_WINSYSTEMHANDLER_H


#include <SystemHandler.h>

class WinSystemHandler : SystemHandler {
public:
    virtual void disableAndStoreCurBuffer() override;

    virtual uint16_t checkKey() override;
};

#endif //VMOFLC3_WINSYSTEMHANDLER_H

