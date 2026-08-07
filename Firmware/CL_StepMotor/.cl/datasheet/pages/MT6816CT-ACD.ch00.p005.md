# MT6816CT-ACD — 第0章 · p5

MT6816
高分辨率、高速磁性角度编码器IC
www.magntek.com.cn | 磁技术带来美妙变革
版本2.1    2022.12
5
3.  芯片功能框图
图-2: 芯片功能框图
       MT6816是一颗基于先进的AMR磁感应技术和先进的信号处理技术的角度传感器芯片，它能够感
应平行于芯片表面的磁场方向变化并输出相应的角度值。集成于芯片中心的磁感应元件检测磁场方向
变化并输出电压值。
       如图-2所示，芯片内集成的两对AMR惠斯通电桥会随着外加磁场的方向变化，输出两路正余弦
模拟电压信号；信号经过模拟前端电路的放大（G）和滤波后，被送入模数转换器（ADC）；被放大
并经数字量化的正余弦信号最终进入数字信号处理器（DSP）进行补偿、校准和求解角度的运算；计
算出绝对角度后，输出解析模块会将此绝对角度解析为PWM、ABZ、UVW、SPI等各种输出形式，
MT6816所有输出形式的源头都是这一绝对角度。另外系统中还包含了低压差稳压模块（LDO）、时
钟振荡器（OSC）、非遗失性寄存器等模块。
       MT6816芯片包括两路电源VDD和HVPP。其中VDD用于给芯片供电，经过内部LDO稳压后给
AMR元件和内部专用计算电路供电；而HVPP是在需要烧录MTP时提供7V的烧录电压，同时做为SPI
通信和ABZ选择引脚，SPI通信时接高电平，其余芯片工作时间接地。
G
G
ADC
ADC
DSP
LDO
Calibration
NVM
ABZ
/-A-B-Z
PWM
VDD
HVPP
A
B
Z
OUT
CSN
M
U
X
OSC
SPI 
UVW
VSS
Angle 
Calculation
Interpolator
Magnetic 
Sensing 
Element
I/V 
REF


<!-- detected tables -->

|  | MT6816 高分辨率、高速磁性角度编码器IC 3. 芯片功能框图 MT6816是一颗基于先进的AMR磁感应技术和先进的信号处理技术的角度传感器芯片，它能够感 应平行于芯片表面的磁场方向变化并输出相应的角度值。集成于芯片中心的磁感应元件检测磁场方向 变化并输出电压值。 VDD HVPP DSP LDO NVM PWM OUT Angle Calculation Magnetic G ADC Sensing Element ABZ /-A-B-Z Calibration G ADC M A UVW U B X Z Interpolator I/V OSC SPI REF VSS CSN 图-2: 芯片功能框图 如图-2所示，芯片内集成的两对AMR惠斯通电桥会随着外加磁场的方向变化，输出两路正余弦 模拟电压信号；信号经过模拟前端电路的放大（G）和滤波后，被送入模数转换器（ADC）；被放大 并经数字量化的正余弦信号最终进入数字信号处理器（DSP）进行补偿、校准和求解角度的运算；计 算出绝对角度后，输出解析模块会将此绝对角度解析为PWM、ABZ、UVW、SPI等各种输出形式， MT6816所有输出形式的源头都是这一绝对角度。另外系统中还包含了低压差稳压模块（LDO）、时 钟振荡器（OSC）、非遗失性寄存器等模块。 MT6816芯片包括两路电源VDD和HVPP。其中VDD用于给芯片供电，经过内部LDO稳压后给 AMR元件和内部专用计算电路供电；而HVPP是在需要烧录MTP时提供7V的烧录电压，同时做为SPI 通信和ABZ选择引脚，SPI通信时接高电平，其余芯片工作时间接地。 |
|---|---|
|  | www.magntek.com.cn \| 磁技术带来美妙变革 5 版本2.1 2022.12 |
