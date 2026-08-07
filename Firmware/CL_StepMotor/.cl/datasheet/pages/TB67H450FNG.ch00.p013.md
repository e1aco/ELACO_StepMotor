# TB67H450FNG — 第0章 · p13

TB67H450FNG 
2020-11-26 
 
13 
 
Electrical Characteristics 2 (Ta =25°C, VM = 24 V, unless otherwise specified) 
Characteristics 
Symbol 
Test conditions 
Min 
Typ. 
Max 
Unit 
VREF pin input current 
Iref 
Vref=2.0 V 
― 
0 
1 
μA 
Vref attenuation ratio 
Vref(gain) 
Vref=2.0 V 
1/10.4 
1/10 
1/9.6 
― 
Thermal shutdown (TSD) circuit operating 
temperature (Note 1) 
TjTSD 
― 
150 
160 
175 
°C 
Thermal shutdown (TSD) hysteresis 
TjTSDhys 
― 
― 
30 
― 
°C 
UVLO voltage (Note 2) 
VUVLO 
At rising VM 
3.8 
4.0 
4.2 
V 
UVLO hysteresis voltage 
Vhys_uvlo 
― 
― 
200 
― 
mV 
Over current detection (ISD) circuit 
operating current (Note 3) 
ISD 
― 
4.1 
4.9 
5.7 
A 
 
Note 1: Thermal shutdown (TSD)  *auto return  
When the junction temperature of the IC reaches the TSD threshold, the TSD circuit is triggered; the internal reset circuit 
then turns off the output transistors. In order to avoid malfunction by switching etc., detection mask time is prepared inside 
IC. Since the operating temperature of TSD circuit has a hysteresis width, the IC returns automatically when the junction 
temperature is lowered to the temperature to return. 
The TSD circuit is a backup function to detect a thermal error, therefore is not recommended to be used aggressively. 
 
Note 2: Under voltage lockout (UVLO) 
When the supply voltage to VM pin is 3.8V (typ.) or less, the internal circuit is triggered; the internal reset circuit then turns 
off the output transistors. Once the UVLO is triggered, it can be cleared by reasserting the VM supply voltage to 4.0V (typ.) 
or more 
 
Note 3: Over current detection (ISD)  *Latch operation 
When the output current reaches the threshold, the ISD circuit is triggered; the internal reset circuit then turns off the output 
transistors. In order to avoid malfunction by switching etc., detection mask time is prepared inside IC. Once the ISD circuit is 
triggered, the IC is set to standby mode, and can be cleared by reasserting VM power supply, or a return operation after 
setting to standby mode (After both pins of IN1 and IN2 are set to Low for 1.5 ms or more, IN1 pin or IN2 pin is set to High).  
Additionally, the IC has a circuit as a short-circuit detection of output pins (OUT1 and OUT2) which are adjacent to RS pin, if 
the voltage more than the threshold is applied to RS pin, the circuit turns off the output transistors. 
 


<!-- detected tables -->

| Characteristics | Symbol | Test conditions | Min | Typ. | Max | Unit |
|---|---|---|---|---|---|---|
| VREF pin input current | Iref | Vref=2.0 V | ― | 0 | 1 | μA |
| Vref attenuation ratio | Vref(gain) | Vref=2.0 V | 1/10.4 | 1/10 | 1/9.6 | ― |
| Thermal shutdown (TSD) circuit operating temperature (Note 1) | TjTSD | ― | 150 | 160 | 175 | °C |
| Thermal shutdown (TSD) hysteresis | TjTSDhys | ― | ― | 30 | ― | °C |
| UVLO voltage (Note 2) | VUVLO | At rising VM | 3.8 | 4.0 | 4.2 | V |
| UVLO hysteresis voltage | Vhys uvlo _ | ― | ― | 200 | ― | mV |
| Over current detection (ISD) circuit operating current (Note 3) | ISD | ― | 4.1 | 4.9 | 5.7 | A |
