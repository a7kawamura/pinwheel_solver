#pragma once
#include "pinwheel/solver.hpp"
#include <iostream>
#include <sstream>
#include <vector>

namespace pinwheel {

template <typename Policy>
void run_solver() {
  std::cout << "周期列を入力してください (例: 3 4 5): ";
  std::string line;
  // 標準入力から1行（スペース区切りの数値列）を読み込む
  if (!std::getline(std::cin, line)) return;

  std::stringstream ss(line);
  std::vector<unsigned int> p;
  unsigned int v;
  while (ss >> v) {
    p.push_back(v);
  }

  if (p.empty()) {
    std::cout << "ERROR: Empty input" << std::endl;
    return;
  }

  // 探索の実行
  auto result = solve_instances<Policy>(all_folds<Policy>(PinwheelInstance(p)));

  if (result) {
    std::cout << "Schedulable via " << result->instance.to_string() << std::endl;
    std::cout << "Schedule: " << result->schedule.to_string() << std::endl;
  } else {
    std::cout << "UNSCHEDULABLE" << std::endl;
  }
}

} // namespace pinwheel
