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
find_package(ICU COMPONENTS uc i18n REQUIRED)
find_package(SDL2 COMPONENTS ttf REQUIRED)
find_package(Bullet REQUIRED)
# INCLUDES
include_directories(
  include
  ${ICU_INCLUDE_DIRS}
  ${SDL2_INCLUDE_DIRS}
  ${BULLET_INCLUDE_DIRS}
  ./submodule/Catch/include
  ./submodule/dlib
)
#---------------

################
# COMPILER
################
# SETTINGS
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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_DEBUG_FLAGS} ${CXX_17} ${WARNINGS} ${ICU_CXX_FLAGS}")
#---------------

################
# LINKER
################
link_directories(
)
SET(LIBRARIES 
  ${ICU_LIBRARIES} 
  ${SDL2_LIBRARIES} 
  SDL2_ttf
  ${BULLET_LIBRARIES} 
)
#---------------

################
# BUILD
################
file(GLOB TEST_SRC
  "tests/*.cpp"
)
file(GLOB MAIN_SRC
  "src/*.cpp"
)
add_executable(tests
  ${TEST_SRC}
)
add_library(${CMAKE_PROJECT_NAME}
  ${MAIN_SRC}
)
target_link_libraries(tests 
  ${LIBRARIES} 
)
target_link_libraries(${CMAKE_PROJECT_NAME}
  ${LIBRARIES} 
)
#---------------

################
# INSTALL
################
install(DIRECTORY 
  include/${CMAKE_PROJECT_NAME}
  DESTINATION include
)
install(TARGETS  ${CMAKE_PROJECT_NAME}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
#---------------
