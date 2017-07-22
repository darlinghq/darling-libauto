project(auto)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -nostdinc")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc -nostdinc++")

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

set(DYLIB_INSTALL_NAME "/usr/lib/libauto.dylib")
add_darling_library(auto SHARED auto_zone.cpp)
make_fat(auto)
target_link_libraries(auto system cxx)

install(TARGETS auto DESTINATION libexec/darling/usr/lib)
