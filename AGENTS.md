# AGENTS.md — Yet Another Sign Up (Yasu) 开发指南

本文件面向在此仓库中工作的 AI 编码代理。内容描述项目结构、构建/测试方式、
代码约定与常见改动流程。改动代码前请先读完本文件。

## 1. 项目概览

班级签到系统（GPLv3）。教师端输入学号签到，日志按天写入文本文件；程序按
早/中/晚三个时段统计签到、迟到、未签到，并可导出 Excel（含导出到 Rewardly
加分表）。

- 语言/标准：C++17
- GUI：Qt 6 或 Qt 5（只依赖 Widgets、Network、Test）
- 构建：CMake ≥ 3.16；QXlsx 与 tomlplusplus 通过 FetchContent 拉取
- 配置：config.toml（toml++ 读写）
- 导出：QXlsx
- 界面文案：全部为简体中文（`QString("中文")`）

## 2. 构建与测试

```bash
cmake -B build          # 首次配置需要网络（FetchContent 拉取第三方库）
cmake --build build -j
cd build && ctest --output-on-failure
```

- 构建目录为 `build/`（Ninja + Debug），已被 `.gitignore` 忽略；第三方源码在
  `build/_deps/`，已配置过就不需要网络，增量构建即可。
- 测试无界面运行：`tests/CMakeLists.txt` 已为每个用例设置
  `QT_QPA_PLATFORM=offscreen` 且 `RUN_SERIAL TRUE`。
- 只跑单个用例：`cd build && QT_QPA_PLATFORM=offscreen ./tests/test_rewardly`
- 运行程序：`./build/yet-another-sign-up`（首次运行会在可执行文件目录生成
  `config.toml`、`logs/`、`data/`）。

## 3. 文件地图

| 文件 | 职责 |
| --- | --- |
| `main.cpp` | 入口；用 QLocalServer/QLocalSocket 做单实例保护 |
| `mainwindow.*` | 主窗口：学号输入、屏幕数字键盘、公告栏、菜单 |
| `signup.*` | 签到核心逻辑（Yasu 类）：学号解析、重复签到、写日志 |
| `configmanager.*` | config.toml 读写、默认值、旧格式迁移、例外合并与校验 |
| `signlogparser.*` | 共享日志解析：时段判定、状态判定、可选套用按日期例外 |
| `adminpanel.*` | 管理页面；用代码动态添加各功能按钮 |
| `arbiter.*` | 「查看数据」窗口：按时间分时段统计；同时定义全局量 |
| `exportdialog.*` | 「导出数据」：前 10 名 + 迟到 + 未签到 → Excel |
| `rewardlyexportdialog.*` | 「导出到 Rewardly」：加分/扣分行，勾选后导出 Excel |
| `rewardlyconfigdialog.*` | 「配置加分项」（`[rewardly]` 段） |
| `deadlinedialog.*` | 「设置迟到时间」（`[periods]` 段） |
| `exceptiondialog.*` | 「设置例外」：按日期临时覆盖某时段的分界/迟到时间（仅内存） |
| `tests/` | Qt Test 用例，覆盖非 UI 逻辑与各窗口的关键交互 |

全局量：`std::vector<QString> extstunames`（下标 = 学号，0 为占位）与
`std::map<QString,int> nti`（姓名 → 学号）在 `arbiter.cpp` 定义。任何链接到
`arbiter.cpp` 之外使用它们的代码（含测试）都必须自行提供定义。

## 4. 数据与配置

所有路径都以 `QCoreApplication::applicationDirPath()` 为基准（不是 CWD），
所以测试会在测试可执行文件所在目录读写 `config.toml`，测试的
`init()/cleanup()` 必须清理这些文件。

```
<程序目录>/
├── names                  # 学生名单：每行一个姓名，第 1 行 = 学号 1
├── config.toml            # 配置（缺失时自动创建默认值）
├── logs/yyyy-MM-dd.log    # 每日签到日志
└── data/yyyy-MM-dd.data   # 每日数据标记文件
```

日志行格式：`yyyy.MM.dd hh:mm:ss, 姓名, Signed|Resigned`；`-`/`=` 开头的是
创建/重启标记，解析时跳过。不要修改日志格式，它决定了历史数据的可读性。

config.toml 结构：

```toml
[namelist]      directory = 'names'
[periods]       morning_deadline / morning_noon_split / noon_deadline /
                noon_evening_split / evening_deadline      # 'HH:mm'
[rewardly]      morning_sign_bonus_count / morning_sign_bonus_score /
                *_late_deduction / *_absent_deduction       # 加分正、扣分负
```

例外（设置例外）**不在 config.toml 中**：它是「导出到 Rewardly」窗口持有的
一次性内存数据，见第 5 节。

## 5. 关键领域规则

- 时段划分（`signlogparser.cpp`）：按签到时间判定，
  `time <= morning_noon_split` → 早上；`<= noon_evening_split` → 中午；否则晚上。
  边界时刻归前一个时段（`<=`）。
