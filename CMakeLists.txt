cmake_minimum_required(VERSION 3.20)
project(PinwheelSolver LANGUAGES CXX)

# 何も指定されなかった場合は、自動的に Release モード（-O3最適化）にする
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# C++20 の使用を強制
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# コンパイラの警告レベルを最大(Wall, Wextra)にし、さらに警告を致命的エラー(Werror)に格上げする
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Werror)
endif()

# 外部ライブラリ（OpenMP, Boost）の自動検出
find_package(OpenMP REQUIRED)
find_package(Boost REQUIRED)

# ヘッダーファイルの検索パス（include ディレクトリ）を指定
include_directories(include)

# 補題検証プログラム
add_executable(verify_packing_lemma apps/verify_packing_lemma.cpp)
target_link_libraries(verify_packing_lemma PRIVATE OpenMP::OpenMP_CXX Boost::boost)

add_executable(verify_covering_lemma apps/verify_covering_lemma.cpp)
target_link_libraries(verify_covering_lemma PRIVATE OpenMP::OpenMP_CXX Boost::boost)

# g++ 11以上における std::vector コピー時のコンパイラ誤検知警告を抑制する設定
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(verify_packing_lemma PRIVATE -Wno-stringop-overflow -Wno-stringop-overread)
    target_compile_options(verify_covering_lemma PRIVATE -Wno-stringop-overflow -Wno-stringop-overread)
endif()
