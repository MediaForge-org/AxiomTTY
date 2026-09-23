cmake_minimum_required(VERSION 3.24)

project(AxiomTTY
    DESCRIPTION "Linux-first terminal application and shell environment"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(AXIOMTTY_DEVELOPMENT_STAGE "pre-alpha")
set(AXIOMTTY_MILESTONE "M3-settings-io-r2.4")

option(AXIOMTTY_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(AXIOMTTY_BUILD_TESTS "Build terminal core smoke tests" ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Quick Qml)
qt_standard_project_setup(REQUIRES 6.8)

set(AXIOMTTY_CORE_SOURCES
    src/terminal/TerminalScreen.cpp
    src/terminal/TerminalScreen.h
    src/terminal/TerminalSearch.cpp
    src/terminal/TerminalSearch.h
    src/terminal/TerminalPalette.cpp
    src/terminal/TerminalPalette.h
    src/terminal/VtParser.cpp
    src/terminal/VtParser.h
)

qt_add_executable(axiomtty
    src/main.cpp
    src/app/AppSettings.cpp
    src/app/AppSettings.h
    src/terminal/PtyProcess.cpp
    src/terminal/SessionManager.cpp
    src/terminal/SessionManager.h
    src/terminal/SplitNode.cpp
    src/terminal/SplitNode.h
    src/terminal/PtyProcess.h
    src/terminal/TerminalSession.cpp
    src/terminal/TerminalSession.h
    src/terminal/TerminalView.cpp
    src/terminal/TerminalView.h
    ${AXIOMTTY_CORE_SOURCES}
)

set(AXIOMTTY_QML_FILES
    qml/Main.qml
    qml/TerminalPane.qml
    qml/SplitNodeView.qml
    qml/SessionTab.qml
    qml/SearchBar.qml
    qml/CloseConfirmPopup.qml
    qml/SettingsDialog.qml
    qml/HeaderActionButton.qml
    qml/SplitControlButton.qml
    qml/TopBar.qml
    qml/StatusBar.qml
    qml/Theme.qml
    qml/WindowControlButton.qml
)

foreach(qml_file IN LISTS AXIOMTTY_QML_FILES)
    get_filename_component(qml_file_name "${qml_file}" NAME)
    set_source_files_properties("${qml_file}" PROPERTIES
        QT_RESOURCE_ALIAS "${qml_file_name}"
    )
endforeach()

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(axiomtty
    URI AxiomTTY
    QML_FILES ${AXIOMTTY_QML_FILES}
)

target_include_directories(axiomtty PRIVATE src)

target_compile_definitions(axiomtty PRIVATE
    AXIOMTTY_DEVELOPMENT_STAGE="${AXIOMTTY_DEVELOPMENT_STAGE}"
    AXIOMTTY_MILESTONE="${AXIOMTTY_MILESTONE}"
)

target_link_libraries(axiomtty PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Qml)

find_library(UTIL_LIBRARY util)
if(UTIL_LIBRARY)
    target_link_libraries(axiomtty PRIVATE ${UTIL_LIBRARY})
endif()

function(axiomtty_enable_warnings target_name)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
        if(AXIOMTTY_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

axiomtty_enable_warnings(axiomtty)

if(AXIOMTTY_BUILD_TESTS)
    enable_testing()

    qt_add_executable(vt_core_smoke
        tests/vt_core_smoke.cpp
        ${AXIOMTTY_CORE_SOURCES}
    )
    target_include_directories(vt_core_smoke PRIVATE src)
    target_link_libraries(vt_core_smoke PRIVATE Qt6::Core Qt6::Gui)
    axiomtty_enable_warnings(vt_core_smoke)
    add_test(NAME vt_core_smoke COMMAND vt_core_smoke)

    qt_add_executable(split_tree_smoke
        tests/split_tree_smoke.cpp
        src/terminal/SplitNode.cpp
        src/terminal/SplitNode.h
        src/terminal/TerminalSession.cpp
        src/terminal/TerminalSession.h
        src/terminal/PtyProcess.cpp
        src/terminal/PtyProcess.h
        ${AXIOMTTY_CORE_SOURCES}
    )
    target_include_directories(split_tree_smoke PRIVATE src)
    target_link_libraries(split_tree_smoke PRIVATE Qt6::Core Qt6::Gui)
    if(UTIL_LIBRARY)
        target_link_libraries(split_tree_smoke PRIVATE ${UTIL_LIBRARY})
    endif()
    axiomtty_enable_warnings(split_tree_smoke)
    add_test(NAME split_tree_smoke COMMAND split_tree_smoke)

    qt_add_executable(settings_smoke
        tests/settings_smoke.cpp
        src/app/AppSettings.cpp
        src/app/AppSettings.h
    )
    target_include_directories(settings_smoke PRIVATE src)
    target_link_libraries(settings_smoke PRIVATE Qt6::Core)
    axiomtty_enable_warnings(settings_smoke)
    add_test(NAME settings_smoke COMMAND settings_smoke)

    qt_add_executable(session_manager_smoke
        tests/session_manager_smoke.cpp
        src/app/AppSettings.cpp
        src/app/AppSettings.h
        src/terminal/SessionManager.cpp
        src/terminal/SessionManager.h
        src/terminal/SplitNode.cpp
        src/terminal/SplitNode.h
        src/terminal/TerminalSession.cpp
        src/terminal/TerminalSession.h
        src/terminal/PtyProcess.cpp
        src/terminal/PtyProcess.h
        ${AXIOMTTY_CORE_SOURCES}
    )
    target_include_directories(session_manager_smoke PRIVATE src)
    target_link_libraries(session_manager_smoke PRIVATE Qt6::Core Qt6::Gui)
    if(UTIL_LIBRARY)
        target_link_libraries(session_manager_smoke PRIVATE ${UTIL_LIBRARY})
    endif()
    axiomtty_enable_warnings(session_manager_smoke)
    add_test(NAME session_manager_smoke COMMAND session_manager_smoke)

    add_executable(pty_smoke tests/pty_smoke.cpp)
    add_executable(pty_shell_input_smoke tests/pty_shell_input_smoke.cpp)
    if(UTIL_LIBRARY)
        target_link_libraries(pty_smoke PRIVATE ${UTIL_LIBRARY})
        target_link_libraries(pty_shell_input_smoke PRIVATE ${UTIL_LIBRARY})
    endif()
    axiomtty_enable_warnings(pty_smoke)
    axiomtty_enable_warnings(pty_shell_input_smoke)
    add_test(NAME pty_smoke COMMAND pty_smoke)
    add_test(NAME pty_shell_input_smoke COMMAND pty_shell_input_smoke)
endif()

install(TARGETS axiomtty
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
