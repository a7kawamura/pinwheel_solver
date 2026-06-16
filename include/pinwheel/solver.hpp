#pragma once
#include "pinwheel/types.hpp"
#include <unordered_set>  // set から unordered_set へ変更 (高速化)
#include <unordered_map>  // map から unordered_map へ変更 (高速化)
#include <variant>
#include <atomic>
#include <iostream>
#include <vector>
#include <omp.h>

namespace pinwheel {

////////////////////////////////////////////////////////////////////////////////
// 周期列 c が詰込割当可能か否かを、状態グラフが閉路をもつか調べることで判定
////////////////////////////////////////////////////////////////////////////////

// find_cycle_sub(v, dead, visited, done): 
// 実行開始時における約束として、dead に属するどの状態からも閉路に到達不能であり、visited は初期状態から v への経路（但し初期状態から i 番目の状態を i へ写す写像として表す）、path はそれに対応する（有限の）日割である。
// v から閉路に到達可能であるとき、その閉路を返す。さもなくば、v から到達可能な状態をすべて dead に加えた上で、真を返す。但し計算中に done が真になったことに気づくと中断して偽を返す。
template <typename Policy>
std::variant<Schedule, bool> find_cycle_sub (
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
    if (done) return false;
    
    path.periods.push_back(v.q[j].second);
    std::variant<Schedule, bool> r = find_cycle_sub<Policy>(w, dead, visited, path, done);
    path.periods.pop_back();
    
    if (std::holds_alternative<Schedule>(r) or not std::get<bool>(r)) return r; // w から到達可能な閉路が見つかった場合はその閉路を（v から到達可能な閉路として）返すし、計算中断ならば中断と返す
  }
  dead.insert(v);
  visited.erase(v);
  return true;
}

// find_cycle(c, done): 状態グラフ上の閉路検出の方法により、周期列 c の割当可能性を判定する。割当可能なら日割の一つを返し、不能なら真を返す。計算中に done が真になると中断して偽を返す。
template <typename Policy>
std::variant<Schedule, bool> find_cycle (const PinwheelInstance& c, std::atomic<bool>& done) {
  std::unordered_set<State> dead{};
  std::unordered_map<State, size_t> visited{};
  State initial_state = Policy::create_initial_state(c);
  Schedule path{};
  return find_cycle_sub<Policy>(initial_state, dead, visited, path, done);
}

// all_folds(c): 周期列 c の割当可能性を調べるために調べるべき周期列集合。c が割当可能であるには、all_folds(c) の周期列のうち一つ以上が割当可能であることが必要十分。ここでは c, fold(c), fold(fold(c)), ... のうち密度 1 以下（詰込型）ないし 1 以上（被覆型）のもの全体としている。
template <typename Policy>
std::vector<PinwheelInstance> all_folds (const PinwheelInstance& c) {
  if (Policy::trivially_unschedulable(c)) return {};
  if (c.periods.size() <= 1) return {c};
  std::vector<PinwheelInstance> r = all_folds<Policy>(Policy::fold(c));
  r.push_back(c);
  return r;
}

// check_one(c, known_schedules): 写像 known_schedules に書かれているのは、既知の周期列と正しい日割の組であるとする。このとき、周期列 c は割当可能か調べ、真偽を返す。これを all_folds(c) の各周期列の割当可能性を並列に調べることで行う。まず known_schedules から直ちに判るか調べる。判らなければ、並列に find_cycle で調べる。割当可能なら、割当できた周期列とその日割とを表示し、known_schedules に記入する。割当不能なら UNSCHEDULABLE と表示する。
// #pragma omp parallel for を使用し、利用可能な最大スレッド数を活かしながら、各スレッドへ動的に周期列の探索タスクを割り振る。
template <typename Policy>
bool check_one (const PinwheelInstance& c, std::unordered_map<PinwheelInstance, Schedule>& known_schedules) {
  std::vector<PinwheelInstance> cs = all_folds<Policy>(c);
  for (const auto& d : cs) if (known_schedules.contains(d)) return true;

  // 【進捗ログ】現在どの周期列を検証しているかリアルタイムで表示
  std::cout << "Checking: " << c.to_string() << " (folds: " << cs.size() << ")" << std::endl;
  
  std::atomic<bool> done{false};

// cs の各要素を並列に調べる。或るスレッドが解を見つけて共有変数 done を真にすると、他のスレッドは中断して終了する。
#pragma omp parallel for schedule(dynamic)
  for (size_t i = 0; i < cs.size(); ++i) {
    if (done) continue; 
    
    PinwheelInstance d = cs[i];
    std::variant<Schedule, bool> s = find_cycle<Policy>(d, done);
    
    if (std::holds_alternative<Schedule>(s)) {
#pragma omp critical // 共通の known_schedules への書き込みは排他制御する
      {
        if (not done) {
          known_schedules[d] = std::get<Schedule>(s); 
          done = true; 
        } 
      }
    }
  }
  if (not done) std::cout << "UNSCHEDULABLE: " << c.to_string() << std::endl;
  return done;
}

} // namespace pinwheel
