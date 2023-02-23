//
// Created by 123456 on 2023/2/23.
//

#ifndef VMOFLC3_UNIXSYSTEMHANDLER_H
#define VMOFLC3_UNIXSYSTEMHANDLER_H

#include <SystemHandler.h>

class UnixSystemHandler : public SystemHandler {
public:
    virtual void disableAndStoreCurBuffer() override;

    virtual uint16_t checkKey() override;
};



#endif //VMOFLC3_UNIXSYSTEMHANDLER_H
