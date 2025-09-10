#[[
    Special Thanks to: sohnryang
    This CmakeLists.txt based on https://github.com/sohnryang/csed451
]]
cmake_minimum_required(VERSION 3.19)
project(CSED451)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(WIN_X64_MSVC_BIN_DIR "${CMAKE_SOURCE_DIR}/win-x64-msvc/bin")

include(FetchContent)

find_package(OpenGL REQUIRED)

if(FETCH_GLUT)
  FetchContent_Declare(
    GLUT::GLUT
    GIT_REPOSITORY https://github.com/freeglut/freeglut
    GIT_TAG d35af9686a6c11b5ed1629a054fccc4414ba9854
    OVERRIDE_FIND_PACKAGE)
endif()

if(WIN32)
  # Manually set GLUT for Windows using win-x64-msvc
  set(GLUT_FOUND TRUE)
  set(GLUT_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/win-x64-msvc/include")
  set(GLUT_IMPLIB "${CMAKE_SOURCE_DIR}/win-x64-msvc/lib/freeglut.lib")
  set(GLUT_DLL "${CMAKE_SOURCE_DIR}/win-x64-msvc/lib/freeglut.dll")
    
  add_library(GLUT::GLUT SHARED IMPORTED)
  set_target_properties(GLUT::GLUT PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GLUT_INCLUDE_DIR}"
    IMPORTED_IMPLIB "${GLUT_IMPLIB}"
    IMPORTED_LOCATION "${GLUT_DLL}"
  )
else()
  find_package(GLUT REQUIRED)
endif()

if(FETCH_GLEW)
  FetchContent_Declare(
    GLEW::GLEW
    GIT_REPOSITORY https://github.com/nigels-com/glew
    GIT_TAG 3a8eff77da3658c13fbd3634c943d5251d76322c
    OVERRIDE_FIND_PACKAGE)
endif()

if(WIN32)
  # Manually set GLEW for Windows using win-x64-msvc
  set(GLEW_FOUND TRUE)
  set(GLEW_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/win-x64-msvc/include")
  set(GLEW_IMPLIB "${CMAKE_SOURCE_DIR}/win-x64-msvc/lib/glew32.lib")
  set(GLEW_DLL "${CMAKE_SOURCE_DIR}/win-x64-msvc/lib/glew32.dll")
    
  add_library(GLEW::glew SHARED IMPORTED)
  set_target_properties(GLEW::glew PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GLEW_INCLUDE_DIRS}"
    IMPORTED_IMPLIB "${GLEW_IMPLIB}"
    IMPORTED_LOCATION "${GLEW_DLL}"
  )
else()
  find_package(GLEW REQUIRED)
endif()

# Always use local GLM from win-x64-msvc/include
set(GLM_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/win-x64-msvc/include")

# Create glm::glm target
if(NOT TARGET glm::glm)
  add_library(glm::glm INTERFACE IMPORTED)
  set_target_properties(glm::glm PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}")
endif()

if(ASAN)
  add_compile_options(-fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

function(add_gl_executable_single_file exec_name source_file)
  add_executable(${exec_name} ${source_file})
  target_link_libraries(${exec_name} OpenGL::GL GLUT::GLUT GLEW::glew)
  target_compile_definitions(${exec_name} PRIVATE GL_SILENCE_DEPRECATION)
  if(DEFINED GLM_INCLUDE_DIR)
    target_include_directories(${exec_name} PRIVATE ${GLM_INCLUDE_DIR})
  else()
    target_link_libraries(${exec_name} glm::glm)
  endif()
  
  # Copy DLLs to output directory on Windows
  if(WIN32)
    if(CMAKE_CONFIGURATION_TYPES)
      # Multi-config generators (Visual Studio) - use generator expression
      add_custom_command(TARGET ${exec_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${WIN_X64_MSVC_BIN_DIR}/freeglut.dll"
          "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/freeglut.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${WIN_X64_MSVC_BIN_DIR}/glew32.dll"
          "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/glew32.dll"
        COMMENT "Copying DLLs to $<CONFIG> directory"
      )
    else()
      # Single-config generators - copy once
      add_custom_command(TARGET ${exec_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${WIN_X64_MSVC_BIN_DIR}/freeglut.dll"
          "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/freeglut.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${WIN_X64_MSVC_BIN_DIR}/glew32.dll"
          "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/glew32.dll"
        COMMENT "Copying DLLs to output directory"
      )
    endif()
  endif()
endfunction()

add_subdirectory(0_setup)
