cmake_minimum_required(VERSION 3.5)

project(TinyParser LANGUAGES C)
set(CMAKE_C_STANDARD 23)


add_executable(TinyParser main.c
    parser.c lexer.c sbo_array.h)

include(GNUInstallDirs)
install(TARGETS TinyParser
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_compile_definitions(TinyParser PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
