# TB67H450FNG — 第0章 · p20

TB67H450FNG 
2020-11-26 
 
20 
 
Points to remember on handling of ICs 
(1) Over current Protection Circuit 
Over current protection circuits (referred to as current limiter circuits) do not necessarily protect ICs under all 
circumstances. If the over current protection circuits operate against the over current, clear the over current status 
immediately.  
Depending on the method of use and usage conditions, such as exceeding absolute maximum ratings can cause the 
over current protection circuit to not operate properly or IC breakdown before operation. In addition, depending on the 
method of use and usage conditions, if over current continues to flow for a long time after operation, the IC may 
generate heat resulting in breakdown.  
 
(2) Thermal Shutdown Circuit 
Thermal shutdown circuits do not necessarily protect ICs under all circumstances. If the thermal shutdown circuits 
operate against the over temperature, clear the heat generation status immediately. 
Depending on the method of use and usage conditions, such as exceeding absolute maximum ratings can cause the 
thermal shutdown circuit to not operate properly or IC breakdown before operation. 
 
(3) Heat Radiation Design 
In using an IC with large current flow such as power amp, regulator or driver, please design the device so that heat is 
appropriately radiated, not to exceed the specified junction temperature (Tj) at any time and condition. These ICs 
generate heat even during normal use. An inadequate IC heat radiation design can lead to decrease in IC life, 
deterioration of IC characteristics or IC breakdown. In addition, please design the device taking into considerate the 
effect of IC heat radiation with peripheral components. 
 
(4) Back-EMF 
When a motor reverses the rotation direction, stops or slows down abruptly, a current flow back to the motor’s power 
supply due to the effect of back-EMF. If the current sink capability of the power supply is small, the device’s motor 
power supply and output pins might be exposed to conditions beyond absolute maximum ratings. To avoid this problem, 
take the effect of back-EMF into consideration in system design. 
 
 

