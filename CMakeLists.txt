cmake_minimum_required(VERSION 3.16)
project(TexasHoldemPoker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Release)
endif()

function(apply_warnings TargetName)
	if(MSVC)
		target_compile_options(${TargetName} PRIVATE /W4 /utf-8)
	else()
		target_compile_options(${TargetName} PRIVATE -Wall -Wextra)
	endif()
endfunction()

# Everything except Main.cpp lives in a library so the game and the tests share it.
file(GLOB POKER_CORE_SOURCES CONFIGURE_DEPENDS Source/*.cpp)
list(FILTER POKER_CORE_SOURCES EXCLUDE REGEX "Main\\.cpp$")

add_library(PokerCore STATIC ${POKER_CORE_SOURCES})
target_include_directories(PokerCore PUBLIC Source)
apply_warnings(PokerCore)

add_executable(TexasHoldemPoker Source/Main.cpp)
target_link_libraries(TexasHoldemPoker PRIVATE PokerCore)
apply_warnings(TexasHoldemPoker)

file(GLOB POKER_TEST_SOURCES CONFIGURE_DEPENDS Tests/*.cpp)
add_executable(PokerTests ${POKER_TEST_SOURCES})
target_link_libraries(PokerTests PRIVATE PokerCore)
apply_warnings(PokerTests)

enable_testing()
add_test(NAME PokerTests COMMAND PokerTests)
