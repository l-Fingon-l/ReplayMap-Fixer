cmake_minimum_required(VERSION 3.20)
project(ReplayParser)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(include)
add_executable(ReplayMapFixer_1.32.10.17734 Source.cpp replay.cpp zlib/adler32.c zlib/compress_zlib.c zlib/crc32.c zlib/deflate.c zlib/inffast.c
 zlib/inflate.c zlib/inftrees.c zlib/trees.c zlib/zutil.c)
