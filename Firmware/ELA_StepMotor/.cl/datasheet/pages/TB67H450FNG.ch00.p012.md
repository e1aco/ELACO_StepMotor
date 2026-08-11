# TB67H450FNG — 第0章 · p12

TB67H450FNG 
2020-11-26 
 
12 
 
Electrical Characteristics 1 (Ta=25°C, VM=24 V unless otherwise specified) 
Characteristics Symbol Test conditions Min Typ. Max Unit 
Logic input pin 
Input voltage 
HIGH VIN(H) Logic input pins 2.0 ― 5.5 V 
LOW VIN(L) Logic input pins 0 ― 0.8 V 
Input hysteresis VIN(HYS) Logic input pins (Note 1) 100 ― 300 mV 
Logic input pin 
Input current 
HIGH IIN(H) Test logic input pins: 3.3 V ― 33 55 μA 
LOW IIN(L) Test logic input pins: 0 V ― ― 1 μA 
Current consumption 
IM1 
Output: Open 
Standby mode (IN1/IN2=Low) ― ― 1 μA 
IM2 Output: Open 
Brake mode (IN1/IN2=High) ― 3 4 mA 
IM3 
Output: Open 
fPWM=30kHz ― 3.5 5 mA 
Motor output 
Leakage current 
High IOH 
VM=50 V, Vout=0 V 
Standby mode (IN1/IN2=Low) ― ― 1 μA 
Low IOL 
VM=Vout=50 V 
Standby mode (IN1/IN2=Low) -1 ― ― μA 
Output setting current accuracy ΔIout Iout=1.5 A -5 0 5 % 
RS pin current IRS 
VRS=0V, VM=24 V 
Standby mode (IN1/IN2=Low) 0 ― 1 μA 
Output transistor 
On-resistance between  
drain and source 
 (High side + low side) 
Ron(H+L) 
Tj=25°C, Forward direction 
(High side + low side) 
Iout=1.5 A 
― 0.6 0.8 Ω 
 
Note 1: VIN (HYS) is defined as the difference between VIN (H) and VIN (L). VIN (H) is the voltage when the voltage (VIN) 
to the input  pins (IN1 and IN2) is raised and the output  pins (OUT1 and OUT2) change from H  to L. VIN (L) is the 
voltage when the VIN (H) is lowered and the output pins (OUT1 and OUT2) change from L to H.  
VIN (HYS) = VIN (H) – VIN (L) 
 
Note: The internal circuits are designed to avoid EMF or leakage current; when the logic signal is applied while the VM is 
not supplied. Please consider the control signal timing before supplying the VM. 
 
