cmake_minimum_required(VERSION 3.31)
project(Rux VERSION 0.2.2)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# LLVM Backend Support (optional)
option(USE_LLVM_BACKEND "Enable LLVM backend for code generation" OFF)

if (USE_LLVM_BACKEND)
    find_package(LLVM CONFIG REQUIRED)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
    message(STATUS "LLVM include dirs: ${LLVM_INCLUDE_DIRS}")

    add_definitions(${LLVM_DEFINITIONS})
    add_definitions(-DUSE_LLVM_BACKEND)

    # For LLVM 21, use the provided LLVM library variable
    set(llvm_libs LLVM)
endif ()

string(TIMESTAMP RUX_BUILD_DATE "%Y-%m-%d")
string(TIMESTAMP RUX_BUILD_TIME "%H:%M:%S")

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/Include/Rux/Version.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/Rux/Version.h
        @ONLY
)

add_executable(rux
        Source/Main.cpp
        Source/Cli.cpp
        Source/Manifest.cpp
        Source/Package.cpp
        Source/Token.cpp
        Source/Lexer.cpp
        Source/SourceLoader.cpp
        Source/Parser.cpp
        Source/Sema.cpp
        Source/Hir.cpp
        Source/Lir.cpp
        Source/Asm.cpp
        Source/Rcu.cpp
        Source/Linker.cpp
        Include/Rux/Cli.h
        Include/Rux/Manifest.h
        Include/Rux/Package.h
        Include/Rux/Token.h
        Include/Rux/Lexer.h
        Include/Rux/SourceLoader.h
        Include/Rux/Ast.h
        Include/Rux/Parser.h
        Include/Rux/Type.h
        Include/Rux/Sema.h
        Include/Rux/Hir.h
        Include/Rux/Lir.h
        Include/Rux/Asm.h
        Include/Rux/Rcu.h
        Include/Rux/Linker.h
)

if (USE_LLVM_BACKEND)
    target_sources(rux PRIVATE
        Source/LLVM.cpp
        Include/Rux/LLVM.h
    )
    target_include_directories(rux PRIVATE ${LLVM_INCLUDE_DIRS})
    target_link_libraries(rux PRIVATE ${llvm_libs})
    if (NOT WIN32)
        target_sources(rux PRIVATE Source/LLVMThunks.c)
    endif ()
endif ()

target_include_directories(rux PRIVATE
        Include
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

if (WIN32)
    target_link_libraries(rux PRIVATE Psapi Winhttp)
endif ()

set_property(TARGET rux
        PROPERTY MSVC_RUNTIME_LIBRARY "$<IF:$<CONFIG:Release>,MultiThreaded,MultiThreadedDebugDLL>"
)

include(GNUInstallDirs)
install(TARGETS rux RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
