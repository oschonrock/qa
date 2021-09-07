# The MIT License (MIT)
#
# Copyright (c) 2018 Mateusz Pusz
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.15)
project(qa)

# set path to custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-scripts/cmake")

# include common tools and workarounds
include(scripts)

# use Conan configuration if available
conan_init(cmake)

set(CMAKE_CXX_STANDARD 20)
if (MSVC)
	add_compile_options(-Wall)
else()
	add_compile_options(-Wall -Wextra -Wpedantic -Wconversion) # -fsanitize=address)
endif()

find_package(benchmark REQUIRED)

include_directories(include include/toolbelt csv-parser/include)

add_subdirectory(csv-parser)
# csv parser throws many sign & conversion errors
target_compile_options(csv PUBLIC -Wno-sign-conversion -Wno-implicit-int-float-conversion -Wno-float-conversion -Wno-shorten-64-to-32)
target_compile_options(csv_info PRIVATE -Wno-sign-conversion)
target_compile_options(csv_stats PRIVATE -Wno-sign-conversion)
add_executable(qa apps/qanda.cpp)
target_sources(qa PRIVATE include/qa_set.cpp include/qa.cpp include/xos/console.cpp)
target_link_libraries(qa PRIVATE Threads::Threads csv fmt)

add_executable(pi apps/pi.cpp)
add_executable(pytrip apps/pytrip.cpp)
add_executable(graph apps/graph.cpp)
add_executable(fib apps/fib.cpp)

add_executable(visitor apps/visitor.cpp)
target_link_libraries(visitor benchmark::benchmark)

add_subdirectory(include/fmt)
add_executable(word_count apps/word_count.cpp)
target_link_libraries(word_count PRIVATE fmt)

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
target_link_libraries(mandel PRIVATE sfml-graphics sfml-window sfml-system tbb Threads::Threads)

add_executable(mysql apps/mysql.cpp)
target_compile_options(mysql PRIVATE -fsanitize=address)
target_link_libraries(mysql PRIVATE asan mariadbcpp)
