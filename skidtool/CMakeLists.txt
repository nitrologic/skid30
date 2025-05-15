cmake_minimum_required(VERSION 3.0)

project(skidtool
	VERSION 0.0.0
	DESCRIPTION "acid500 development harness"
	HOMEPAGE_URL "https://github.com/nitrologic/skid30"
	LANGUAGES C CXX ASM
)

message("### PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}")
message("### CMAKE_CXX_COMPILER_ID ${CMAKE_CXX_COMPILER_ID}")

if(MSVC)
#msvc rules
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std:c++17")
else()
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w -std=c++17")
if(WIN32)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif(WIN32)
endif()

if(WIN32)
add_definitions( -DWIN32 )
add_definitions( -DUNICODE )
add_definitions( -D_UNICODE )
endif(WIN32)

include_directories(
	.
	musashi
	steam/sdk/public
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

ADD_LIBRARY(musashi ${musashi_src} ${musashi_include})

ADD_EXECUTABLE(acid500 
	skidshaders.s
#    steamstub.cpp
#    steamstub.h
	freeaudio/freeaudio.cpp
	freeaudio/freeaudio.h
	freeaudio/mmdevice.cpp
	skidtool.cpp 
	skidnative.cpp
	skidnative.h
	skidkick.h
	machine.cpp
	machine.h 
	exec.h
	monitor.h 
	filedecoder.h 
	loadiff.h
)

# bootstrap that generates m68kops sources
# ADD_EXECUTABLE(make68k musashi/m68kmake.c)

# target_link_libraries(skidtool musashi zlib libpng winmm steam_api64)

if(WIN32)
target_link_libraries(acid500 musashi winmm kernel32)
else()
target_link_libraries(acid500 musashi)
endif()

#macos here
#target_link_libraries(skidtool musashi zlib libpng)
#ADD_EXECUTABLE(test test.cpp)
