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
# INCLUDES
include_directories(
  include
  ./submodule/Catch/include
  ./submodule/dlib
  ./submodule/panda3d/built/include
  ./submodule/portaudio/include
  /usr/include/python2.7
  /usr/include/freetype2
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
set(CMAKE_CXX_FLAGS "-O3 ${CXX_17} ${WARNINGS}")
#---------------

################
# LINKER
################
link_directories(
  ./submodule/panda3d/built/lib
  ./submodule/portaudio/built
)
SET(LIBRARIES 
  panda
  pandaexpress
  p3framework
  p3dtool
  p3dtoolconfig
  python2.7
  pthread
  freetype
  portaudio
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
