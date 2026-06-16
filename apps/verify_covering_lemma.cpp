#include "pinwheel/types.hpp"
#include "pinwheel/policies.hpp"
#include "pinwheel/solver.hpp"
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace pinwheel;

// 補題の主張は、以下の定数を使って「max_period 以下の自然数からなる任意の PinwheelInstance c は、もし modified_density(c) ≧ bound_on_modified_density ならば被覆割当可能」。以下ではこの補題を確かめる。
const unsigned int half_theta = 10;
const unsigned int max_period = 2 * half_theta;
const rational bound_on_modified_density = rational(126449978, 100000000) - rational(1, half_theta);

// modified_density(c): 周期列 c の密度、但し half_theta を超える各周期から 1 を減じて求めたもの。
rational modified_density (const PinwheelInstance& c) { 
  rational sum = 0;
  for (const auto& a : c.periods) sum += rational(1, a - ((a > half_theta) ? 1 : 0));
  return sum; 
}

// check_all(c): 周期列 c の末尾に 3 以上 max_period 以下 half_theta 以外の周期を一つ以上付け加えてできる周期列 d であって modified_density(d) ≧ bound_on_modified_density を満す（が途中まででは満さないない）ものすべてについて、割当可能性を確かめ、その根拠となる周期列と日割の組を known_schedules に記入する。もし割当不能なら UNSCHEDULABLE と表示する。
void check_all (const PinwheelInstance& c, std::unordered_map<PinwheelInstance, Schedule>& known_schedules) {
  for (unsigned int e = c.periods.empty() ? 3 : c.periods[c.periods.size() - 1]; e <= max_period; e += (e == half_theta - 1 ? 2 : 1)) { // 直前の周期（先頭なら 3）以上、max_period 以下、half_theta 以外の整数を小さい順に
    PinwheelInstance d = c;
    d.periods.push_back(e);
    if (modified_density(d) >= bound_on_modified_density) check_one<CoveringPolicy>(d, known_schedules); // 実際には check_one する対象を、既知の割当可能な（或いは単に密度条件を満す）周期列から単調性や分割性で得られないものに限ってもよい（実際それにより若干は高速になる）が、簡潔を優先する
    else check_all(d, known_schedules);
  }
}

// main(): これを実行して UNSCHEDULABLE が表示されないことを以て、補題が確かめられる。根拠となる周期列と日割の組を表示する。
int main (void) {
  std::unordered_map<PinwheelInstance, Schedule> known_schedules{};
  check_all(PinwheelInstance(), known_schedules);
  for (const auto& [c, s] : known_schedules) std::cout << c.to_string() << ": " << s.to_string() << std::endl;
  return 0;
}
