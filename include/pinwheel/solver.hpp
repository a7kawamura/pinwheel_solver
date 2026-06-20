#pragma once
#include "pinwheel/types.hpp"
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <optional>
#include <atomic>
#include <iostream>
#include <vector>
#include <omp.h>

namespace pinwheel {

struct Unschedulable {};
struct Interrupted {};
using CycleResult = std::variant<Schedule, Unschedulable, Interrupted>;

struct SolveResult {
  PinwheelInstance instance;
  Schedule schedule;
};

// find_cycle_sub(v, dead, visited, path, done): 
// 実行開始時における約束として、dead に属するどの状態からも閉路に到達不能であり、visited は初期状態から v への経路（但し初期状態から i 番目の状態を i へ写す写像として表す）、path はそれに対応する（有限の）日割である。
// v から閉路に到達可能であるとき、その閉路を返す。さもなくば、v から到達可能な状態をすべて dead に加えた上で、Unschedulable() を返す。但し計算中に done が真になったことに気づくと中断して Interrupted() を返す。
template <typename Policy>
CycleResult find_cycle_sub (
    const State& v, 
    std::unordered_set<State>& dead, 
    std::unordered_map<State, size_t>& visited, 
    Schedule& path, 
    std::atomic<bool>& done) 
{
  if (visited.contains(v)) return Schedule(path.periods.begin() + visited[v], path.periods.end()); // path のうち第 visited[v] 位置以降を出力
  visited[v] = path.periods.size();
  
  for (size_t j = 0; j < v.q.size(); ++j) {
    if (not Policy::is_executable(v, j)) {
      if (Policy::should_break_loop(v, j)) break;
      continue;
    }
    
    auto ww = Policy::next_state(v, j);
    if (not ww) continue;
    
    State w = ww.value();
    if (dead.contains(w)) continue;
    if (done) return Interrupted();
    
    path.periods.push_back(v.q[j].second);
    CycleResult r = find_cycle_sub<Policy>(w, dead, visited, path, done);
    path.periods.pop_back();
    
    if (std::holds_alternative<Schedule>(r) or std::holds_alternative<Interrupted>(r)) return r; // w から到達可能な閉路が見つかった場合はその閉路を（v から到達可能な閉路として）返すし、計算中断ならば中断と返す
  }
  dead.insert(v);
  visited.erase(v);
  return Unschedulable();
}

// find_cycle(c, done): 状態グラフ上の閉路検出の方法により、周期列 c の割当可能性を判定する。割当可能なら日割の一つを返し、不能なら Unschedulable() を返す。計算中に done が真になると中断して Interrupted() を返す。
template <typename Policy>
CycleResult find_cycle (const PinwheelInstance& c, std::atomic<bool>& done) {
  std::unordered_set<State> dead{};
  std::unordered_map<State, size_t> visited{};
  State initial_state = Policy::create_initial_state(c);
  Schedule path{};
  return find_cycle_sub<Policy>(initial_state, dead, visited, path, done);
}

// solve_instances(cs): cs の各周期列の割当可能性を並列に調べる。割当可能なものが一つでも見つかったらその周期列と日割の組を返す。見つからなければstd::nulloptを返す。
// #pragma omp parallel for を使用し、利用可能な最大スレッド数を活かしながら、各スレッドへ動的に周期列の探索タスクを割り振る。
template <typename Policy>
std::optional<SolveResult> solve_instances (const std::vector<PinwheelInstance>& cs) {
  std::atomic<bool> done{false};
  std::optional<SolveResult> result = std::nullopt;

#pragma omp parallel for schedule(dynamic)
  for (size_t i = 0; i < cs.size(); ++i) {
    if (done) continue;
    auto s = find_cycle<Policy>(cs[i], done);
    if (std::holds_alternative<Schedule>(s)) {
#pragma omp critical
      {
	if (not done) {
	  result = SolveResult{cs[i], std::get<Schedule>(s)};
	  done = true;
	}
      }
    }
  }
  return result;
}


// GCC13の誤検知警告（-Warray-boundsと-Wstringop-overflow）を無視
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wstringop-overread"

// all_folds(c): 周期列 c の割当可能性を調べるために調べるべき周期列集合。c が割当可能であるには、all_folds(c) の周期列のうち一つ以上が割当可能であることが必要十分。ここでは c, fold(c), fold(fold(c)), ... のうち密度 1 以下（詰込型）ないし 1 以上（被覆型）のもの全体としている。
template <typename Policy>
std::vector<PinwheelInstance> all_folds (const PinwheelInstance& c) {
  if (Policy::trivially_unschedulable(c)) return {};
  if (c.periods.size() <= 1) return {c};
  std::vector<PinwheelInstance> r = all_folds<Policy>(Policy::fold(c));
  r.push_back(c);
  return r;
}

#pragma GCC diagnostic pop

// find_and_cache(c, known_schedules): 写像 known_schedules に書かれているのは、既知の周期列と正しい日割の組であるとする。このとき、周期列 c は割当可能か調べ、真偽を返す。これを all_folds(c) の各周期列の割当可能性を並列に調べることで行う。まず known_schedules から直ちに判るか調べる。判らなければ、solve_instances を呼んで調べる。割当可能なら、割当できた周期列とその日割とを表示し、known_schedules に記入する。割当不能なら UNSCHEDULABLE と表示する。
template <typename Policy>
bool find_and_cache (const PinwheelInstance& c, std::unordered_map<PinwheelInstance, Schedule>& known_schedules) {
  std::vector<PinwheelInstance> cs = all_folds<Policy>(c);
  for (const auto& d : cs) if (known_schedules.contains(d)) return true;
  // 【進捗ログ】現在どの周期列を検証しているかリアルタイムで表示
  std::cout << "Checking: " << c.to_string() << " (folds: " << cs.size() << ")" << std::endl;

  auto result = solve_instances<Policy>(cs);
  if (result) {
    known_schedules[result->instance] = result->schedule;
    std::cout << "FOUND: " << result->instance.to_string() << " with schedule: " << result->schedule.to_string() << std::endl;
    return true;
  }
  std::cout << "UNSCHEDULABLE: " << c.to_string() << std::endl;
  return false;
}

} // namespace pinwheel
