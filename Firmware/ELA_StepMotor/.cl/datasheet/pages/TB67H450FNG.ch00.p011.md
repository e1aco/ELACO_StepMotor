# TB67H450FNG — 第0章 · p11

TB67H450FNG 
2020-11-26 
 
11 
 
Absolute Maximum Ratings (Ta = 25°C) 
Characteristics Symbol Rating Unit Remarks 
Motor power supply (non active) VM 50 V Standby mode 
Motor power supply (active) -0.4 to 44 V Operation mode 
Motor output voltage Vout 50 V ― 
Motor output current Iout 3.5 A (Note 1) 
Logic input pin voltage VIN(H) 6.0 V ― 
VIN(L) -0.4 V ― 
VREF pin voltage Vref 0 to 5.5 V ― 
Power dissipation PD 2.85 W (Note 2) 
Operating temperature Topr -40 to 85 °C ― 
Storage temperature Tstg -55 to 150 °C ― 
Junction temperature Tj 150 °C ― 
Note 1: The maximum current value in normal operation should be used at 70% or less (Iout ≤ 2.45A) of the absolute 
maximum ratings after thermal calculation. The maximum output current may be further limited in view of thermal 
considerations, depending on ambient temperature and board conditions. 
Note 2: On PCB (JEDEC 4 layers). When the ambient temperature exceeds above Ta =25°C, derate the power 
dissipation by 22.8 mW/°C. 
 
Ta :  Ambient temperature 
Topr : Ambient temperature while the device is active.  
Tj : Junction temperature while the device is active. The maximum junction temperature is limited by thermalshutdown 
(TSD) circuitry. It is advisable to keep the maximum current below a certain level so that the maximum junction 
temperature, Tj (max), will not exceed 120°C. 
 
Caution) Absolute maximum ratings 
The absolute maximum ratings of a semiconductor device are a set of ratings that must not be exceeded, even for a  
moment. Do not exceed any of these ratings. Exceeding the rating (s) may cause device breakdown, damage or  
deterioration, and may result in injury by explosion or combustion. The value of even one parameter of the absolute  
maximum ratings should not be exceeded under any circumstances. The TB67H45 0FNG does not have overvoltage  
detection circuit. Therefore, the device is damaged if a voltage exceeding its rated maximum is applied. All voltage ratings, 
including supply voltages, must always be followed. The other notes and considerations described later should  also be 
referred to. 
 
 
Operating Range (Ta=-40 to 85°C) 
Characteristics Symbol Min Typ. Max Unit Remarks 
Motor power supply voltage VM 4.5 24 44 V ― 
Motor output current Iout ― 1.5 3.0 A ― 
Logic input voltage 
VIN(H) 2.0 ― 5.5 V H level of logic 
VIN(L) 0 ― 0.8 V L level of logic 
Input range of control logic 
frequency fLOGIC ― ― 400 kHz IN1, IN2 
Input range of Vref voltage Vref 0 2.0 4.0 V Constant current 
drive 
 
Note: The actual maximum current may be limited by the operating environment (operating conditions such operating 
duration, or by the surrounding temperature or board heat dissipation). Determine a realistic maximum current by 
calculating the heat generated under the operating environment. 
 
