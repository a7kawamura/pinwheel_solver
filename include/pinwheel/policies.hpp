#pragma once
#include "pinwheel/types.hpp"
#include <optional>
#include <algorithm>

namespace pinwheel {

struct PackingPolicy {
  static State create_initial_state(const PinwheelInstance& c) {
    State initial_state(c.periods.size());
    for (const auto& a : c.periods) initial_state.q.push_back({a - 1, a});
    return initial_state;
  }

  // w = next_State(v, j0): 状態 v であった日に第 j0 番目（v において）の仕事を行うと、次の日は状態 w になる。
  static std::optional<State> next_state(const State& v, size_t j0) {
    State w(v.q.size()); // v と同じ容量を確保して状態 w を作る
    // 以下では v.q の各成分を、適切に書き換えながら w.q に書き写してゆく。
    // j = 0, 1, ..., v.q.size() について順に見てゆく。j = j0 の場合以外は単に対 v.q[j] の左成分を 1 減らして w.q の末尾に書き加える。j = j0 の場合は直ちには書き加えないが、shifting を真にする。shifting が真であるときには、もし次に書き込もうとする対よりも j0 に由来する対 (v.q[j0].second - 1, v.q[j0].second) が辞書順で早ければ、この対を書き加えておく。但し辞書順でといっても、(v.q[j0].second - 1, v.q[j0].second) は左成分が v.q[j0].second - 1 である対のうち最小なので、左成分だけを見ればよい。
    bool shifting = false; // 現在の j の値が j0 以上 j1 以下のとき true（j1 は v.q の第 j0 成分の、w.q における行先）
    for (size_t j = 0; j < v.q.size(); j++) { // j は v 中の着目する成分の番地であり、この繰返しの開始時点で w.q の要素数は j - (shifting ? 1 : 0)
      if (j == j0) shifting = true;
      else {
        if ((not shifting) and v.q[j].first <= j) return std::nullopt;
        w.q.push_back({v.q[j].first - 1, v.q[j].second});
      }
      if (shifting and (j + 1 == v.q.size() or v.q[j + 1].first >= v.q[j0].second)) { // v.q[j + 1].first が（無いか）v.q[j0].second 以上、すなわち現在の j が j1 である
        w.q.push_back({v.q[j0].second - 1, v.q[j0].second});
        shifting = false;
      }
    }
    return w;
  }

  static bool is_executable(const State&, size_t) { return true; }
  static bool should_break_loop(const State&, size_t) { return false; }
  static bool trivially_unschedulable(const PinwheelInstance& c) {
    double rough_density = 0.0;
    for (auto p : c.periods) rough_density += 1.0 / p;
    return rough_density > 1.0;
  }

  // fold(c): 長さ 2 以上の周期列 c の最後の二つの周期を一つに纏めて得られる周期列。fold(c) が割当可能ならば c も然り。
  static PinwheelInstance fold(const PinwheelInstance& c) {
    PinwheelInstance d = c;
    unsigned int new_period = d.periods[d.periods.size() - 2] / 2;
    d.periods.pop_back();
    d.periods[d.periods.size() - 1] = new_period;
    std::sort(d.periods.begin(), d.periods.end());
    return d;
  }
};

struct CoveringPolicy {
  static State create_initial_state(const PinwheelInstance& c) {
    State initial_state(c.periods.size());
    for (const auto& a : c.periods) initial_state.q.push_back({0, a});
    return initial_state;
  }

  // w = next_State(v, j0): 状態 v であった日に第 j0 番目（v において）の活動を行うと（行える入力であることは保証されている）、次の日は状態 w になる。
  static std::optional<State> next_state(const State& v, size_t j0) {
    State w(v.q.size()); // v と同じ容量を確保して状態 w を作る
    bool shifting = false; // 現在の j の値が j0 以上 j1 以下のとき true（j1 は v の第 j0 成分の、w における行先）
    for (size_t j = 0; j < v.q.size(); ++j) { // j は v 中の着目する成分の番地であり、この繰返しの開始時点で w.q の要素数は j - (shifting ? 1 : 0)
      if (j == j0) shifting = true;
      else w.q.push_back({(v.q[j].first == 0) ? 0 : (v.q[j].first - 1), v.q[j].second});
      if (shifting and (j + 1 == v.q.size() or v.q[j + 1].first >= v.q[j0].second)) {
        w.q.push_back({v.q[j0].second - 1, v.q[j0].second});
        shifting = false;
      }
    }
    return w;
  }

  static bool is_executable(const State& v, size_t j) { return v.q[j].first == 0; }
  static bool should_break_loop(const State& v, size_t j) { return v.q[j].first > 0; }
  static bool trivially_unschedulable(const PinwheelInstance& c) {
    double rough_density = 0.0;
    for (auto p : c.periods) rough_density += 1.0 / p;
    return rough_density < 1.0;
  }

  // fold(c): 長さ 2 以上の周期列 c の最後の二つの周期を一つに纏めて得られる周期列。fold(c) が割当可能ならば c も然り。
  static PinwheelInstance fold(const PinwheelInstance& c) {
    PinwheelInstance d = c;
    unsigned int new_period = std::min((d.periods[d.periods.size() - 1] + 1) / 2, d.periods[d.periods.size() - 2]);
    d.periods.pop_back();
    d.periods[d.periods.size() - 1] = new_period;
    std::sort(d.periods.begin(), d.periods.end());
    return d;
  }
};

} // namespace pinwheel
