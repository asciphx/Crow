cmake_minimum_required(VERSION 2.8)
project (cc_test)
set(TEST_SRCS unittest.cpp)

set(LIBS ${Boost_LIBRARIES} ${MYSQL_LIBRARY} ${SQLite3_LIBRARIES} ${PostgreSQL_LIBRARIES}
${OPENSSL_LIBRARIES} ${Tcmalloc_LIBRARIES} ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

add_executable(unittest ${TEST_SRCS})
target_link_libraries(unittest ${LIBS})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
# using Clang
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
# using GCC
set_target_properties(unittest PROPERTIES COMPILE_FLAGS "--coverage -fprofile-arcs -ftest-coverage")
target_link_libraries(unittest gcov)
endif()

add_subdirectory(template)
#CXXFLAGS="-g -O0 -Wall -W -Wshadow -Wunused-variable \
#Wunused-parameter -Wunused-function -Wunused -Wno-system-headers \
#-Wno-deprecated -Woverloaded-virtual -Wwrite-strings -fprofile-arcs -ftest-coverage"
#CFLAGS="-g -O0 -Wall -W -fprofile-arcs -ftest-coverage"
#LDFLAGS="-fprofile-arcs -ftest-coverage"
