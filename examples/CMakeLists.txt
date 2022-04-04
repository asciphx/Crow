cmake_minimum_required(VERSION 2.8)
project (cc_examples)
set(LIBS ${Boost_LIBRARIES} ${MYSQL_LIBRARY} ${SQLite3_LIBRARIES} ${PostgreSQL_LIBRARIES}
${OPENSSL_LIBRARIES} ${Tcmalloc_LIBRARIES} ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})
if (MSVC)
	add_executable(example_vs example_vs.cpp)
	target_link_libraries(example_vs ${LIBS})
else ()

add_executable(benchmark benchmark.cpp)
target_link_libraries(benchmark ${LIBS})

add_executable(example_websocket websocket/example_ws.cpp)
target_link_libraries(example_websocket ${LIBS})
add_custom_command(OUTPUT ws.html
    COMMAND ${CMAKE_COMMAND} -E
    copy ${PROJECT_SOURCE_DIR}/websocket/static/ws.html ${CMAKE_CURRENT_BINARY_DIR}/static/ws.html
    DEPENDS ${PROJECT_SOURCE_DIR}/websocket/static/ws.html
)
add_custom_target(example_ws_copy ALL DEPENDS ws.html)

# add_executable(helloworld helloworld.cpp)
# target_link_libraries(helloworld ${LIBS})

add_executable(example_ssl ssl/example_ssl.cpp)
target_link_libraries(example_ssl ${LIBS})

add_executable(example_vs example_vs.cpp)
target_link_libraries(example_vs ${LIBS})

add_executable(example_with_all example_with_all.cpp)
add_dependencies(example_with_all amalgamation)
target_link_libraries(example_with_all ${LIBS})
add_custom_command(OUTPUT python/example_test.py
	COMMAND ${CMAKE_COMMAND} -E
	copy ${PROJECT_SOURCE_DIR}/python/example_test.py ${CMAKE_CURRENT_BINARY_DIR}/python/example_test.py
	DEPENDS ${PROJECT_SOURCE_DIR}/python/example_test.py 
)
add_custom_target(example_copy ALL DEPENDS python/example_test.py)
add_executable(example_chat example_chat.cpp)
target_link_libraries(example_chat ${LIBS})
add_custom_command(OUTPUT example_chat.html
	COMMAND ${CMAKE_COMMAND} -E
	copy ${PROJECT_SOURCE_DIR}/example_chat.html ${CMAKE_CURRENT_BINARY_DIR}/example_chat.html
	DEPENDS ${PROJECT_SOURCE_DIR}/example_chat.html 
)
add_custom_target(example_chat_copy ALL DEPENDS example_chat.html)

endif()
