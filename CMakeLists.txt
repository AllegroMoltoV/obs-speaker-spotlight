cmake_minimum_required(VERSION 3.28...3.30)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version} LANGUAGES C CXX)

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" ON)
option(ENABLE_QT "Use Qt functionality" ON)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

# C++17 を要求する
target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_17)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON AUTOUIC ON AUTORCC ON
  )
endif()

# src/ と data/ を自動で拾う (追加のたびに CMakeLists.txt を編集しないため)
file(
  GLOB PLUGIN_SOURCES
  CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.h"
)

file(GLOB_RECURSE PLUGIN_RESOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/locale/*.ini")

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${PLUGIN_SOURCES} ${PLUGIN_RESOURCES})

# Solution Explorer のツリー表示を実ディレクトリに揃える
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${PLUGIN_SOURCES} ${PLUGIN_RESOURCES})

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})

# OBS に依存しない src/core/ の単体テスト。CI のプリセットで有効にする
option(ENABLE_TESTS "Build unit tests for src/core" OFF)
if(ENABLE_TESTS)
  enable_testing()
  file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/core/*.cpp")
  add_executable(speaker-spotlight-core-tests tests/core-tests.cpp ${CORE_SOURCES})
  target_compile_features(speaker-spotlight-core-tests PRIVATE cxx_std_17)
  target_include_directories(speaker-spotlight-core-tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  add_test(NAME core-tests COMMAND speaker-spotlight-core-tests)

  # 生成した CSS をファイルに書き出す道具。ローカルのページで表示を確かめるときに使う
  add_executable(speaker-spotlight-css-dump tests/css-dump.cpp ${CORE_SOURCES})
  target_compile_features(speaker-spotlight-css-dump PRIVATE cxx_std_17)
  target_include_directories(speaker-spotlight-css-dump PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

  # 立ち絵の縮小と書き出しの大きさと時間を計る道具。Qt の webp のプラグインの有無が環境で違うため、ctest には登録しない
  if(ENABLE_QT)
    add_executable(speaker-spotlight-image-bench tests/image-bench.cpp src/portrait-image.cpp ${CORE_SOURCES})
    target_compile_features(speaker-spotlight-image-bench PRIVATE cxx_std_17)
    target_include_directories(speaker-spotlight-image-bench PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(speaker-spotlight-image-bench PRIVATE Qt6::Core Qt6::Gui)
  endif()
endif()

# ビルド直後に cmake --install を実行する (Windows 開発用)
option(ENABLE_POST_BUILD_INSTALL "Run cmake --install after build" ON)

if(WIN32 AND ENABLE_POST_BUILD_INSTALL)
  add_custom_command(
    TARGET ${CMAKE_PROJECT_NAME}
    POST_BUILD
    COMMAND "${CMAKE_COMMAND}" --install "${CMAKE_BINARY_DIR}" --config "$<CONFIG>"
    COMMENT "Installing OBS plugin (cmake --install)"
    VERBATIM
  )
endif()
