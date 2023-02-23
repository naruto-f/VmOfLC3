add_executable(lc3vm main.cpp vm.cpp ../memory/mem.cpp ../iodevice/UnixSystemHandler.cpp ../iodevice/WinSystemHandler.cpp ../iodevice/SystemHandlerFactory.cpp)
install(TARGETS lc3vm DESTINATION .)