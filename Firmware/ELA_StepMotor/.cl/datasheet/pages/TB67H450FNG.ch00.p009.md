# TB67H450FNG — 第0章 · p9

TB67H450FNG 
2020-11-26 
 
9 
 
Operation Mode of Output Transistor 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
Operation Function of Output Transistor 
Mode U1 U2 L1 L2 
Charge ON OFF OFF ON 
Fast OFF ON ON OFF 
Slow OFF OFF ON ON 
Note: The parameters shown in the table above are examples when the current flows in the directions shown in the figures 
above. For the current flowing in the reverse direction, the parameters change as shown in the table below. 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
Mode U1 U2 L1 L2 
Charge OFF ON ON OFF 
Fast ON OFF OFF ON 
Slow OFF OFF ON ON 
 
This IC controls the motor current to be constant by 3 modes listed above. 
The equivalent circuit diagrams may be simplified or some parts of them may be omitted for explanatory purposes. 
 
Note: In the timing of an output switching, the time to prevent a through current is predefined (200 ns to 300 ns (design 
value)). 
Fast mode 
The energy of the motor coil 
is fed back to the power 
Slow mode 
A current circulates around the 
motor coil and this IC. 
Charge mode 
A current flows into the motor coil. 
 
Load 
ON 
U1 
L1 
U2 
L2 
VM 
OFF 
OFF ON 
RRS 
RS pin 
U1 
L1 
U2 
L2 
OFF 
OFF  
ON 
ON 
Load 
Charge mode 
A current flows into the motor 
coil. 
 
RRS 
VM  
RS pin  
ON 
U1 
L1 
U2 
L2 
 
Load 
Fast mode 
The energy of the motor coil 
is fed back to the power 
 
OFF 
OFF ON 
RRS  
VM  
RS pin 
U1 
L1 
U2 
L2 
OFF 
ON 
 
Load 
Slow mode 
A current circulates around the 
motor coil and this IC. 
 
ON 
OFF 
RRS 
VM   
RS pin  
U1 
L1 
U2 
L2 
OFF 
OFF  
RS pin 
VM 
ON 
ON 
Load 
RRS 
U1 
L1 
U2 
L2 
OFF 
ON 
 
Load 
ON 
RS pin 
VM 
OFF 
RRS 
