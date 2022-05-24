cmake_minimum_required(VERSION 3.13)

project(fluke_1900a_dou_usb
    VERSION 0.1.0.0
    DESCRIPTION "USB Data-Output-Unit for Fluke 1900A"
    LANGUAGES CXX ASM
)


add_compile_options(-fdiagnostics-color=always)


if (NOT (${CMAKE_SYSTEM_PROCESSOR} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR}))
    # build firmware only when compiling for target

    set(MMCU "msp430g2452")

    add_compile_options(-Werror -Wall -Wextra -pedantic -pedantic-errors
                        -Wconversion -Wsign-conversion -Wreorder
                        -flto -ffunction-sections -fdata-sections
                        -fno-rtti -fno-exceptions
                        -gstabs
                        -Os
                        -mmcu=${MMCU})


    add_executable(dou_firmware)

    set_target_properties(dou_firmware PROPERTIES
                          LINK_DEPENDS ${PROJECT_SOURCE_DIR}/src/msp430g2452.ld)

    target_compile_features(dou_firmware PRIVATE cxx_std_20)

    target_include_directories(dou_firmware PRIVATE src/)

    target_link_directories(dou_firmware PRIVATE /opt/gcc-msp430-none/include)

    target_link_options(dou_firmware PRIVATE LINKER:--gc-sections
                                             LINKER:-print-memory-usage
                                             LINKER:-Map,${PROJECT_BINARY_DIR}/dou_firmware.map
                                             -T${PROJECT_SOURCE_DIR}/src/${MMCU}.ld
                                             -mmcu=${MMCU})

    target_sources(dou_firmware PRIVATE src/dou.cpp src/dou.hpp
                                        src/msp430.cpp src/msp430.hpp
                                        src/nostd.hpp
                                        src/util.hpp)

    add_custom_command(TARGET dou_firmware POST_BUILD
                       COMMAND ${CMAKE_OBJDUMP} -D $<TARGET_FILE:dou_firmware> > dou_firmware.S
                       COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:dou_firmware> dou_firmware.bin)

else() # building for host => unit tests

    add_executable(dou_unit_tests)

    target_compile_features(dou_unit_tests PRIVATE cxx_std_20)

    target_sources(dou_unit_tests PRIVATE src/dou.cpp src/test.cpp)

endif()
