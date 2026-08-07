# MT6816CT-ACD — 第0章 · p16

MT6816
高分辨率、高速磁性角度编码器IC
www.magntek.com.cn | 磁技术带来美妙变革
版本2.1    2022.12
16
     Z信号的宽度由一个3比特位宽的寄存器‘Z_PUL_WID[2:0]’来定义。
Z信号宽度寄存器‘Z_PUL_WID[2:0]’(MTP)
Z信号角度位置寄存器(MTP)
ABZ 分辨率寄存器(MTP)
     Z信号的绝对角度位置由一个12比特位宽的寄存器‘ZERO_POS[11:0]’;
寄存器
位7
位6
位5
位4
位3
位2
位1
位0
ABZ_RES
NA
NA
NA
NA
NA
NA
ABZ_RES<9:8>
ABZ_RES
ABZ_RES<7:0>
    ABZ输出分辨率由一个10比特位宽的寄存器‘ABZ_RES[9:0]’来定义;
寄存器：
Z_Pulse_Width<2:0>
宽度 (LSBs或度)
寄存器：
Z_Pulse_Width<2:0>
宽度 (LSBs或度)
000
1
100
12
001
2
101
16
010
4
110
180°
011
8
111
1
寄存器
bit7
bit6
bit5
bit4
bit3
bit2
bit1
bit0
Zero_MSB
NA
NA
NA
NA
Zero<11:8>
Zero_LSB
Zero<7:0>


<!-- detected tables -->

|  | MT6816 高分辨率、高速磁性角度编码器IC Z信号的宽度由一个3比特位宽的寄存器‘Z PUL WID[2:0]’来定义。 _ _ Z信号宽度寄存器‘ZPULWID[2:0]’(MTP) _ _ 寄存器： 寄存器： 宽度 (LSBs或度) 宽度 (LSBs或度) Z Pulse Width<2:0> Z Pulse Width<2:0> _ _ _ _ 000 1 100 12 001 2 101 16 010 4 110 180° 011 8 111 1 Z信号的绝对角度位置由一个12比特位宽的寄存器‘ZERO POS[11:0]’; _ Z信号角度位置寄存器(MTP) 寄存器 bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0 Zero MSB NA NA NA NA Zero<11:8> _ Zero LSB Zero<7:0> _ ABZ输出分辨率由一个10比特位宽的寄存器‘ABZ RES[9:0]’来定义; _ ABZ 分辨率寄存器(MTP) 寄存器 位7 位6 位5 位4 位3 位2 位1 位0 ABZ RES NA NA NA NA NA NA ABZ RES<9:8> _ _ ABZ RES ABZ RES<7:0> _ _ |
|---|---|
|  | www.magntek.com.cn \| 磁技术带来美妙变革 16 版本2.1 2022.12 |

| 寄存器： Z Pulse Width<2:0> _ _ | 宽度 (LSBs或度) | 寄存器： Z Pulse Width<2:0> _ _ | 宽度 (LSBs或度) |
|---|---|---|---|
| 000 | 1 | 100 | 12 |
| 001 | 2 | 101 | 16 |
| 010 | 4 | 110 | 180° |
| 011 | 8 | 111 | 1 |

| 寄存器 | bit7 | bit6 | bit5 | bit4 | bit3 | bit2 | bit1 | bit0 |
|---|---|---|---|---|---|---|---|---|
| Zero MSB _ | NA | NA | NA | NA | Zero<11:8> |  |  |  |
| Zero LSB _ | Zero<7:0> |  |  |  |  |  |  |  |

| 寄存器 | 位7 | 位6 | 位5 | 位4 | 位3 | 位2 | 位1 | 位0 |
|---|---|---|---|---|---|---|---|---|
| ABZ RES _ | NA | NA | NA | NA | NA | NA | ABZ RES<9:8> _ |  |
| ABZ RES _ | ABZ RES<7:0> _ |  |  |  |  |  |  |  |
