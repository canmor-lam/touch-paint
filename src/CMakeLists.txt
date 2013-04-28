cmake_minimum_required (VERSION 2.8.1)

project (timelink-touchwin-paint-x11)

if (WIN32)
    set (libtimelink-touchwin-sdk_NAME TouchWin_SDK)
else (WIN32)
    set (libtimelink-touchwin-sdk_NAME timelink-touchwin-sdk)
endif (WIN32)

add_executable (timelink-touchwin-paint-x11 main.cpp)

target_link_libraries (timelink-touchwin-paint-x11 ${libtimelink-touchwin-sdk_NAME})
target_link_libraries (timelink-touchwin-paint-x11 X11 Xi)

if (NOT DEFINED CMAKE_INSTALL_BINDIR)
    set (CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
endif ()
install (TARGETS timelink-touchwin-paint-x11 DESTINATION ${CMAKE_INSTALL_BINDIR})
