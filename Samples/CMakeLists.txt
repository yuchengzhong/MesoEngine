# Meso Engine 2024
cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME "MesoEngine Samples")

if(MSVC)
  add_definitions(-D_CONSOLE)
else()
  add_compile_options(-Wno-deprecated-volatile)
endif()

if(WIN32)
  add_definitions("-DVK_USE_PLATFORM_WIN32_KHR=1")
  add_definitions("-DNOMINMAX")
endif()

function(MESO_set_cxxstd target cpp_version)
  set_property(TARGET ${target} PROPERTY CXX_STANDARD ${cpp_version})
  set_property(TARGET ${target} PROPERTY CXX_STANDARD_REQUIRED ON)
endfunction()

function(MESO_set_folder target folder_name)
  set_property(TARGET ${target} PROPERTY FOLDER ${folder_name})
endfunction()

macro(ADD_DEMO app)
    add_executable(${app} "${app}.cpp")
    target_compile_options(${app} PRIVATE -O3)
    MESO_set_cxxstd(${app} 20)
    MESO_set_folder(${app} ${PROJECT_NAME})
    target_link_libraries(${app} PRIVATE LVKLibrary)
    target_link_libraries(${app} PRIVATE bc7enc)
    target_link_libraries(${app} PRIVATE meshoptimizer)
    target_link_libraries(${app} PRIVATE tinyobjloader)
    target_link_libraries(${app} PRIVATE glfw)
    if(UNIX AND NOT APPLE)
        target_link_libraries(${app} PRIVATE EGL)
    endif()
    target_link_libraries(${app} PRIVATE LVKstb)
    target_link_libraries(${app} PRIVATE MesoEngineRuntimes)
    target_link_libraries(${app} PRIVATE ${Boost_LIBRARIES})
endmacro()

ADD_DEMO("SimpleVoxel")
ADD_DEMO("SimpleShadertoy")
ADD_DEMO("DefaultInstance")

ADD_DEMO("TestVoxel")