//
// Created by 123456 on 2023/2/23.
//

#include <SystemHandlerFactory.h>
#include <WinSystemHandler.h>
#include <UnixSystemHandler.h>

std::unique_ptr<SystemHandler> SystemHandlerFactory::getSystemHandler() {
#if defined(__linux__) || defined(__unix__)
    return std::make_unique<UnixSystemHandler>();
#elif defined(_WIN32) || defined(_WIN64)
    return std::make_unique<WinSystemHandler>();
#endif
}
