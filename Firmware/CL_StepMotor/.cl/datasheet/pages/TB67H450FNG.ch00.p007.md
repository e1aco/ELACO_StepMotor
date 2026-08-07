# TB67H450FNG — 第0章 · p7

TB67H450FNG 
2020-11-26 
 
7
 
Constant current PWM blanking time 
 
In TB67H450FNG, the following blanking time is set to prevent a spike current and external noise which are generated 
during driving a motor. 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
The timing charts or constants may be omitted or simplified for explanatory.  
 
tBLK (For preventing incorrect detection of a spike current at changing from Decay to Charge): 3.6 μs (typ.): (1) 
 
The blanking time, 400 ns (typ.) is also set for preventing an incorrect detection around setting current value (NFth).: (2) 
* 
The time widths shown in the above figure are the design values, and the values are not guaranteed. 
 
Blanking time between Input signal and tBLK 
The tBLK is intended to avoid inrush current detection. The TB67H450FNG not only can be controlled by constant current 
PWM, but also by direct PWM; with IN control signals. Therefore the tBLK is set at each IN switch timing; shown with gray in 
the timing chart below. 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
Timing charts may be simplified for explanatory purposes. 
 
IN1  
IN2  
Iout  
Charge  
Fast  
Slow  
NFth  
(1) tBLK  
(2)  


<!-- detected tables -->

|  |  | (1) tBLK |  |  | NFth |  |
|---|---|---|---|---|---|---|
| Charge Fast Slow iming charts or constants may be omitted or simplified for explanatory. |  |  |  |  |  |  |

| IN1 IN2 Iout |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
