# TB67H450FNG — 第0章 · p8

TB67H450FNG 
2020-11-26 
 
8 
 
Motor control (Constant current control) 
 
Current waveform in Mixed Decay Mode and the setting 
 
In case of constant current control, the OFF time (toff) is fixed to determine the current ripple (pulsating), and the rate of Mix 
Decay Mode is 50 % in Fast Mode, and 50% in Slow mode. 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
If the output current is zero-detected during Fast mode, the output becomes High impedance.  
 
 
 
 
 
Waveform in Mixed Decay Mode (Current waveform) 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
Timing charts may be simplified for explanatory purposes.
 
 
 
 
MDT (Mixed Decay Timing) 
 
Charge Mode -> NF detection: Reaches setting current value  -> 
Fast Mode -> Mixed Decay Timing -> Slow Mode -> Charge Mode 
Iout 
NFth 
NFth 
Iout  
toff (fixed) 
  
 
NF detection   
NF detection   
toff / 2   toff / 2   
toff (fixed)  
Charge  
Slow  
Fast  Charge  
toff / 2   toff / 2   
toff (fixed) 
  
 
NF detection   
toff / 2   toff / 2   
