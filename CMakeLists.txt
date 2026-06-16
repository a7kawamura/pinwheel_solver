cmake_minimum_required(VERSION 3.20)
project(PinwheelSolver LANGUAGES CXX)

# 何も指定されなかった場合は、自動的に Release モード（-O3最適化）にする
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# C++20 の使用を強制
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 外部ライブラリ（OpenMP, Boost）の自動検出
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)

# ヘッダーファイルの検索パス（include ディレクトリ）を指定
include_directories(include)

# 1. 詰込型 (packing) の実行ファイル設定
add_executable(packing apps/verify_packing_lemma.cpp)
target_link_libraries(packing PRIVATE OpenMP::OpenMP_CXX Boost::boost)

# 2. 被覆型 (covering) の実行ファイル設定
add_executable(covering apps/verify_covering_lemma.cpp)
target_link_libraries(covering PRIVATE OpenMP::OpenMP_CXX Boost::boost)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(packing PRIVATE -Wno-stringop-overflow -Wno-stringop-overread)
    target_compile_options(covering PRIVATE -Wno-stringop-overflow -Wno-stringop-overread)
endif()
