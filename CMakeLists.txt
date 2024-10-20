cmake_minimum_required(VERSION 3.15)
project(qa)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-scripts/cmake")
include(scripts)
conan_init(cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

string(APPEND CMAKE_EXE_LINKER_FLAGS " -fuse-ld=mold")
string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fuse-ld=mold")
string(APPEND CMAKE_MODULE_LINKER_FLAGS " -fuse-ld=mold")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# finding this is not very c++20 compat
# find_program(CPPCHECK cppcheck)
# if(CPPCHECK)
#   set(CMAKE_CXX_CPPCHECK
#     ${CPPCHECK}
#     --suppress=missingInclude
#     --enable=all
#     --inline-suppr)
# endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG          " -fsanitize=address,undefined,leak")
string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -fsanitize=address,undefined,leak")

if (MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wextra-semi -Wshadow-field-in-constructor -Wmissing-noreturn -Wunreachable-code-break)
endif()

# if we want shared libs for some reason
# option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

find_package(benchmark REQUIRED)

include_directories(include csv-parser/include)

add_subdirectory(include/fmt)

add_subdirectory(include/date)

add_subdirectory(include/fast_float)

add_library(toolbelt INTERFACE)
target_include_directories(toolbelt INTERFACE include/toolbelt)
target_link_libraries(toolbelt INTERFACE date fast_float)

add_subdirectory(csv-parser)
# csv parser throws many sign & conversion errors. also a few shadow warnings
target_compile_options(csv PUBLIC -Wno-sign-conversion -Wno-implicit-int-float-conversion -Wno-float-conversion -Wno-shorten-64-to-32 -Wno-shadow -Wno-extra-semi)
target_compile_options(csv_info PRIVATE -Wno-sign-conversion)
target_compile_options(csv_stats PRIVATE -Wno-sign-conversion)
add_executable(qa apps/qanda.cpp)
target_sources(qa PRIVATE include/qa_set.cpp include/qa.cpp include/xos/console.cpp)
target_link_libraries(qa PRIVATE Threads::Threads csv fmt)

add_executable(pi apps/pi.cpp)
add_executable(pytrip apps/pytrip.cpp)
add_executable(graph apps/graph.cpp)
add_executable(fib apps/fib.cpp)
add_executable(sets apps/sets.cpp)
target_link_libraries(sets toolbelt)

add_executable(visitor apps/visitor.cpp)
target_link_libraries(visitor benchmark::benchmark)

add_executable(word_count apps/word_count.cpp)
target_link_libraries(word_count PRIVATE fmt)
target_compile_options(fmt PUBLIC -Wno-missing-noreturn)

add_executable(wgraph apps/wgraph.cpp)
add_executable(move_copy_sem apps/move_copy_sem.cpp)

add_executable(inheritance apps/inheritance.cpp)
add_executable(variant apps/variant.cpp)
add_executable(type_erasure apps/type_erasure.cpp)

add_executable(dyno apps/dyno.cpp)
# dyno header files cause some warnings
target_compile_options(dyno PRIVATE -Wno-sign-conversion -Wno-implicit-int-conversion -Wno-gnu-string-literal-operator-template)
target_include_directories(dyno PRIVATE include/dyno/include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(mandel apps/mandel.cpp)
target_link_libraries(mandel PRIVATE toolbelt sfml-graphics sfml-window sfml-system tbb Threads::Threads)

add_library(mypp include/mypp/mypp.cpp)
target_include_directories(mypp PUBLIC /usr/include/mariadb)
target_link_libraries(mypp PRIVATE mariadb date fmt)
target_link_libraries(mypp PUBLIC date toolbelt)
target_compile_options(mypp PUBLIC -Wno-missing-noreturn)

add_library(conf include/conf/conf.cpp)
target_link_libraries(conf PRIVATE toolbelt)

add_subdirectory(include/ctre)
add_library(myslice include/myslice/myslice.cpp
  include/myslice/table_parse.cpp)
target_include_directories(myslice PRIVATE /usr/include/mariadb)
target_link_libraries(myslice PRIVATE fmt ctre mypp conf)

add_executable(myslice_demo apps/myslice_demo.cpp)
target_include_directories(myslice_demo PRIVATE /usr/include/mariadb)
target_link_libraries(myslice_demo PRIVATE myslice mypp conf fmt)
target_compile_options(myslice_demo PUBLIC -Wno-missing-noreturn)

add_custom_command(
  TARGET myslice POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
  ${CMAKE_SOURCE_DIR}/apps/myslice_demo.ini
  ${CMAKE_CURRENT_BINARY_DIR}/bin/myslice_demo.ini
  COMMENT "Deploying myslice_demo.ini config")

add_executable(wc apps/wc.cpp)
target_link_libraries(wc PRIVATE toolbelt mypp conf fmt date)

add_library(sha1 INTERFACE)
target_include_directories(sha1 INTERFACE include/sha1)

add_executable(hibp apps/hibp.cpp)
target_link_libraries(hibp PRIVATE toolbelt fmt sha1)

