# TB67H450FNG — 第0章 · p19

TB67H450FNG 
2020-11-26 
 
19 
 
Notes on Contents 
(1) Block Diagrams 
Some of the functional blocks, circuits, or constants in the block diagram may be omitted or simplified for explanatory 
purposes. 
 
(2) Equivalent Circuits 
The equivalent circuit diagrams may be simplified or some parts of them may be omitted for explanatory purposes. 
 
(3) Timing Charts 
Timing charts may be simplified for explanatory purposes. 
 
(4) Application Circuits 
The application circuits shown in this document are provided for reference purposes only. Thorough evaluation is 
required, especially at the mass production design stage. Providing these application circuit examples does not grant 
a license for industrial property rights.  
 
 IC Usage Considerations 
 
 Notes on handling of ICs 
[1] The absolute maximum ratings of a semiconductor device are a set of ratings that must not be exceeded, 
even for a moment. Do not exceed any of these ratings. 
Exceeding the rating(s) may cause the device breakdown, damage or deterioration, and may result injury by 
explosion or combustion. 
 
[2] Use an appropriate power supply fuse to ensure that a large current does not continuously flow in case of 
over current and/or IC failure. The IC will fully break down when used under conditions that exceed its 
absolute maximum ratings, when the wiring is routed improperly or when an abnormal pulse noise occurs 
from the wiring or load, causing a large current to continuously flow and the breakdown can lead smoke or 
ignition. To minimize the effects of the flow of a large current in case of breakdown, appropriate settings, such 
as fuse capacity, fusing time and insertion circuit location, are required.  
 
[3] If your design includes an inductive load such as a motor coil, incorporate a protection circuit into the design 
to prevent device malfunction or breakdown caused by the current resulting from the inrush current at power 
ON or the negative current resulting from the back electromotive force at power OFF. IC breakdown may 
cause injury, smoke or ignition.  
Use a stable power supply with ICs with built-in protection functions. If the power supply is unstable, the 
protection function may not operate, causing IC breakdown. IC breakdown may cause injury, smoke or 
ignition.  
 
[4] Do not insert devices in the wrong orientation or incorrectly. 
Make sure that the positive and negative terminals of power supplies are connected properly. 
Otherwise, the current or power consumption may exceed the absolute maximum rating, and exceeding the 
rating(s) may cause the device breakdown, damage or deterioration, and may result injury by explosion or 
combustion. 
In addition, do not use any device that is applied the current with inserting in the wrong orientation or 
incorrectly even just one time. 
 
[5] Carefully select external components (such as inputs and negative feedback capacitors) and load components 
(such as speakers), for example, power amp and regulator.  
If there is a large amount of leakage current such as input or negative feedback condenser, the IC output DC 
voltage will increase. If this output voltage is connected to a speaker with low input withstand voltage, overcurrent 
or IC failure can cause smoke or ignition. (The over current can cause smoke or ignition from the IC itself.) In 
particular, please pay attention when using a Bridge Tied Load (BTL) connection type IC that inputs output DC 
voltage to a speaker directly. 

