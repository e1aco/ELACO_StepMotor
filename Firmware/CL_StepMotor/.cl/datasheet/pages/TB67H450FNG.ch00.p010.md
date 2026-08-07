# TB67H450FNG — 第0章 · p10

TB67H450FNG 
2020-11-26 
 
10 
 
Calculation of Predefined Output Current 
This IC controls a motor operation by PWM constant current control. The peak current value (setting current value) can be 
determined by settings of the current-sensing resistor (RRS) and the reference voltage (Vref). 
 
Vref (V) 
Iout (max) = Vref (gain)  ×  
RRS (Ω) 
 
Vref (gain) : The Vref decay rate is 1 / 10.0 (typ.). 
 
Example: In case of 100% setting 
 
When Vref is 3.0 V and RRS is 0.51 Ω, the motor constant current (Peak current) is calculated as: 
 
Iout = 3.0 V / 10.0 / 0.51 Ω= 0.59 A 
 
 
If the constant current control function is disabled, the RS pin should be connected to GND, and the voltage (1 to 5V) is 
input to VREF pin. 
 

