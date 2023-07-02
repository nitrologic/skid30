cmake_minimum_required(VERSION 3.24)

project(skidtool)

include_directories(
  zlib
  libpng
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

ADD_EXECUTABLE(tool main.cpp)

target_link_libraries(tool zlib libpng)
