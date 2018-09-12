cmake_minimum_required(VERSION 3.9)

set(PROJECT_NAME agiledisruption::[NAME])

project(${PROJECT_NAME})

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

#ALL THE WARNINGS
add_compile_options("-Wall" "-Wextra" "-Werror" "-pedantic" "-Weffc++" "-Wno-unused")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB_RECURSE source src/*.cpp)

if(WIN32)
  file(GLOB_RECURSE platform_source_tmp src/*.WINDOWS.cxx)
  set(platform_source ${platform_source} ${platform_source_tmp})
endif()

# Handle all unicies
if(UNIX)
  file(GLOB_RECURSE platform_source_tmp src/*.UNIX.cxx)
  set(platform_source ${platform_source} ${platform_source_tmp})
endif()

if(APPLE)
  file(GLOB_RECURSE platform_source_tmp src/*.APPLE.cxx)
  set(platform_source ${platform_source} ${platform_source_tmp})
endif()

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  file(GLOB_RECURSE platform_source_tmp src/*.LINUX.cxx)
  set(platform_source ${platform_source} ${platform_source_tmp})
endif()

add_executable(${PROJECT_NAME} ${source} ${platform_source})

target_link_libraries(${PROJECT_NAME})

file(GLOB_RECURSE interface_source interface/*.cpp)

add_library(${PROJECT_NAME}-interface ${interface_source})

target_link_libraries(${PROJECT_NAME}-interface)

file(GLOB_RECURSE tests tests/*.cpp)

foreach(test ${tests})
  file(RELATIVE_PATH test_rel ${CMAKE_SOURCE_DIR}/tests/ ${test})
  get_filename_component(test_fname ${test_rel} NAME_WE)
  get_filename_component(test_dir ${test_rel} DIRECTORY)
  #string(REGEX REPLACE "/" "_" test_dir ${test_dir})

  set(test_name ${test_dir}_${test_fname})

  add_executable(${test_name} ${test})

  target_link_libraries(${test_name} ${PROJECT_NAME}-interface)

  add_test(${test_name} ${test_name})
endforeach()
