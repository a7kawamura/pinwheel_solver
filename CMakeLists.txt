cmake_minimum_required(VERSION 3.20)
project(PinwheelSolver LANGUAGES CXX)

include(CTest)

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

function(configure_pinwheel_target target_name)
  target_include_directories(${target_name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

  # GCC/Clangでは警告を有効にし、警告をエラーとして扱う
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Werror)
  endif()
endfunction()

# 1. 対話型ソルバー
add_executable(packing_solver apps/packing_solver.cpp)
configure_pinwheel_target(packing_solver)
target_link_libraries(packing_solver PRIVATE OpenMP::OpenMP_CXX Boost::boost)

add_executable(covering_solver apps/covering_solver.cpp)
configure_pinwheel_target(covering_solver)
target_link_libraries(covering_solver PRIVATE OpenMP::OpenMP_CXX Boost::boost)

# 2. 補題検証プログラム
add_executable(verify_packing_lemma apps/verify_packing_lemma.cpp)
configure_pinwheel_target(verify_packing_lemma)
target_link_libraries(verify_packing_lemma PRIVATE OpenMP::OpenMP_CXX Boost::boost)

add_executable(verify_covering_lemma apps/verify_covering_lemma.cpp)
configure_pinwheel_target(verify_covering_lemma)
target_link_libraries(verify_covering_lemma PRIVATE OpenMP::OpenMP_CXX Boost::boost)

if(BUILD_TESTING)
  add_executable(packing_solver_test tests/packing_solver_test.cpp)
  configure_pinwheel_target(packing_solver_test)
  target_link_libraries(packing_solver_test PRIVATE OpenMP::OpenMP_CXX Boost::boost)
  add_test(NAME packing_solver_test COMMAND packing_solver_test)
endif()
