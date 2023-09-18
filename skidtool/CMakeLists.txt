cmake_minimum_required(VERSION 3.0)

project(skidtool)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std:c++17")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")

if(WIN32)
add_definitions( -DWIN32 )
add_definitions( -DUNICODE )
add_definitions( -D_UNICODE )
endif(WIN32)


include_directories(
  .
  zlib
  libpng
  musashi
  steam/sdk/public
  raylib/src
)
link_directories(
    steam/sdk/redistributable_bin/win64
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

ADD_EXECUTABLE(skidtool 
    steamstub.cpp
    steamstub.h
    freeaudio/freeaudio.cpp
    freeaudio/freeaudio.h
    freeaudio/mmdevice.cpp
    skidtool.cpp 
    machine.cpp
    machine.h 
    exec.h
    monitor.h 
    filedecoder.h 
    loadiff.h
)

# bootstrap that generates m68kops sources
# ADD_EXECUTABLE(make68k musashi/m68kmake.c)

target_link_libraries(skidtool musashi zlib libpng winmm steam_api64)

#macos here
#target_link_libraries(skidtool musashi zlib libpng)
#ADD_EXECUTABLE(test test.cpp)


ADD_EXECUTABLE(raydisplay raydisplay.cpp)
target_link_libraries(raydisplay raylib)
target_link_libraries(raydisplay )
