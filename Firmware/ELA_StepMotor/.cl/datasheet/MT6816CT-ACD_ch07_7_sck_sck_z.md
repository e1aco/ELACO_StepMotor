# MT6816CT-ACD — 第7章 7 SCK SCK Z W

--- [PAGE 10] ---
MT6816
高分辨率、高速磁性角度编码器IC
www.magntek.com.cn | 磁技术带来美妙变革 版本2.1    2022.1210
8.  输出模式
       MT6816可以输出ABZ、UVW和PWM信号，另外还可以通过4线或3线SPI接口读取14位的绝对
角度寄存器。其中ABZ、UVW和SPI接口是互相复用I/O引脚的。SPI接口和ABZ/UVW之间是通过
HVPP引脚进行配置的，当HVPP接高电平VDD时，相关I/O管脚切换至SPI模式；当HVPP接地时，芯
片相关I/O切换至ABZ或UVW模式。ABZ和UVW模式的切换，由芯片内部相关寄存器控制。4线SPI和
3线SPI也是通过芯片内部寄存器进行切换控制的，MT6816出厂默认配置为4线SPI。
8.1  I/O引脚功能配置
管脚 3线 SPI 4线 SPI ABZ+PWM UVW+PWM
1 CSN CSN - -
3 PWM PWM PWM PWM
5 SDAT MOSI A U
6 - MISO B V
7 SCK SCK Z W
I/O引脚配置表
    MT6816提供的ABZ、UVW、PWM以及SPI接口的引脚配置如下表。
