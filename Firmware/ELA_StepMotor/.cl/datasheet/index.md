# .cl/datasheet 索引（导航 → 按需拉取，禁止整读超大文件）

> 提取规则见 `details/init.md` §4.2：先读本索引定位，再按需 Read 目标章节/页文件。
> 提取引擎：pypdf（technical）。所有产物 ≤ ~4k tokens。

## 数据手册清单

| 手册 | 芯片 | 页数 | ~tokens | 拆分模式 |
|------|------|------|---------|----------|
| MT6816CT-ACD | 编码器 (SPI) | 30 | ~5.9k | 章节(标题检测) |
| TB67H450FNG | 电机驱动 H 桥 | 22 | ~7.7k | 逐页 |

## MT6816CT-ACD（编码器，SPI 通信）

| 章节 | 页 | 文件 | ~tok |
|------|----|------|------|
| 前部（特性/引脚/SPI 概述） | 1–9 | `MT6816CT-ACD_ch00_front.md` | 2676 |
| 第 5 章 A/U | 11–17 | `MT6816CT-ACD_ch05_5_a_u.md` | 921 |
| 第 7 章 SCK/SCK/Z/W | 10 | `MT6816CT-ACD_ch07_7_sck_sck_z.md` | 194 |
| 第 8 章 Clock（时钟） | 18–30 | `MT6816CT-ACD_ch08_8_clock.md` | 2104 |

## TB67H450FNG（步进电机 H 桥驱动）

> 22 页均拆为逐页文件（ch00）：`pages/TB67H450FNG.ch00.pNNN.md`
> p001–p022，关键页：p001 概述、p003-p009 功能框图/时序、p019-p020 特性、p022 引脚/应用。

| 页 | 文件 | ~tok |
|----|------|------|
| 1 | `pages/TB67H450FNG.ch00.p001.md` | 338 |
| 2 | `pages/TB67H450FNG.ch00.p002.md` | 29 |
| 3 | `pages/TB67H450FNG.ch00.p003.md` | 336 |
| 4 | `pages/TB67H450FNG.ch00.p004.md` | 83 |
| 5 | `pages/TB67H450FNG.ch00.p005.md` | 140 |
| 6 | `pages/TB67H450FNG.ch00.p006.md` | 333 |
| 7 | `pages/TB67H450FNG.ch00.p007.md` | 268 |
| 8 | `pages/TB67H450FNG.ch00.p008.md` | 213 |
| 9 | `pages/TB67H450FNG.ch00.p009.md` | 372 |
| 10 | `pages/TB67H450FNG.ch00.p010.md` | 179 |
| 11 | `pages/TB67H450FNG.ch00.p011.md` | 705 |
| 12 | `pages/TB67H450FNG.ch00.p012.md` | 421 |
| 13 | `pages/TB67H450FNG.ch00.p013.md` | 591 |
| 14 | `pages/TB67H450FNG.ch00.p014.md` | 376 |
| 15 | `pages/TB67H450FNG.ch00.p015.md` | 235 |
| 16 | `pages/TB67H450FNG.ch00.p016.md` | 81 |
| 17 | `pages/TB67H450FNG.ch00.p017.md` | 30 |
| 18 | `pages/TB67H450FNG.ch00.p018.md` | 25 |
| 19 | `pages/TB67H450FNG.ch00.p019.md` | 878 |
| 20 | `pages/TB67H450FNG.ch00.p020.md` | 549 |
| 21 | `pages/TB67H450FNG.ch00.p021.md` | 59 |
| 22 | `pages/TB67H450FNG.ch00.p022.md` | 1459 |
