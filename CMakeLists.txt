cmake_minimum_required(VERSION 3.20)

project(obs-teleprompter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

set(OBS_SOURCE_DIR "" CACHE PATH "Path to a matching OBS Studio source tree")
set(OBS_APP_DIR "/Applications/OBS.app" CACHE PATH "Path to OBS.app on macOS")

find_package(libobs QUIET)
find_package(obs-frontend-api QUIET)

if(NOT TARGET OBS::libobs OR NOT TARGET OBS::obs-frontend-api)
  if(NOT APPLE OR NOT EXISTS "${OBS_SOURCE_DIR}/libobs/obs-module.h")
    message(FATAL_ERROR "OBS development packages were not found. On macOS, set OBS_SOURCE_DIR to a matching OBS source tree.")
  endif()

  add_library(OBS::libobs SHARED IMPORTED GLOBAL)
  set_target_properties(OBS::libobs PROPERTIES
    IMPORTED_LOCATION "${OBS_APP_DIR}/Contents/Frameworks/libobs.framework/Versions/A/libobs"
    INTERFACE_INCLUDE_DIRECTORIES "${OBS_SOURCE_DIR}/libobs"
  )

  add_library(OBS::obs-frontend-api SHARED IMPORTED GLOBAL)
  set_target_properties(OBS::obs-frontend-api PROPERTIES
    IMPORTED_LOCATION "${OBS_APP_DIR}/Contents/Frameworks/obs-frontend-api.dylib"
    INTERFACE_INCLUDE_DIRECTORIES "${OBS_SOURCE_DIR}/frontend/api;${OBS_SOURCE_DIR}/libobs"
  )

  set(OBS_DATA_PATH "../Resources")
  set(OBS_PLUGIN_PATH "../PlugIns")
  set(OBS_PLUGIN_DESTINATION "obs-plugins")
  set(OBS_RELEASE_CANDIDATE 0)
  set(OBS_BETA 0)
  configure_file("${OBS_SOURCE_DIR}/libobs/obsconfig.h.in" "${CMAKE_CURRENT_BINARY_DIR}/generated/obsconfig.h")
  set_property(TARGET OBS::libobs APPEND PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}/generated"
  )
endif()
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

add_library(${PROJECT_NAME} MODULE
  src/plugin-main.cpp
  src/teleprompter-window.cpp
  src/teleprompter-window.hpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  OBS::libobs
  OBS::obs-frontend-api
  Qt6::Core
  Qt6::Widgets
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
  PLUGIN_NAME="${PROJECT_NAME}"
  PLUGIN_VERSION="${PROJECT_VERSION}"
)

find_path(SIMDE_INCLUDE_DIR simde/x86/sse2.h
  HINTS /opt/homebrew/opt/simde/include /usr/local/include /usr/include
)
if(SIMDE_INCLUDE_DIR)
  target_include_directories(${PROJECT_NAME} PRIVATE "${SIMDE_INCLUDE_DIR}")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

if(COMMAND add_obs_plugin)
  add_obs_plugin(${PROJECT_NAME} WITH_MESSAGE)
endif()

include(GNUInstallDirs)
install(TARGETS ${PROJECT_NAME}
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/64bit"
)
install(DIRECTORY data/
  DESTINATION "${CMAKE_INSTALL_DATADIR}/obs/obs-plugins/${PROJECT_NAME}"
)
