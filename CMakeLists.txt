cmake_minimum_required(VERSION 3.7)

project(TableGen)

set(CMAKE_CXX_STANDARD 17)

# find LLVM
find_package(LLVM REQUIRED CONFIG)

# llvm libraries
llvm_map_components_to_libnames(llvm_libs Support)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Found LLVM libs ${llvm_libs}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# set CXX flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")

# add include directories
include_directories("include")

set(SOURCE_FILES include/tblgen/Parser.h
        src/Parser.cpp include/tblgen/Record.h src/Record.cpp
        include/tblgen/TableGen.h src/TableGen.cpp include/tblgen/Type.h
        src/Type.cpp include/tblgen/Value.h src/Value.cpp
        include/tblgen/Backend/TableGenBackends.h
        src/Backend/PrintRecords.cpp
        src/Backend/EmitClassHierarchy.cpp
        include/tblgen/Message/Diagnostics.h
        src/Message/Diagnostics.cpp include/tblgen/Lex/Lexer.h src/Lex/Lexer.cpp
        include/tblgen/Lex/TokenKinds.h include/tblgen/Lex/Token.h src/Lex/Token.cpp
        include/tblgen/Lex/SourceLocation.h include/tblgen/Basic/FileManager.h
        src/Basic/FileManager.cpp include/tblgen/Basic/FileUtils.h src/Basic/FileUtils.cpp
        include/tblgen/Basic/IdentifierInfo.h src/Basic/IdentifierInfo.cpp
        include/tblgen/Support/Casting.h
        include/tblgen/Support/Format.h src/Support/Format.cpp include/tblgen/Basic/DependencyGraph.h
        src/Support/LiteralParser.cpp include/tblgen/Support/LiteralParser.h
        src/Message/DiagnosticsEngine.cpp include/tblgen/Message/DiagnosticsEngine.h
        include/tblgen/Support/StringSwitch.h)

add_executable(tblgen main.cpp ${SOURCE_FILES})
target_link_libraries(tblgen ${llvm_libs})

