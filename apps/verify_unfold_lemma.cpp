#include "pinwheel/types.hpp"
#include "pinwheel/policies.hpp"
#include "pinwheel/solver.hpp"
#include <omp.h>
#include <atomic>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <variant>
#include <optional>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <iterator>
#include <algorithm>
#include <string>
using namespace pinwheel;

#include <fstream> //ファイル出力用
#include <chrono> //実行時間計測用
int all_check_one_count = 0; //check_oneを呼び出した回数
int sub_check_one_count = 0; //thetaそれぞれについてcheck_oneを呼び出した回数
int sub_skip_count = 0; //thetaそれぞれについてRに入った個数
int r_check_one_count = 0; //現在Rの何番目のケースを探索しているか
int r_all = 0; //Rの元の個数
const bool density_log = false; //密度条件の検証を出力
const bool unfold_operation_log = true; //unfoldの具体的な操作を出力
const bool unfold_result_log = true; //unfoldの展開操作によって出てきたcaseの合計を出力する。この段階では、密度条件による絞り込みは行われていない。
const int all_check_one_count_log_interval = 200; //現在どのケースを調査しているかを表示する。ケース番号がこの値の倍数のときのみ出力する。
double possible_case_maxtime = 0.0; //割当可能なケースに対するcheck_one動作時間の最大値
// int all_find_cycle_sub_count = 0; //find_cycle_subを呼び出した回数
// const bool dead_visited_log = false; //定期的にdeadとvisitedのsizeを出力
// const int all_find_cycle_sub_count_log_interval = 200000; //この値に1回の頻度で出力

// check_one(c, known_schedules): 写像 known_schedules に書かれているのは、既知の周期列と正しい日割の組であるとする。このとき、周期列 c は割当可能か調べ、真偽を返す。これを all_folds(c) の各周期列の割当可能性を並列に調べることで行う。まず known_schedules から直ちに判るか調べる。判らなければ、並列に find_cycle で調べる。割当可能なら、割当できた周期列とその日割とを表示し、known_schedules に記入する。割当不能なら UNSCHEDULABLE と表示する。
// bool check_one (const PinwheelInstance& c, std::unordered_map<PinwheelInstance, Schedule>& known_schedules, std::unordered_set<PinwheelInstance>& impossible_schedules, std::vector<PinwheelInstance>& remain_instances) {
//   // ログ
//   all_check_one_count++;
//   sub_check_one_count++;
//   if(all_check_one_count % all_check_one_count_log_interval == 0 ) std::cout << "all_check_one_count=" << all_check_one_count << ", R:[" << r_check_one_count << "/" << r_all << "], Impossible: " << (int)(impossible_schedules.size()) << ", checking: " << c.to_string() << "\n";

//   if (impossible_schedules.contains(c)) return false;
//   std::vector<PinwheelInstance> cs = all_folds(c);
//   for (const auto& d : cs) if (known_schedules.contains(d)) return true;
//   std::atomic<bool> done{false}; // 共有変数
//   if(cs.size()!=0){ //csの要素数が1以上の場合調査を行う。
//     #pragma omp parallel num_threads(cs.size()) // cs の各要素を並列に調べる。或るスレッドが解を見つけて共有変数 done を真にすると、他のスレッドは中断して終了する。
//     {
//       PinwheelInstance d = cs[omp_get_thread_num()]; // 第 omp_get_thread_num() スレッド（cs[omp_get_thread_num()] を調べる）
//       std::variant<Schedule, bool> s = find_cycle(d, done);
//       #pragma omp critical
//       {
//         if (std::holds_alternative<Schedule>(s) and not done) {
//           known_schedules[d] = std::get<Schedule>(s); 
//           done = true; 
//         } 
//       }
//     }
//   }
//   if (not done) { //cが割当不能ならば remain_instances にcを格納する。
//     for (const auto& d : cs) impossible_schedules.insert(d);
//     remain_instances.push_back(c);
//     sub_skip_count++;
//   }
//   return done;
// }

////////////////////////////////////////////////////////////////////////////////
// main: 補題の検証
////////////////////////////////////////////////////////////////////////////////
// 補題の主張は、以下の定数を使って「"min_period以上 max_period 以下"の自然数からなる任意の PinwheelInstance c は、もし modified_density(c) ＜ bound_on_modified_density ならば詰込割当可能」。以下ではこの補題を確かめる。

const rational M=rational(5,6); //目標値(0.84など)
unsigned int min_period = 1;
unsigned int min_half_theta = std::max(2U,min_period);
// unsigned int min_half_theta = 11;
unsigned int max_half_theta = 11; //half_theta = min_half_theta, 7, ... , max_half_thetaの順に調査する

