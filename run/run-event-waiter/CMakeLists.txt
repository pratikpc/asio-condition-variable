project(run-event-waiter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(asio CONFIG REQUIRED)

if (UNIX)
	find_package( Threads )
endif (UNIX)

add_executable(${PROJECT_NAME})

file(GLOB_RECURSE LIBRARY_CXX_SOURCES src/*.cxx)
file(GLOB_RECURSE LIBRARY_HXX_SOURCES include/*.hxx)

target_sources(${PROJECT_NAME}
PRIVATE
${LIBRARY_CXX_SOURCES}
${LIBRARY_HXX_SOURCES}
)


if (MSVC)
target_compile_options(${PROJECT_NAME} PRIVATE /Zc:__cplusplus)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
target_compile_options(${PROJECT_NAME} PRIVATE -fcoroutines-ts)
endif()


# Add compile definitions to export
target_compile_definitions(${PROJECT_NAME}
PRIVATE
#	Boost Asio/Beast Coroutine TS support
	ASIO_HAS_CO_AWAIT
	ASIO_HAS_STD_COROUTINE
# HTTP_SERVER Start in Build mode
	ASIO_STANDALONE
	)

message(INFO hello)

target_link_libraries(${PROJECT_NAME}
PRIVATE
	asio::asio
	pc::condition-variable-asio
)

if (UNIX)
	target_link_libraries( ${PROJECT_NAME}
		PRIVATE
 		${CMAKE_THREAD_LIBS_INIT} 
	)
endif (UNIX)