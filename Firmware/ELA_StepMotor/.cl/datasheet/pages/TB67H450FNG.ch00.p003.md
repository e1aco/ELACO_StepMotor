# TB67H450FNG — 第0章 · p3

TB67H450FNG 
2020-11-26 
 
3 
 
Block Diagram 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
Some of the functional blocks, circuits, or constants in the block diagram may be omitted or simplified for explanatory 
purposes. 
 
 
Note: All the grounding wires of the TB67H 450FNG must run on the solder mask of the PCB.  It must also be externally  
terminated at a single point. Also, the grounding method should be considered for efficient heat dissipation. 
 
Careful attention should be paid to the layout of the output, VM and GND traces, to avoid short circuits across outpu t 
pins or to the power supply or ground. If such a short circuit occurs, the device may be permanently damaged. 
Also, the utmost care should be taken for pattern designing and implementation of the device since it has power  
supply pins (VM, RS, OUT 1, OUT2, and GND) through which a particularly large current may run. If these pins are 
wired incorrectly, an operation error may occur or the device may be destroyed. 
 
The logic input pins must also be wired correctly. Otherwise, the device may be damaged owing t o a current running  
through the IC that is larger than the specified current. Careful attention should be paid to design patterns and  
mountings.  
 
 
 
 
  
IN1 
IN2 
 
Pre-driver 
 
 
H-Bridge 
OUT1 
OUT2 
UVLO  
 
 
Control 
Logic 
VM 
RS 
VCC Regulator 
VREF 
Current 
Comp 
GND 
Charge 
Pump 
Up to 5V 
ISD 
TSD 