rational modified_density (const PinwheelInstance& c, unsigned int& half_theta) { // 周期列 c の half_theta 以上の各周期に 1 を加えたものの密度。
  rational sum = 0;
  for (const auto& a : c.periods) sum += rational(1, a + (a >= half_theta ? 1 : 0));
  return sum; 
}

// critical(c): 周期列 c は空でなく modified_density(c) ＜ bound_on_modified_density を満すとする。このとき、この条件を満す周期列すべての割当可能性を確かめるという文脈において周期列 c は本質的であるか判定する。但し c が本質的でないとは、周期列 d が存在してやはり modified_density(d) ＜ bound_on_modified_density を満し、d から c が明らかに単調性で得られることをいう。
bool critical (std::unordered_map<PinwheelInstance, Schedule>& known_schedules, std::unordered_set<PinwheelInstance>& impossible_schedules, std::vector<PinwheelInstance>& remain_schedules, const PinwheelInstance& c, unsigned int& half_theta, rational& bound_on_modified_density) {
  if (c.periods.empty()) return false;

  PinwheelInstance e = c;
  e.periods.pop_back();
  for(int i=c.periods.size()-1;i>=0;i--){
    // e は c から i 番目を抜いたものになっている

    if(impossible_schedules.contains(e)) {
      //cも割当不能
      impossible_schedules.insert(c);
      remain_schedules.push_back(c);
      return false;
    }

    if(i>0){
      e.periods[i-1]=c.periods[i];
    }
  }

  if (c.periods[c.periods.size()-1]==min_period) return true;
  PinwheelInstance d = c;
  d.periods[d.periods.size() - 1]--;
  if (modified_density(d, half_theta) < bound_on_modified_density) {
    if(known_schedules.contains(d)){
      //cも割当可能
      known_schedules[c]={};
      return false;
    }
  }
  return true;
}

// check_all(c, known_schedules): 写像 known_schedules に書かれているのは、既知の周期列と正しい日割の組であるとする。このとき、周期列 c の末尾に max_period 以下の周期を一つ以上付け加えてできる周期列 d であって modified_density(d) ＜ bound_on_modified_density なるものすべてについて、詰込割当可能性を確かめ、その根拠となる周期列と日割の組を known_schedules に記入する。もし割当不能なら UNSCHEDULABLE と表示する。
//  | 
//  v
// check_all(c, known_schedules, remain_schedules, half_theta, bound_on_modified_density): 写像 known_schedules に書かれているのは、既知の周期列と正しい日割の組であるとする。このとき、周期列 c の末尾に max_period 以下の周期を一つ以上付け加えてできる周期列 d であって modified_density(d, half_theta) ＜ bound_on_modified_density なるものすべてについて、詰込割当可能性を確かめ、その根拠となる周期列と日割の組を known_schedules に記入する。もし割当不能なら remain_schedules に追加する。
void check_all (const PinwheelInstance& c, std::unordered_map<PinwheelInstance, Schedule>& known_schedules, std::unordered_set<PinwheelInstance>& impossible_schedules, std::vector<PinwheelInstance>& remain_schedules, unsigned int& half_theta, rational& bound_on_modified_density) {
  // if(all_check_one_count % all_check_one_count_log_interval == 0 ) std::cout << "now " << c.to_string() << std::endl;
    if (critical(known_schedules, impossible_schedules, remain_schedules, c, half_theta, bound_on_modified_density)){
        if(!c.periods.empty()) {
        // ログ
        all_check_one_count++;
        sub_check_one_count++;
        if(all_check_one_count % all_check_one_count_log_interval == 0 ) std::cout << "all_check_one_count=" << all_check_one_count << ", R:[" << r_check_one_count << "/" << r_all << "], Impossible: " << (int)(impossible_schedules.size()) << ", pcm=" << possible_case_maxtime << ", checking: " << c.to_string() << "\n";
        auto t1 = std::chrono::steady_clock::now();
        long long cnt = (long long)(impossible_schedules.size());
        check_one<PackingPolicy>(c, known_schedules, impossible_schedules, remain_schedules);
        auto t2 = std::chrono::steady_clock::now();
        double sec = std::chrono::duration<double>(t2 - t1).count();
            if(sec>=0.1){
                std::cout << "Time: " << sec << " ";
                if((long long)(impossible_schedules.size()) - cnt == 1){
                    std::cout << "Impossible";
                }
                else{
                    std::cout << "Possible";
                    possible_case_maxtime=std::max(possible_case_maxtime,sec);
                }
                std::cout << " " << c.to_string() << "\n";
            }
        }
    }
  for (unsigned int e = c.periods.empty() ? min_period : c.periods[c.periods.size() - 1]; e <= 2 * half_theta - 1; ++e) {
    PinwheelInstance d = c;
    d.periods.push_back(e);
    if (modified_density(d, half_theta) < bound_on_modified_density) check_all(d, known_schedules, impossible_schedules, remain_schedules, half_theta, bound_on_modified_density);
  }
}

