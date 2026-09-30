#include "pinwheel/policies.hpp"
#include "pinwheel/solver.hpp"
#include <iostream>
#include <optional>
#include <unordered_set>
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

std::optional<pinwheel::State> reference_next_state(
    const pinwheel::State& state,
    std::size_t selected) {
  pinwheel::State next(state.q.size());
  bool shifting = false;
  for (std::size_t j = 0; j < state.q.size(); ++j) {
    if (j == selected) {
      shifting = true;
    } else {
      if (!shifting && state.q[j].first <= j) return std::nullopt;
      next.q.push_back({state.q[j].first - 1, state.q[j].second});
    }
    if (shifting &&
        (j + 1 == state.q.size() || state.q[j + 1].first >= state.q[selected].second)) {
      next.q.push_back({state.q[selected].second - 1, state.q[selected].second});
      shifting = false;
    }
  }
  return next;
}

bool transition_filter_matches_reference() {
  using namespace pinwheel;
  const std::vector<PinwheelInstance> instances{
      PinwheelInstance(std::vector<unsigned int>{2, 3}),
      PinwheelInstance(std::vector<unsigned int>{3, 3}),
      PinwheelInstance(std::vector<unsigned int>{3, 4, 5, 8})};

  for (const auto& instance : instances) {
    std::vector<State> states{PackingPolicy::create_initial_state(instance)};
    std::unordered_set<State> visited(states.begin(), states.end());
    for (std::size_t cursor = 0; cursor < states.size() && states.size() < 500; ++cursor) {
      const State state = states[cursor];
      const auto info = PackingPolicy::transition_info(state);
      for (std::size_t j = 0; j < state.q.size(); ++j) {
        const auto expected = reference_next_state(state, j);
        const bool candidate = PackingPolicy::can_transition(state, j, info);
        if (candidate != expected.has_value()) return false;
        if (!expected) continue;

        const State actual = PackingPolicy::next_state(state, j);
        if (actual != *expected) return false;
        if (visited.insert(actual).second) states.push_back(actual);
      }
    }
  }
  return true;
}

} // namespace

int main() {
  using namespace pinwheel;

  const PinwheelInstance schedulable(std::vector<unsigned int>{2, 3});
  if (!transition_filter_matches_reference()) {
    std::cerr << "Packing transition filter differs from the reference\n";
    return 1;
  }

  const auto result = solve_instances<PackingPolicy>(
      std::vector<PinwheelInstance>{schedulable});
  if (!result) {
    std::cerr << "Expected [2, 3] to be schedulable\n";
    return 1;
  }
  if (result->instance != schedulable ||
      !schedule_meets_deadlines(schedulable, result->schedule)) {
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
