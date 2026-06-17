# 輪番割当問題（詰込型・被覆型）

輪番割当（Pinwheel Scheduling）問題における周期列の詰込割当可能性（packing schedulability）と被覆割当可能性（covering schedulability）を判定するソルバーです。密度予想の検証を目的として開発されました。

## 📁 ディレクトリ構成

```text
.
├── include/
│   └── pinwheel/
│       ├── types.hpp      # 有理数型（Boost）や周期型の定義
│       ├── policies.hpp   # 詰込/被覆の各探索ポリシー
│       └── solver.hpp     # 探索コアロジック
├── apps/
│   ├── verify_packing_lemma.cpp  # 詰込に関する補題の検証プログラム
│   └── verify_covering_lemma.cpp # 被覆に関する補題の検証プログラム
├── CMakeLists.txt
└── README.md
```

## 🛠️ ビルドと実行方法
必要な環境
- C\+\+20に対応したコンパイラ（g++ 11.4 以上、または g++ 13 以上を推奨）
- CMake（3.20以上）
- Boost ライブラリ（Multiprecision, Rational）
- OpenMP（並列処理用）

### 手順
ターミナルで以下を実行します。

```bash
# 1. ビルドディレクトリの作成と移動
mkdir build && cd build

# 2. CMakeの実行
cmake ..

# 3. コンパイル
make

# 4. 実行（詰込補題の検証の場合）
./packing
```