// check_unfold(known_schedules, remain_schedules, half_theta, bound_on_modified_density): 写像 known_schedules に書かれているのは、既知の周期列と正しい日割の組であるとする。
// remain_schedulesのある周期列に対するunfoldから得られるような周期列 d であって modified_density(d, half_theta) ＜ bound_on_modified_density なるものすべてについて、詰込割当可能性を確かめ、その根拠となる周期列と日割の組を known_schedules に記入する。
// ここで割当不能となった d からなる集合をあらためて remain_schedules とする。
void check_unfold (std::unordered_map<PinwheelInstance, Schedule>& known_schedules, std::unordered_set<PinwheelInstance>& impossible_schedules, std::vector<PinwheelInstance>& remain_schedules, unsigned int& half_theta, rational& bound_on_modified_density) {
  //unfold
  std::unordered_set<PinwheelInstance> unfolds;
  for (auto c : remain_schedules) { //各cに対してunfold(c)に含まれる周期列を全て求め、unfoldsに追加する。
    if(unfold_operation_log) std::cout << "# remain: " << c.to_string() << "\n";
    //c自身もunfoldに含まれる
    unfolds.insert(c);
    if(unfold_operation_log) std::cout << " -> " << c.to_string() << "\n";
    int now=0;
    int count=0;
    int L=0;
    //theta-3はtheta-2かtheta-1になりうる
    if(c.periods.back()==2*(half_theta-1)-1){
        L=1;
        auto d=c;
        d.periods[(int)(d.periods.size())-1]=2*(half_theta-1);
        unfolds.insert(d);
        if(unfold_operation_log) std::cout << " -> " << d.to_string() << "\n";
        d.periods[(int)(d.periods.size())-1]=2*(half_theta-1)+1;
        unfolds.insert(d);
        if(unfold_operation_log) std::cout << " -> " << d.to_string() << "\n";
    }
    while(now<(int)(c.periods.size())){ //half_theta-1はunfoldによって2つの周期に置き換えられる。そのそれぞれの周期は2*(half_theta-1)または2*(half_theta-1)+1である。
        if(c.periods[now]==half_theta-1){
            count++;
            for(int i=now;i<(int)(c.periods.size())-1;i++){
                c.periods[i]=c.periods[i+1];
            }
            c.periods.pop_back();
            c.periods.push_back(2*(half_theta-1));
            c.periods.push_back(2*(half_theta-1));
            auto d=c;
            unfolds.insert(d);
            if(unfold_operation_log) std::cout << " -> " << d.to_string() << "\n";
            for(int i=1;i<=2*count;i++){
                d.periods[(int)(d.periods.size())-i]=2*(half_theta-1)+1;
                unfolds.insert(d);
                if(unfold_operation_log) std::cout << " -> " << d.to_string() << "\n";
            }
            if(L==1){
                auto e=c;
                e.periods[(int)(e.periods.size())-2*count-1]=2*(half_theta-1);
                unfolds.insert(e);
                if(unfold_operation_log) std::cout << " -> " << e.to_string() << "\n";
                for(int i=1;i<=2*count+1;i++){
                    e.periods[(int)(e.periods.size())-i]=2*(half_theta-1)+1;
                    unfolds.insert(e);
                    if(unfold_operation_log) std::cout << " -> " << e.to_string() << "\n";
                }
            }
        }
        else if(c.periods[now]<half_theta-1){
            now++;
        }
        else{
            break;
        }
    }
  }

  if(unfold_result_log) std::cout << " -> unfolded " << (int)(unfolds.size()) << " cases" << "\n";
  r_all = (int)(unfolds.size());
  r_check_one_count = 0;

  remain_schedules.clear();
  //unfolded list内の密度条件を満す全ての周期列を調査する

  std::vector<PinwheelInstance> sorted_list(unfolds.begin(),unfolds.end());
  std::sort(sorted_list.begin(), sorted_list.end(),
  [](auto const& A, auto const& B){
      if(A.periods.size()!=B.periods.size())
          return A.periods.size()<B.periods.size();
      return A.periods < B.periods;
  });

  for(auto c:sorted_list){
    r_check_one_count++;
    if(density_log){
      std::cout << "# " << c.to_string() << " " << modified_density(c,half_theta) << " " << modified_density(c,half_theta) - bound_on_modified_density << "\n";
    }
    if(modified_density(c,half_theta)<bound_on_modified_density){
        if (critical(known_schedules, impossible_schedules, remain_schedules, c, half_theta, bound_on_modified_density)){
            // ログ
            all_check_one_count++;
            sub_check_one_count++;
            if(all_check_one_count % all_check_one_count_log_interval == 0 ) std::cout << "all_check_one_count=" << all_check_one_count << ", R:[" << r_check_one_count << "/" << r_all << "], Impossible: " << (int)(impossible_schedules.size()) << ", pcm=" << possible_case_maxtime << ", checking: " << c.to_string() << "\n";
            auto t1 = std::chrono::steady_clock::now();
            long long cnt = (long long)(impossible_schedules.size());
            check_one<PackingPolicy>(c, known_schedules, impossible_schedules, remain_schedules);
            auto t2 = std::chrono::steady_clock::now();
            double sec = std::chrono::duration<double>(t2 - t1).count();
            if(sec>=0.1){
                std::cout << "Time: " << sec << " ";
                if((long long)(impossible_schedules.size()) - cnt == 1){
                    std::cout << "Impossible";
                }
                else{
                    std::cout << "Possible";
                    possible_case_maxtime=std::max(possible_case_maxtime,sec);
                }
                std::cout << " " << c.to_string() << "\n";
            }
        }
    }
  }
}

