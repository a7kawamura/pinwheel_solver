#include "pinwheel/types.hpp"
#include "pinwheel/policies.hpp"
#include "pinwheel/solver.hpp"
#include <map>

using namespace pinwheel;

const bool fac_log = false;
const bool case_stat = true;
const bool evidence = false;
const bool detail = false;
const bool unfold_case_count = false;
std::map<int, int> L_case_count;
std::map<int, int> R_case_count;

// 補題の主張は、以下の定数を使って「max_period 以下の自然数からなる任意の PinwheelInstance c は、もし D'(c) ＜ bound_on_modified_density ならば詰込割当可能」。但し D' は、half_theta 以上の各周期に 1 を加えて求めた密度（論文参照）。以下ではこの補題を確かめる。
const rational M = rational(5,6); //目標値(0.84など)
const unsigned int min_period = 1;
const unsigned int min_half_theta = std::max(2U,min_period);
const unsigned int max_skip_theta = 10;
// const unsigned int min_half_theta = 11;

rational modified_density (const PinwheelInstance& c, const unsigned int& half_theta) { // 周期列 c の half_theta 以上の各周期に 1 を加えたものの密度。
  rational sum = 0;
  for (const auto& a : c.periods) sum += rational(1, a + (a >= half_theta ? 1 : 0));
  return sum; 
}

void check_sub (const PinwheelInstance& c, std::unordered_map<PinwheelInstance, Schedule>& known_schedules, std::unordered_set<PinwheelInstance>& impossible_schedules, const unsigned int& half_theta) {
  //log
  if(case_stat){
    std::cout << "known:" << (int)(known_schedules.size()) << ", impossible:" << (int)(impossible_schedules.size()) << "\n";
    std::cout << "L:";
    for(auto [theta, count]: L_case_count){
      std::cout << "[" << std::setw(2) << theta << ":" << std::setw(5) << count << "]"; 
    }
    std::cout << "\n";
    std::cout << "R:";
    for(auto [theta, count]: R_case_count){
      std::cout << "[" << std::setw(2) << theta << ":" << std::setw(5) << count << "]"; 
    }
    std::cout << "\n";
  }

  if(detail) {
    std::cout << "now:" << c.to_string() << "\n";
  }
  
  bool skip=false;
  if(half_theta<=max_skip_theta){
    skip=true;
  }
  bool result = find_and_cache<PackingPolicy>(c, known_schedules, &impossible_schedules, fac_log, skip);
  if(result){
    L_case_count[2 * half_theta]++;
    return;
  }
  R_case_count[2 * half_theta]++;
  //cが割当不能ならばunfold(c)に含まれる全ての周期列を調査する

  //はじめに, cに対して展開操作をして得られる全ての周期列をunfoldsに列挙する
  // std::unordered_set<PinwheelInstance> unfolds;
  //unfolds.insert(c); //c自身もunfoldsに含まれる
  if(modified_density(c, half_theta + 1) < M + rational(1, 2 * half_theta + 2) ) check_sub(c, known_schedules, impossible_schedules, half_theta+1);
  //cの各周期をチェックしていく. nowは現在見ているcの添え字, countはcに含まれていたhalf_thetaの個数, exist_halfはcがhalf_thetaを含んでいるときtrue, そうでなければfalseとする.
  int now=0;
  int count=0;
  bool exist_theta_minus_1=false;
  //theta-1はthetaかtheta+1になりうる
  if(c.periods.back()==2*half_theta-1){
      exist_theta_minus_1=true;
      auto d=c;
      d.periods[(int)(d.periods.size())-1]=2*half_theta;
      // unfolds.insert(d);
      if(modified_density(d, half_theta + 1) < M + rational(1, 2 * half_theta + 2) ) check_sub(d, known_schedules, impossible_schedules, half_theta+1);
      d.periods[(int)(d.periods.size())-1]=2*half_theta+1;
      // unfolds.insert(d);
      if(modified_density(d, half_theta + 1) < M + rational(1, 2 * half_theta + 2) ) check_sub(d, known_schedules, impossible_schedules, half_theta+1);
  }
  //half_thetaはunfoldによって2つの周期に置き換えられる。そのそれぞれの周期は2*half_thetaまたは2*half_theta+1である。
  PinwheelInstance cc = c;
  while(now<(int)(cc.periods.size())){
      if(cc.periods[now]==half_theta){
          count++;
          for(int i=now;i<(int)(cc.periods.size())-1;i++){
              cc.periods[i]=cc.periods[i+1];
          }
          cc.periods.pop_back();
          cc.periods.push_back(2*half_theta);
          cc.periods.push_back(2*half_theta);
          auto d=cc;
          // unfolds.insert(d);
          if(modified_density(d, half_theta + 1) < M + rational(1, 2 * half_theta + 2) ) check_sub(d, known_schedules, impossible_schedules, half_theta+1);
          for(int i=1;i<=2*count;i++){
              d.periods[(int)(d.periods.size())-i]=2*half_theta+1;
              // unfolds.insert(d);
              if(modified_density(d, half_theta + 1) < M + rational(1, 2 * half_theta + 2) ) check_sub(d, known_schedules, impossible_schedules, half_theta+1);
          }
          if(exist_theta_minus_1){
              auto e=cc;
              e.periods[(int)(e.periods.size())-2*count-1]=2*half_theta;
              // unfolds.insert(e);
              if(modified_density(e, half_theta + 1) < M + rational(1, 2 * half_theta + 2) ) check_sub(e, known_schedules, impossible_schedules, half_theta+1);
              for(int i=1;i<=2*count+1;i++){
                  e.periods[(int)(e.periods.size())-i]=2*half_theta+1;
                  // unfolds.insert(e);
                  if(modified_density(e, half_theta + 1) < M + rational(1, 2 * half_theta + 2) ) check_sub(e, known_schedules, impossible_schedules, half_theta+1);
              }
          }
      }
      else if(cc.periods[now]<half_theta){
          now++;
      }
      else{
          break;
      }
  }

  // //次に, unfoldsの周期列を, 密度条件を満すもののみに絞り込む
  // std::vector<PinwheelInstance> sorted_list;
  // for(auto c_next:unfolds){
  //   if(detail){
  //     std::cout << "->" << c_next.to_string() << "\n";
  //     std::cout << "density:" << modified_density(c_next, half_theta + 2) << ", bound:" << M+rational(1,2*half_theta+2) << ", dist:" << M+rational(1,2*half_theta+2)-modified_density(c_next, half_theta + 2) << "\n";
  //   }
  //   if(modified_density(c_next, half_theta + 1) < M + rational(1, 2 * half_theta + 2) ){
  //     sorted_list.push_back(c_next);
  //   }
  // }
  // std::sort(sorted_list.begin(), sorted_list.end(),
  // [](auto const& A, auto const& B){
  //     if(A.periods.size()!=B.periods.size())
  //         return A.periods.size()<B.periods.size();
  //     return A.periods < B.periods;
  // });

  // //sorted_listにはunfold(c)の全ての周期列が含まれており, これらを調査する
  // if(unfold_case_count){
  //   std::cout << (int)(sorted_list.size()) << " cases\n";
  // }
  // for(auto c_next:sorted_list){
  //   check_sub(c_next, known_schedules, impossible_schedules, half_theta+1);
  // }
}

