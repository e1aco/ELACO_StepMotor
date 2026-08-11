# TB67H450FNG — 第0章 · p6

TB67H450FNG 
2020-11-26 
 
6 
 
Functional Description 
Input and output function 
IN1 IN2 OUT1 OUT2 Mode 
L L OFF (Hi-Z) OFF (Hi-Z) Stop 
Standby mode after 1 ms  
H L H L Forward 
L H L H Reverse 
H H L L Brake 
Current path: Forward rotation (OUT1 to OUT2), Reverse rotation (OUT2 to OUT1) 
 
 
 
Standby mode 
When both IN1 and IN2 pins are set to L for 1 ms (typ.), the operation mode translates to the standby mode.  
 
Item Min Typ. Max Unit 
Time to standby 0.7 1 1.5 ms 
 
The following period in which both IN1 and IN2 pins are set to L is the standby transition period. 
Do not change the input states during this period since the IC becomes unstable.  
 
• If [STOP] mode is used, set period of IN1 =L and IN2 =L to 0.7 ms or less. 
• If [Standby] mode is used, set period of IN1 =L and IN2 =L to 1.5 ms or more. 
 
In standby mode, when IN1 or IN2 is set to H, the mode returns from the standby mode, and enters to the operation mode.  
Maximum 30 μs is required for the return time from the standby release. 
The OUT1 and OUT2 outputs operate after 30 μs (max) from the standby release. 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
OUT1: Hi-Z 
OUT2: Hi-Z 
IN1 
IN2 
OUT1 
OUT2 
30 μs (max) 
OUT1: Hi-Z 
OUT2: Hi-Z 
OUT1: H 
OUT2: L 
OUT1: H 
OUT2: L 
OUT1: Hi-Z 
OUT2: Hi-Z 
OUT1: H 
OUT2: L 
Return 
time 
Standby 
mode 
0.7 ms 
Standby 
transition period 
1.5 ms 
Stop 
mode 
H 
L 
H 
L 