void solve (std::unordered_map<PinwheelInstance, Schedule>& known_schedules, std::unordered_set<PinwheelInstance>& impossible_schedules, std::vector<PinwheelInstance>& remain_schedules) {
  //theta = min_half_theta, 14, ... , max_theta
  for (unsigned int half_theta = min_half_theta; half_theta <= max_half_theta; half_theta++) {
    sub_skip_count = 0;
    sub_check_one_count = 0;

    rational bound_on_modified_density = M + rational(1, 2 * half_theta);
    if (half_theta == min_half_theta) { //theta = min_half_theta については密度条件を満たす全ての周期列を調査
        check_all(PinwheelInstance(), known_schedules, impossible_schedules, remain_schedules, half_theta, bound_on_modified_density);
    }
    else { //theta > min_half_theta については、unfold_{theta - 2}(remain_schedules)に含まれる全ての周期列を調査
        check_unfold (known_schedules, impossible_schedules, remain_schedules, half_theta, bound_on_modified_density);
    }

    //調査ケース数, #L, #R
    std::cout << "# theta=" << 2*half_theta << ", all:" << sub_check_one_count << ", L:" << sub_check_one_count - sub_skip_count << ", R:" << sub_skip_count << "\n";
  }
}

// main(): これを実行して REMAIN が表示されないことを以て、補題が確かめられる。根拠となる周期列と日割の組を表示する。
int main (void) {
  //開始通知
  auto time_start = std::chrono::steady_clock::now();
  std::cout << "started" << std::endl;
  //ファイル出力の準備
  std::ofstream outputfile("result.txt");

  ////////////////////////////////////////////////////////////////////////////////
  //実際の処理
  //これによりremain_schedulesがemptyであれば、R_max_theta.empty()が言える
  std::unordered_map<PinwheelInstance, Schedule> known_schedules{};
  std::unordered_set<PinwheelInstance> impossible_schedules{};
  std::vector<PinwheelInstance> remain_schedules; //R_theta
  solve(known_schedules, impossible_schedules, remain_schedules);
  ////////////////////////////////////////////////////////////////////////////////

  //周期列と日割の組をファイルへ出力
  for (const auto& [c, s] : known_schedules) outputfile << c.to_string() << ": " << s.to_string() << std::endl;
  //remainsをファイル・ターミナルへ出力
  for (auto c : remain_schedules) {
    std::cout << "REMAIN: " << c.to_string() << "\n";
    outputfile << "REMAIN: " << c.to_string() << "\n";
  }

  //実行時間
  auto time_end = std::chrono::steady_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
  long long total_sec = diff / 1000;
  long long hours   = total_sec / 3600;
  long long minutes = (total_sec % 3600) / 60;
  long long seconds = total_sec % 60;
  std::cout << "\n" << hours << "h " << minutes << "m " << seconds << "s\n";
  outputfile << hours << "h " << minutes << "m " << seconds << "s\n";

  //ファイル出力の終了
  outputfile.close();
  return 0;
}
