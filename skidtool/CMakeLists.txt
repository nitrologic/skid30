cmake_minimum_required(VERSION 3.18)

project(skidtool)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std:c++17")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")

include_directories(
  .
  zlib
  libpng
  musashi
)

set(musashi_src
    musashi/m68kcpu.c
    musashi/m68kdasm.c
#    musashi/m68kfpu.c
    musashi/m68kops.c
    musashi/sim.c
#    musashi/osd.c
    musashi/softfloat/softfloat.c
)
set(musashi_include
    musashi/m68kconf.h
    musashi/m68k.h
    musashi/m68kconf.h
    musashi/m68kcpu.h
    musashi/m68kmmu.h
    musashi/m68kops.h
    musashi/sim.h
#    musashi/osd.h
    musashi/mmu.h
    musashi/softfloat/softfloat.h
)

set(zlib_src
    zlib/adler32.c
    zlib/compress.c
    zlib/crc32.c
    zlib/deflate.c
    zlib/gzclose.c
    zlib/gzlib.c
    zlib/gzread.c
    zlib/gzwrite.c
    zlib/infback.c
    zlib/inffast.c
    zlib/inflate.c
    zlib/inftrees.c
    zlib/trees.c
    zlib/uncompr.c
    zlib/zutil.c
)
set(zlib_include
    zlib/crc32.h
    zlib/deflate.h
    zlib/gzguts.h
    zlib/inffast.h
    zlib/inffixed.h
    zlib/inflate.h
    zlib/inftrees.h
    zlib/trees.h
    zlib/zconf.h
    zlib/zlib.h
    zlib/zutil.h
)

set(png_src
    libpng/png.c
    libpng/pngerror.c
    libpng/pngget.c
    libpng/pngmem.c
    libpng/pngpread.c
    libpng/pngread.c
    libpng/pngrio.c
    libpng/pngrtran.c
    libpng/pngrutil.c
    libpng/pngset.c
    libpng/pngtrans.c
    libpng/pngwio.c
    libpng/pngwrite.c
    libpng/pngwtran.c
    libpng/pngwutil.c
)

set(png_include
    libpng/png.h
    libpng/pngconf.h
    libpng/pngdebug.h
    libpng/pnginfo.h
    libpng/pnglibconf.h
    libpng/pngpriv.h
    libpng/pngstruct.h
)

ADD_LIBRARY(zlib ${zlib_src} ${zlib_include})
ADD_LIBRARY(libpng ${png_src} ${png_include})
ADD_LIBRARY(musashi ${musashi_src} ${musashi_include})

ADD_EXECUTABLE(skidtool main.cpp)

target_link_libraries(skidtool musashi zlib libpng)
#curses
# m68kmake

set(make68k_src
    musashi/m68kmake.c
)

ADD_EXECUTABLE(make68k ${make68k_src})

ADD_EXECUTABLE(test test.cpp)
target_link_libraries(test musashi zlib libpng)
