project(auto)

cmake_minimum_required(VERSION 2.4.0)
enable_language(C ASM)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
	cmake_policy(SET CMP0005 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse -msse2 -msse3 -w -nostdinc -fblocks")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D__DARWIN_UNIX03 -fPIC -w")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -nostdinc -nostdinc++ -fblocks -Wno-deprecated")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -nostdlib -Wl,--version-script=${DARLING_TOP_DIRECTORY}/darwin.map")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/auto_tester)
include_directories(${DARLING_TOP_DIRECTORY}/platform-include)
include_directories(${DARLING_TOP_DIRECTORY}/src/external/libcxx/include)
include_directories(${DARLING_TOP_DIRECTORY}/src/external/libdispatch)
include_directories(${DARLING_TOP_DIRECTORY}/src/external/libdispatch/private)
include_directories(${CMAKE_SOURCE_DIR}/src/external/corefoundation/Headers)
include_directories(${CMAKE_BINARY_DIR}/src/external/corefoundation/Headers)
include_directories(${CMAKE_SOURCE_DIR}/src/external/foundation/Headers)

add_definitions(-DOBJC2RUNTIME=1)
add_definitions(-D__strong=)
add_definitions(-Ddlopen=__darwin_dlopen -Ddlsym=__darwin_dlsym)

set(auto_sources
	Admin.cpp
	auto_gdb_interface.cpp
	auto_impl_utilities.c
	auto_weak.cpp
	auto_zone.cpp
	Bitmap.cpp
	BlockRef.cpp
	ClearStack.cpp
	Definitions.cpp
	Environment.cpp
	InUseEnumerator.cpp
	Large.cpp
	PointerHash.cpp
	Region.cpp
	Subzone.cpp
	SubzonePartition.cpp
	Thread.cpp
	ThreadLocalCollector.cpp
	WriteBarrier.cpp
	ZoneCollectionChecking.cpp
	ZoneCollectors.cpp
	ZoneCompaction.cpp
	Zone.cpp
	ZoneDump.cpp
)

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/darling")
SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_library(auto SHARED ${auto_sources})
target_link_libraries(auto system cxx)

install(TARGETS auto DESTINATION ${CMAKE_INSTALL_LIBDIR}/darling)