- 状态判定：`time <= deadline` → `签到`，否则 `迟到`。
- 例外（设置例外）：`split` 覆盖该时段结束的分界（早上 → `morning_noon_split`，
  中午 → `noon_evening_split`），`deadline` 覆盖该时段的截止时间。晚上是当天
  最后一个时段，没有后续分界，其 `split` 无意义（界面禁用，也不存入列表）。
  例外是**一次性内存数据**：只由「导出到 Rewardly」窗口持有，不写 config.toml，
  关闭窗口即失效；因此 `ConfigManager::forDate(config, date, exceptions)` 与
  `parseSignLogFile*(path, date, config, exceptions)` 都要求调用方**显式传入**列表
  （默认空列表 = 不套用）。查看数据/导出数据窗口不传例外，仍按全局配置统计。
- 早上加分只给早上时段 `签到` 的前 N 名（`morning_sign_bonus_count`）。
- 未签到 = 该时段内没有任何记录；迟到也算"已签到"，不重复计未签到。
- 分数为小数（2 位），正数加分、负数扣分，Excel 单元格用 `Format 0.00`。

## 6. 代码约定

1. 许可证头：每个源文件顶部保留 GPLv3 注释块（复制现有文件即可）。
2. 配置项改动三步走：`ConfigData` 加字段 → `defaultConfig()` 给默认值 →
   `loadConfig()`/`saveConfig()` 读写；随后补 `tests/test_config.cpp` 的用例。
   读取缺失键时保持默认值（旧配置文件必须仍可加载）。
3. 窗口：新建窗口继承 `QWidget`，用 `setWindowTitle(QString("中文"))` +
   `setFixedSize(...)` + 布局（QVBoxLayout/QFormLayout）纯代码搭建；打开方式与
   `adminpanel.cpp` 一致：

   ```cpp
   auto *dlg = new XxxDialog;
   dlg->setAttribute(Qt::WA_DeleteOnClose);
   dlg->setWindowFlag(Qt::WindowStaysOnTopHint);   // 管理页内可选
   dlg->show();
   ```

   ⚠️ 打开窗口时**不要传父对象**（`new XxxDialog(this)` 是错的）：带父对象的
   `QWidget` 会变成父窗口的子控件、绘制在父窗口内容之上，而不是新窗口。

   持久化配置的窗口（如 `deadlinedialog.*`、`rewardlyconfigdialog.*`）用
   「保存 + 关闭」，保存成功用 `QMessageBox::information` 提示；一次性设置的
   窗口（如 `exceptiondialog.*`）用「确定 / 取消」，由「确定」发出信号把结果
   交回调用方（不要写 config.toml）。
4. 可测试性：把纯逻辑写成 static 公有函数（参考
   `RewardlyExportDialog::buildRows`），UI 只做取值/展示，这样测试不需要弹窗。
5. 测试：`tests/CMakeLists.txt` 中每个用例是一个可执行目标，链接它所需的 .cpp
   （例如 `test_rewardly` 同时链接 `exceptiondialog.cpp`）。测试文件末尾用
   `QTEST_MAIN`（需要 Widgets）或 `QTEST_GUILESS_MAIN`（纯逻辑）。仅测试 UI
   内部状态时才使用仓库既有的 `#define private public` 技巧（需先包含相关 Qt
   头文件），并且避免触发模态 `QMessageBox`，否则 offscreen 下会阻塞
   （只测不会弹窗的路径）。
6. 新增源文件：加入 `CMakeLists.txt` 的 `PROJECT_SOURCES`；若被某个测试目标链接
   的 .cpp 引用，也要加入 `tests/CMakeLists.txt` 对应目标，否则会链接失败。
7. 提交信息：Conventional Commits，英文类型 + 简短描述，
   如 `feat: add per-date period exceptions`、`docs:`、`fix:`、`chore:`。

## 7. 常见任务

- 加一项配置：见「代码约定 2」；若与时段/加分有关，同步更新 README 的配置示例。
- 加一个管理功能：在 `adminpanel.cpp` 中仿照现有按钮新增
  `new QPushButton(QString("中文名"), this)` + `setGeometry(40, y, w, 28)`
  （y 每行 +40）并连接 slot；或在具体窗口内加按钮（如
  `rewardlyexportdialog.cpp` 的「设置例外」）。
- 改时段/迟到判定：只改 `signlogparser.*`，并同步 `ConfigManager::forDate()`
  与 `validateExceptions()`（两者都接收显式的例外列表）及测试；不要在 UI 里
  复制一份判定逻辑。

## 8. 已知坑

- 路径都基于 `applicationDirPath()`：测试与程序运行目录不同会读到不同的
  config.toml，务必让测试自己清理。
- `parseSignLogFileAll()` 的 `date` 参数决定"哪一天的例外生效"；直接调用
  `determineSignPeriod`/`determineSignStatus` 时不会自动套用例外。
- 例外不落盘：它必须由调用方（目前是「导出到 Rewardly」窗口）以
  `QVector<PeriodException>` 显式传给 `parseSignLogFile*()`；config.toml 中出现
  `[[exceptions]]` 会被忽略，也不要在其中写入例外。
- QTime 比较用 `<=`：分界时刻归属前一时段，写测试时注意边界。
- `main.cpp` 的单实例锁（QLocalServer 名字 `Hanatomizu_Yetanothersignup`）会阻止
  同时运行两个实例；做手动 UI 验证前确认已关闭旧实例。
