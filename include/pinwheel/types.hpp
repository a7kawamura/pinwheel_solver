#pragma once
#include <vector>
#include <string>
#include <utility>
#include <functional> // std::hash のため
#include <boost/rational.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace pinwheel {

// using rational = boost::rational<long long>; // 不等号の正確な判定のため有理数で計算するが、本プログラムで用いる周期列は各要素が下述の max_period 以下であり、それらの最小公倍数は数億程度なので、分母・分子は long long で十分
using rational = boost::rational<boost::multiprecision::cpp_int>; // 任意桁数（多倍長整数）を使った有理数型
  
////////////////////////////////////////////////////////////////////////////////
// 周期列
////////////////////////////////////////////////////////////////////////////////
// 周期列の型 PinwheelInstance。周期は必ず昇順に並べるものとする。
class PinwheelInstance {
public:
  std::vector<unsigned int> periods;
  PinwheelInstance() = default;
  explicit PinwheelInstance(const std::vector<unsigned int>& p) : periods(p) {}
  
  std::string to_string () const {
    std::string r{};
    for (size_t i = 0; i < periods.size(); ++i) r += (i == 0 ? "(" : " ") + std::to_string(periods[i]) + (i == periods.size() - 1 ? ")" : "");
    return r;
  };
  auto operator <=>(const PinwheelInstance&) const = default; // C++20の三方向比較（== も自動生成されます）
};

inline rational density (const PinwheelInstance& c) { 
  rational sum = 0;
  for (const auto& a : c.periods) sum += rational(1, a);
  return sum; 
}

////////////////////////////////////////////////////////////////////////////////
// 日割
////////////////////////////////////////////////////////////////////////////////
// 有限の日割（或いは、それを繰り返す無限の日割）の型 Schedule。周期（仕事番号ではなく）の列。
class Schedule {
public:
  std::vector<unsigned int> periods;
  template<typename Iter>
  Schedule(Iter first, Iter last) : periods(first, last) {}
  Schedule() = default;
  std::string to_string() const {
    std::string r{};
    for (size_t j = 0; j < periods.size(); ++j) r += (j == 0 ? "[" : " ") + std::to_string(periods[j]) + (j == periods.size() - 1 ? "]" : "");
    return r;
  }
};

////////////////////////////////////////////////////////////////////////////////
// 状態
////////////////////////////////////////////////////////////////////////////////
// 状態の型 State
// 詰込型の場合、「周期 c_j の仕事を c_j - r_j 日前にした（j = 0, ..., k）」という状態（緊急度ベクトル）を列 {{r_1, c_1}, {r_2, c_2}, ..., {r_k, c_k}} で表す。
// 但し (r_j, c_j) が辞書順で昇順になるように並べるものとする。
// 被覆型の場合、「周期 c_j の活動を c_j - r_j 日前にした（j = 0, ..., k）」という状態（残り待ち日数ベクトル）を列 {{r_1, c_1}, {r_2, c_2}, ..., {r_k, c_k}} で表す。
// 但し (r_j, c_j) が辞書順で昇順になるように並べるものとする（例えば r_0 が非 0 ならば行き止り）。
class State {
public:
  std::vector<std::pair<unsigned int, unsigned int>> q;
  explicit State(size_t reserve_size = 0) {
    q.reserve(reserve_size); // 容量 reserve_size を確保して状態を作る
  }
  auto operator <=>(const State&) const = default;
}; 

} // namespace pinwheel

////////////////////////////////////////////////////////////////////////////////
// 自作クラスを unordered_set / unordered_map のキーにするためのハッシュ関数定義
////////////////////////////////////////////////////////////////////////////////
namespace std {
  // PinwheelInstance のハッシュ関数
  template <>
  struct hash<pinwheel::PinwheelInstance> {
    size_t operator()(const pinwheel::PinwheelInstance& c) const {
      size_t seed = 0;
      for (auto v : c.periods) {
        // 一般的なハッシュ結合（Hash Combine）アルゴリズム
        seed ^= std::hash<unsigned int>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }
  };

  // State のハッシュ関数
  template <>
  struct hash<pinwheel::State> {
    size_t operator()(const pinwheel::State& s) const {
      size_t seed = 0;
      for (const auto& p : s.q) {
        seed ^= std::hash<unsigned int>{}(p.first) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<unsigned int>{}(p.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      }
      return seed;
    }
  };
}