// check_all(c, known_schedules, impossible_schedules): 写像 known_schedules に書かれているのは、既知の周期列と正しい日割の組であるとする。このとき、周期列 c の末尾に max_period 以下の周期を一つ以上付け加えてできる周期列 d であって modified_density(d, half_theta) ＜ bound_on_modified_density なるものすべてについて、詰込割当可能性を確かめ、その根拠となる周期列と日割の組を known_schedules に記入する。もし割当不能なら impossible_schedules に追加する。
void check_all (const PinwheelInstance& c, std::unordered_map<PinwheelInstance, Schedule>& known_schedules, std::unordered_set<PinwheelInstance>& impossible_schedules) {
  //ここにcriticalに相当する処理を入れる可能性がある

  if(!c.periods.empty()) {
    check_sub(c, known_schedules, impossible_schedules, min_half_theta);
  }
  for (unsigned int e = c.periods.empty() ? min_period : c.periods[c.periods.size() - 1]; e <= 2 * min_half_theta - 1; ++e) {
    PinwheelInstance d = c;
    d.periods.push_back(e);
    if (modified_density(d, min_half_theta) < M + rational(1, 2 * min_half_theta)) check_all(d, known_schedules, impossible_schedules);
  }
}

// main(): これを実行して UNSCHEDULABLE が表示されないことを以て、補題が確かめられる。根拠となる周期列と日割の組を表示する。
int main (void) {
  std::unordered_map<PinwheelInstance, Schedule> known_schedules{};
  std::unordered_set<PinwheelInstance> impossible_schedules{};
  check_all(PinwheelInstance(), known_schedules, impossible_schedules);
  if(evidence) {
    for (const auto& [c, s] : known_schedules) std::cout << c.to_string() << ": " << s.to_string() << std::endl;
  }
  return 0;
}
