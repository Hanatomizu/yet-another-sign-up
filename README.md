# Yet Another Sign Up System（Yasu）

一套用于班级管理的签到系统，支持每日早/中/晚三个时段的签到登记、数据统计与 Excel 导出。

- 许可证：GPL v3

## 功能特性

- **学号签到**：主窗口输入学号（支持屏幕数字键盘 / 物理键盘回车）完成签到，签到结果实时显示在公告栏。
- **单实例运行**：程序同时只允许运行一个实例，重复启动会给出提示。
- **防误触退出**：直接点击窗口关闭按钮会被拦截，需通过「操作 → 退出程序」正常退出。
- **三个签到时段**：早上 / 中午 / 晚上，由配置中的分界时间按**签到时间**划分。
- **查看数据**：选择日期查看当日各时段的签到记录、未签到名单、重复签到名单。
- **导出数据**：选择日期范围，导出每日早上签到前 10 名 + 全部迟到 / 未签到记录为 Excel。
- **导出到 [Rewardly](https://github.com/Hanatomizu/rewardly)**：选择日期范围，按加分规则生成加分数据（早上签到前 N 名加分、早/中/晚迟到与未签到扣分），逐条勾选后导出 Excel。
- **灵活配置**：签到截止时间、时段分界、加分/扣分规则均可通过界面修改并保存到 `config.toml`。

## 界面说明

### 主窗口

- 学号输入框 + 屏幕数字键盘（0-9、退格、清空）+「签到」按钮
- 公告栏：显示签到结果与最近记录
- 「管理页面」按钮：打开管理面板
- 菜单栏：「管理页面」「查看数据」「关于」「退出程序」等

### 管理页面

| 按钮 | 功能 |
| --- | --- |
| 查看统计数据 | 打开「查看数据」窗口（同主窗口菜单） |
| 导出数据 | 打开「导出数据」窗口 |
| 设置迟到时间 | 打开「设置迟到时间」窗口 |
| 导出到 Rewardly | 打开「导出到 Rewardly」窗口 |
| 配置加分项 | 打开「配置加分项」窗口 |
| 关闭 | 关闭管理页面 |

### 查看数据

选择日期后点击「查看数据」，按三个时段（早上/中午/晚上）分别展示：

- 签到记录（时间 + 姓名，同一时段内重复出现记入「重复签到」）
- 未签到名单
- 重复签到名单

时段划分使用 `config.toml` 中的 `morning_noon_split` / `noon_evening_split` 分界时间。

### 导出数据

选择开始/结束日期，导出内容：

- 每日早上签到前 10 名 → 类型「签到」
- 全部时段迟到记录 → 类型「迟到」
- 每个时段未签到学生 → 类型「未签到」（一人缺席多个时段则生成多行）

Excel 列：`姓名 | 日期（含早上/中午/晚上）| 类型`

### 导出到 Rewardly

选择开始/结束日期，下方表格自动生成待导出数据（每行左侧复选框，默认全选）：

- 早上签到前 N 名 → 加分（原因「早上签到」）
- 早/中/晚迟到 → 扣分（原因「早上迟到/中午迟到/晚上迟到」）
- 早/中/晚未签到 → 扣分（原因「早上未签到/中午未签到/晚上未签到」）

勾选需要导出的行后点击「保存」，仅将选中行写入 Excel：

Excel 列：`姓名 | 日期（含早上/中午/晚上）| 加分 | 原因`

加分/扣分为**小数**（精确到小数点后两位），**正数代表加分，负数代表扣分**。

### 设置迟到时间

配置各时段签到截止时间与早/午、午/晚分界时间，保存到 `config.toml` 的 `[periods]` 段。

### 配置加分项

配置 8 项加分规则，保存到 `config.toml` 的 `[rewardly]` 段：

| 配置项 | 含义 |
| --- | --- |
| 早上签到加分人数 | 每天早上按时签到可获得加分的人数（前 N 名） |
| 早上签到加分分数 | 早上签到加分分数（正数） |
| 早上/中午/晚上迟到扣分分数 | 各时段迟到扣分（负数） |
| 早上/中午/晚上未签到扣分分数 | 各时段未签到扣分（负数） |

## 数据与日志

程序在**可执行文件所在目录**下维护数据：

```
<程序目录>/
├── names                 # 学生名单：每行一个姓名（第 1 行对应学号 1，依此类推）
├── config.toml           # 配置文件（首次运行自动创建默认值）
├── logs/
│   └── yyyy-MM-dd.log    # 每日签到日志
└── data/
    └── yyyy-MM-dd.data   # 每日数据文件（创建标记）
```

日志格式（每行一条记录，使用英文逗号 + 空格分隔）：

```
yyyy.MM.dd hh:mm:ss, 姓名, Signed     # 签到成功
yyyy.MM.dd hh:mm:ss, 姓名, Resigned   # 重复签到尝试
- yyyy-MM-dd yasu created this file   # 文件创建标记
= yyyy-MM-dd yasu rechecked this file # 程序重启标记（仅存档，不再用于时段划分）
```

## 配置文件

`config.toml` 在程序目录下，缺失时自动以默认值创建，完整示例：

```toml
[namelist]
directory = 'names'

[periods]
morning_deadline = '09:00'      # 早上签到截止
morning_noon_split = '12:00'    # 早/午分界
noon_deadline = '12:30'         # 中午签到截止
noon_evening_split = '17:00'    # 午/晚分界
evening_deadline = '18:00'      # 晚上签到截止

[rewardly]
morning_sign_bonus_count = 10   # 早上签到加分人数
morning_sign_bonus_score = 2.0  # 早上签到加分分数（正数加分）
morning_late_deduction = -1.0   # 早上迟到扣分分数（负数扣分）
morning_absent_deduction = -2.0 # 早上未签到扣分分数
noon_late_deduction = -1.0      # 中午迟到扣分分数
noon_absent_deduction = -2.0    # 中午未签到扣分分数
evening_late_deduction = -1.0   # 晚上迟到扣分分数
evening_absent_deduction = -2.0 # 晚上未签到扣分分数
```

> 旧版无 `[rewardly]` 段的配置文件仍可正常加载，缺失项自动使用默认值。

## 构建

依赖：CMake ≥ 3.16、C++17 编译器、Qt 6 或 Qt 5（Widgets）。`QXlsx` 与 `tomlplusplus` 通过 CMake `FetchContent` 自动拉取（首次配置需要网络）。

```bash
cmake -B build
cmake --build build -j
```

运行：

```bash
./build/yet-another-sign-up
```

> 首次运行会在程序目录生成默认 `config.toml`、`logs/`、`data/`；请确保程序目录有写入权限，并在程序目录放置 `names` 学生名单文件。

## 测试

测试基于 Qt Test，无界面运行（`QT_QPA_PLATFORM=offscreen`）：

```bash
cmake --build build -j
cd build && ctest --output-on-failure
```

| 测试目标 | 覆盖内容 |
| --- | --- |
| test_config | 配置读写、默认值、旧格式迁移 |
| test_signup | 签到核心逻辑（学号解析、重复签到） |
| test_export | 日志解析、时段/状态判定、导出 Excel 内容 |
| test_rewardly | Rewardly 加分数据生成（加分/扣分/未签到规则） |
| test_arbiter | 查看数据窗口的按时间分时段统计 |

## 项目结构

```
├── main.cpp                 # 入口（单实例保护）
├── mainwindow.*             # 主窗口
├── signup.*                 # 签到核心逻辑与日志写入
├── configmanager.*          # config.toml 读写
├── signlogparser.*          # 共享日志解析（时段/状态判定）
├── adminpanel.*             # 管理页面
├── arbiter.*                # 查看数据（按时间分时段统计）
├── exportdialog.*           # 导出数据（Excel）
├── rewardlyexportdialog.*   # 导出到 Rewardly（Excel）
├── rewardlyconfigdialog.*   # 配置加分项
├── deadlinedialog.*         # 设置迟到时间
└── tests/                   # Qt Test 单元测试
```

## 许可证

```
Yet Another Sign Up - A new sign up system for class managements
Copyright (C) 2025  知念夏世 <chart11from21@outlook.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

## 致谢

- [Qt](https://www.qt.io)
- [QXlsx](https://github.com/QtExcel/QXlsx)
- [toml++](https://github.com/marzer/tomlplusplus)
