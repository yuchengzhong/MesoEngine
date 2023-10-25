# Meso Engine 2024
cmake_minimum_required(VERSION 3.16)

project("MesoEngine" CXX C)
add_definitions(-DGLM_FORCE_DEPTH_ZERO_TO_ONE)

set(Boost_DEBUG ON)
set(Boost_USE_STATIC_LIBS ON)
set(BOOST_ROOT "C:/local/boost_1_83_0")
find_package(Boost REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(LVK_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/lightweightvk)
set(MESO_ENGINE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(MESO_RUNTIMES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Runtimes)
add_subdirectory(ThirdParty/lightweightvk)

function(GroupSourcesByFolder target)
  set(SOURCE_GROUP_DELIMITER "/")
  set(last_dir "")
  set(files "")

  get_target_property(sources ${target} SOURCES)

  foreach(file ${sources})
    file(RELATIVE_PATH relative_file "${PROJECT_SOURCE_DIR}" ${file})
    get_filename_component(dir "${relative_file}" PATH)
    if(NOT "${dir}" STREQUAL "${last_dir}")
      if(files)
        source_group("${last_dir}" FILES ${files})
      endif()
      set(files "")
    endif()
    set(files ${files} ${file})
    set(last_dir "${dir}")
  endforeach()

  if(files)
    source_group("${last_dir}" FILES ${files})
  endif()
endfunction()

set(MESO_ENGINE_INCLUDE_DIRS
    "${LVK_ROOT_DIR}/third-party/deps/src"
    "${LVK_ROOT_DIR}/third-party/deps/src/bc7enc"
    "${LVK_ROOT_DIR}/third-party/deps/src/gli"
    "${LVK_ROOT_DIR}/third-party/deps/src/glm"
    "${LVK_ROOT_DIR}/third-party/deps/src/stb"
    "${LVK_ROOT_DIR}/third-party/deps/src/taskflow"
    "${LVK_ROOT_DIR}/third-party/deps/src/3D-Graphics-Rendering-Cookbook"
    "${MESO_RUNTIMES_ROOT}"
    "${Boost_INCLUDE_DIRS}"
)

foreach(DIR ${MESO_ENGINE_INCLUDE_DIRS})
    include_directories(${DIR})
endforeach()
IF(WIN32)
    set(Boost_LIBRARY_DIRS "${BOOST_ROOT}/lib64-msvc-14.3/")
	MESSAGE(STATUS "Boost_LIBRARY_DIRS=${Boost_LIBRARY_DIRS}")
	link_directories(${Boost_LIBRARY_DIRS})
ENDIF(WIN32)

if(UNIX)
    find_package(OpenGL REQUIRED)
endif()

file(GLOB_RECURSE MESO_RUNTIME_SOURCES CONFIGURE_DEPENDS "${MESO_RUNTIMES_ROOT}/*.h" "${MESO_RUNTIMES_ROOT}/*.cpp" "${MESO_RUNTIMES_ROOT}/*.hpp")
add_library(MesoEngineRuntimes ${MESO_RUNTIME_SOURCES})
GroupSourcesByFolder(MesoEngineRuntimes)
target_include_directories(MesoEngineRuntimes PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(MesoEngineRuntimes PRIVATE LVKLibrary ${Boost_LIBRARIES})
add_subdirectory(Samples)