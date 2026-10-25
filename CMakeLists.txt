cmake_minimum_required(VERSION 3.5)

project(TinyParser LANGUAGES C)

add_executable(TinyParser main.c
    parser.c
    parser.h)

include(GNUInstallDirs)
install(TARGETS TinyParser
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
