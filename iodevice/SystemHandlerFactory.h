//
// Created by 123456 on 2023/2/23.
//

#ifndef VMOFLC3_SYSTEMHANDLERFACTORY_H
#define VMOFLC3_SYSTEMHANDLERFACTORY_H

#include <SystemHandler.h>
#include <memory>

//TODO 这里使用的是简单工厂模式，可以使用工厂方法模式进一步优化增加程序的可拓展性
class SystemHandlerFactory {
public:
    ///根据当前vm运行的平台自动创建相关工具类对象，目前支持Unix和Windows
    static std::unique_ptr<SystemHandler> getSystemHandler();
};


#endif //VMOFLC3_SYSTEMHANDLERFACTORY_H
