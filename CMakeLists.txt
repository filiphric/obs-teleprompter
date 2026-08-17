cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" PROJECT_VERSION LIMIT_COUNT 1)
if(NOT PROJECT_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
  message(FATAL_ERROR "VERSION.txt must contain a SemVer core version such as 1.2.3")
endif()
if(NOT PROJECT_VERSION STREQUAL _version)
  message(FATAL_ERROR "VERSION.txt (${PROJECT_VERSION}) and buildspec.json (${_version}) must match")
endif()

project(${_name} VERSION "${PROJECT_VERSION}" LANGUAGES CXX)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
find_package(obs-frontend-api REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

target_sources(
  ${CMAKE_PROJECT_NAME}
  PRIVATE src/plugin-main.cpp src/teleprompter-window.cpp src/teleprompter-window.hpp
)

target_link_libraries(
  ${CMAKE_PROJECT_NAME}
  PRIVATE OBS::libobs OBS::obs-frontend-api Qt6::Core Qt6::Widgets
)

target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_definitions(
  ${CMAKE_PROJECT_NAME}
  PRIVATE PLUGIN_NAME="${CMAKE_PROJECT_NAME}" PLUGIN_VERSION="${PROJECT_VERSION}"
)
target_compile_options(
  ${CMAKE_PROJECT_NAME}
  PRIVATE $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON)
set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
