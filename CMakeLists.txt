set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_C_COMPILER "gcc")

cmake_minimum_required(VERSION 3.0.0)
project(garden)

################
# OUTPUT 
################
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
#---------------

################
# DEPENDENCIES
################
find_package(Boost REQUIRED)
# INCLUDES
include_directories(
  include
  ./submodule/Catch/include
  ./submodule/dlib
)
#---------------

################
# COMPILER
################
# SETTINGS
set(CXX_14 "-std=c++14")
set(CXX_17 "-std=c++17 -fconcepts")
set(WARNINGS "-Wall \
  -Wreturn-type \
  -Wno-misleading-indentation  \
  -Wno-unused-but-set-variable  \
  -Wno-unused-local-typedefs  \
  -Wno-unused-variable  \
  -Wno-deprecated-declarations  \
  -Wno-ignored-attributes \
")
# MODES
set(CMAKE_CXX_DEBUG_FLAGS "${CMAKE_CXX_DEBUG_FLAGS} -g -ftemplate-backtrace-limit=0")
set(PROFILE "-fno-omit-frame-pointer")
set(VISIBILITY "-fvisibility")
# FEATURES
set(OPENMP "-fopenmp")
################
# COMMAND
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_DEBUG_FLAGS} ${CXX_17} ${WARNINGS}")
#---------------

################
# LINKER
################
link_directories(
)
SET(LIBRARIES 
  pthread
)
#---------------

################
# BUILD
################
file(GLOB TEST_SRC
  "tests/*.cpp"
)
add_executable(tests
  ${TEST_SRC}
)
target_link_libraries(tests 
  ${LIBRARIES} 
)

file(GLOB MAIN_SRC
  "src/*.cpp"
)
add_executable(main
  ${MAIN_SRC}
)
target_link_libraries(main
  ${LIBRARIES} 
)
#---------------

################
# INSTALL
################
#install(DIRECTORY 
#  include/${CMAKE_PROJECT_NAME}
#  DESTINATION include
#)
#install(TARGETS  ${CMAKE_PROJECT_NAME}
#  RUNTIME DESTINATION bin
#  LIBRARY DESTINATION lib
#  ARCHIVE DESTINATION lib
#)
#---------------
