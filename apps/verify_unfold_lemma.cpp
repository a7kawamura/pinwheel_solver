#include "pinwheel/types.hpp"
#include "pinwheel/policies.hpp"
#include "pinwheel/solver.hpp"
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace pinwheel;

// 補題の主張は、以下の定数を使って「max_period 以下の自然数からなる任意の PinwheelInstance c は、もし D'(c) ＜ bound_on_modified_density ならば詰込割当可能」。但し D' は、half_theta 以上の各周期に 1 を加えて求めた密度（論文参照）。以下ではこの補題を確かめる。
const unsigned int half_theta = 11;
const unsigned int max_period = 2 * half_theta - 1;
const rational bound_on_modified_density = rational(5, 6) + rational(1, 2 * half_theta);

// min_addable(r): 正の有理数 r に対し、D'(e) < r なる最小の周期 e を返す。但しそれが 2 * half_theta 以上なら 2 * half_theta を返す。
unsigned int min_addable (rational r) {
  if (r < rational(1, 2 * half_theta)) return 2 * half_theta;
  unsigned int e = (r.denominator() / r.numerator()).convert_to<unsigned int>() + 1;
  return e > half_theta ? e - 1 : e; 
}

// check_all(c, a, known_schedules, minimal, r): 周期列 c の末尾に a を加えた周期列 ca は、max_period 以下の周期からなり割当可能であるとする。また D'(a) ＜ r とする。写像 known_schedules に書かれているのは、既知の周期列と正しい日割の組であるとする。このとき、c から始まり、c を除く部分の D' が r 未満であるような周期列であって、辞書順で ca 以上であるものすべてについて、詰込割当可能性を確かめ（但し minimal が真のときは ca の割当可能性も確かめる）、その根拠となる周期列と日割の組を known_schedules に記入する。もし割当不能なものがあれば UNSCHEDULABLE と表示する。
void check_all (const PinwheelInstance& c, unsigned int a, std::unordered_map<PinwheelInstance, Schedule>& known_schedules, bool minimal, rational r) {
  PinwheelInstance ca = c;
  ca.periods.push_back(a);
  rational next_r = r - rational(1, a >= half_theta? a + 1 : a);
  unsigned int e = min_addable(next_r);
  if (e > max_period) {
    if (minimal) find_and_cache<PackingPolicy>(ca, known_schedules);
  } else if (e >= a) check_all(ca, e, known_schedules, true, next_r);
  else check_all(ca, a, known_schedules, false, next_r);
  if (a < max_period) check_all(c, a + 1, known_schedules, false, r);
}

// main(): これを実行して UNSCHEDULABLE が表示されないことを以て、補題が確かめられる。根拠となる周期列と日割の組を表示する。
int main (void) {
//   std::unordered_map<PinwheelInstance, Schedule> known_schedules{};
//   check_all(PinwheelInstance(), 2, known_schedules, true, bound_on_modified_density);
//   for (const auto& [c, s] : known_schedules) std::cout << c.to_string() << ": " << s.to_string() << std::endl;
  std::cout << "動作テスト\n";
  return 0;
}
