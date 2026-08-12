cmake_minimum_required(VERSION 3.24)

project(TerminalCpp
    DESCRIPTION "Linux-first terminal application and shell environment"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TERMINALCPP_DEVELOPMENT_STAGE "pre-alpha")
set(TERMINALCPP_MILESTONE "M1-terminal-core-r4")

option(TERMINALCPP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(TERMINALCPP_BUILD_TESTS "Build terminal core smoke tests" ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml)
qt_standard_project_setup(REQUIRES 6.5)

set(TERMINALCPP_CORE_SOURCES
    src/terminal/TerminalScreen.cpp
    src/terminal/TerminalScreen.h
    src/terminal/VtParser.cpp
    src/terminal/VtParser.h
)

qt_add_executable(terminal_cpp
    src/main.cpp
    src/terminal/PtyProcess.cpp
    src/terminal/PtyProcess.h
    src/terminal/TerminalSession.cpp
    src/terminal/TerminalSession.h
    src/terminal/TerminalView.cpp
    src/terminal/TerminalView.h
    ${TERMINALCPP_CORE_SOURCES}
)

set(TERMINALCPP_QML_FILES
    qml/Main.qml
    qml/TerminalPane.qml
    qml/TopBar.qml
    qml/StatusBar.qml
    qml/Theme.qml
)

foreach(qml_file IN LISTS TERMINALCPP_QML_FILES)
    get_filename_component(qml_file_name "${qml_file}" NAME)
    set_source_files_properties("${qml_file}" PROPERTIES
        QT_RESOURCE_ALIAS "${qml_file_name}"
    )
endforeach()

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(terminal_cpp
    URI TerminalCpp
    QML_FILES ${TERMINALCPP_QML_FILES}
)

target_include_directories(terminal_cpp PRIVATE src)

target_compile_definitions(terminal_cpp PRIVATE
    TERMINALCPP_DEVELOPMENT_STAGE="${TERMINALCPP_DEVELOPMENT_STAGE}"
    TERMINALCPP_MILESTONE="${TERMINALCPP_MILESTONE}"
)

target_link_libraries(terminal_cpp PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml)

find_library(UTIL_LIBRARY util)
if(UTIL_LIBRARY)
    target_link_libraries(terminal_cpp PRIVATE ${UTIL_LIBRARY})
endif()

function(terminalcpp_enable_warnings target_name)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(TERMINALCPP_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

terminalcpp_enable_warnings(terminal_cpp)

if(TERMINALCPP_BUILD_TESTS)
    enable_testing()
    qt_add_executable(vt_core_smoke
        tests/vt_core_smoke.cpp
        ${TERMINALCPP_CORE_SOURCES}
    )
    target_include_directories(vt_core_smoke PRIVATE src)
    target_link_libraries(vt_core_smoke PRIVATE Qt6::Core Qt6::Gui)
    terminalcpp_enable_warnings(vt_core_smoke)
    add_test(NAME vt_core_smoke COMMAND vt_core_smoke)
endif()

install(TARGETS terminal_cpp
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
