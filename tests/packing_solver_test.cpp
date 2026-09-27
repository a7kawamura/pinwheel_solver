#include "pinwheel/policies.hpp"
#include "pinwheel/solver.hpp"
#include <iostream>
#include <vector>

namespace {

bool schedule_meets_deadlines(
    const pinwheel::PinwheelInstance& instance,
    const pinwheel::Schedule& schedule) {
  const auto& days = schedule.periods;
  if (days.empty()) return false;

  for (unsigned int period : instance.periods) {
    for (std::size_t start = 0; start < days.size(); ++start) {
      bool found = false;
      for (unsigned int offset = 0; offset < period; ++offset) {
        if (days[(start + offset) % days.size()] == period) {
          found = true;
          break;
        }
      }
      if (!found) return false;
    }
  }
  return true;
}

} // namespace

int main() {
  using namespace pinwheel;

  const PinwheelInstance schedulable(std::vector<unsigned int>{2, 3});
  const auto result = solve_instances<PackingPolicy>(
      std::vector<PinwheelInstance>{schedulable});
  if (!result) {
    std::cerr << "Expected [2, 3] to be schedulable\n";
    return 1;
  }
  if (!schedule_meets_deadlines(result->instance, result->schedule)) {
    std::cerr << "Solver returned a schedule that misses a deadline\n";
    return 1;
  }

  const PinwheelInstance unschedulable(std::vector<unsigned int>{2, 3, 7});
  const auto impossible = solve_instances<PackingPolicy>(
      all_folds<PackingPolicy>(unschedulable));
  if (impossible) {
    std::cerr << "Expected [2, 3, 7] to be unschedulable\n";
    return 1;
  }

  std::cout << "Packing solver tests passed\n";
  return 0;
}
