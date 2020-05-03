cmake_minimum_required (VERSION 2.6)
project (sitctl C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -Wall -Wextra")

find_path(NL_INCLUDE_DIR netlink/netlink.h
    /usr/include
    /usr/include/libnl3
    /usr/local/include
    /usr/local/include/libnl3
)

find_library(NL_LIBRARY NAMES nl nl-3)
find_library(NL_ROUTE_LIBRARY NAMES nl-route nl-route-3)

if (NL_INCLUDE_DIR AND NL_LIBRARY)
    set(NL_LIBRARIES ${NL_LIBRARY} ${NL_ROUTE_LIBRARY})
ELSE (NL_INCLUDE_DIR AND NL_LIBRARY)
    message("libnl-*-3-dev packages cannot be found.")
    message("in debian, install the following to continue:")
    message("libnl-3-dev libnl-route-3-dev")
    message(FATAL_ERROR "can't find libnl.")
endif (NL_INCLUDE_DIR AND NL_LIBRARY)

include_directories("${PROJECT_SOURCE_DIR}" "${NL_INCLUDE_DIR}")

add_executable(sitctl 
    src/sit.c
    src/sitctl.c
)

target_link_libraries(sitctl ${NL_LIBRARIES})