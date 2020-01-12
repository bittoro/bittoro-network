if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
  # we switched to libuv
  set(SOLARIS ON)
  set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lsocket -lnsl")
  add_definitions(-D_POSIX_PTHREAD_SEMANTICS)
endif()
