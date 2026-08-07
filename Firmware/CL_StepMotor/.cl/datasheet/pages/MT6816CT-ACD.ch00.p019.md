# MT6816CT-ACD — 第0章 · p19

MT6816
高分辨率、高速磁性角度编码器IC
www.magntek.com.cn | 磁技术带来美妙变革
版本2.1    2022.12
19
       MT6816提供了4线或者3线（‘SPI_Mode’寄存器置高来开启3线模式）SPI输出，用于上位
机读取芯片内部角度信息。
图-14 ：4线SPI参考电路                                                      图-15：3线SPI参考电路
8.6  SPI接口
8.6.1  SPI参考电路
       SPI参考电路如图-14和图-15所示。
1
2
3
4
8
7
6
5
VDD
TVS(6V)
0.1uf
Host MCU
S C K
M O S I
M IS O
C S N
NC
1
2
3
4
8
7
6
5
VDD
TVS(6V)
0.1uf
Host MCU
S C K
S D A T
C S N
NC
N C
寄存器：SPI_Mode
SPI 接口
0
4线Mode
1
3线Mode
SPI模式寄存器(OTP)


<!-- detected tables -->

|  | MT6816 高分辨率、高速磁性角度编码器IC 8.6 SPI接口 MT6816提供了4线或者3线（‘SPI Mode’寄存器置高来开启3线模式）SPI输出，用于上位 _ 机读取芯片内部角度信息。 8.6.1 SPI参考电路 SPI参考电路如图-14和图-15所示。 Host MCU Host MCU C S N M O S I C S N S D A T M IS O S C K S C K 1 8 1 8 2 7 2 7 NC 3 6 NC 3 6 N C VDD 4 5 VDD 4 5 0.1uf TVS(6V) 0.1uf TVS(6V) 图-14 ：4线SPI参考电路 图-15：3线SPI参考电路 SPI模式寄存器(OTP) 寄存器：SPI Mode SPI 接口 _ 0 4线Mode 1 3线Mode |
|---|---|
|  | www.magntek.com.cn \| 磁技术带来美妙变革 19 版本2.1 2022.12 |

|  | 1 |
|---|---|

| 8 |  |
|---|---|

|  | 1 |
|---|---|

| 8 |  |
|---|---|

|  | 2 |
|---|---|

|  | 2 |
|---|---|

| 7 |  |
|---|---|

|  | 3 |
|---|---|

|  | 3 |
|---|---|

| 6 |  |
|---|---|

|  | 4 |
|---|---|

|  | 4 |
|---|---|

| 5 |  |
|---|---|

| 寄存器：SPI Mode _ | SPI 接口 |
|---|---|
| 0 | 4线Mode |
| 1 | 3线Mode |
