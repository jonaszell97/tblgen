cmake_minimum_required(VERSION 3.7)

project(TableGen)

# set CXX flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wall -pedantic")
if(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
endif()

# set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET default)

# set Linker flags
if(UNIX AND NOT APPLE)
    set(linker_flags "-fuse-ld=/usr/bin/ld.lld-12 -Wl,--start-group")
else()
    set(linker_flags "")
endif()

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
        include/tblgen/Support/StringSwitch.h src/Support/DynamicLibrary.cpp include/tblgen/Support/DynamicLibrary.h
        include/tblgen/Support/Allocator.h include/tblgen/Support/Optional.h src/TemplateParser.cpp)

add_executable(tblgen main.cpp ${SOURCE_FILES})
target_link_libraries(tblgen PUBLIC dl ${linker_flags} -fvisibility=hidden)