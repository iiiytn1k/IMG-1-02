## Flyback Inductor partlist
<pre>
TDK-EPCOS B66208W1010T001    E25/13/7, Vertical coil former  
TDK-EPCOS B66317G0000X187    E25/13/7, N87 Core  
TDK-EPCOS B66317G0250X187    E25/13/7, N87 Core with 0.25mm air gap  
TDK-EPCOS B66208A2010X000    E25/13/7, Yoke (need two pieces)  
</pre>  
## Jumpers
There are several jumpers on the bottom layer of the driver board.  
In normal operation (when all 74HC595 connected in a daisy chain configuration) jumpers **DATA_SW1**, **DATA_SW3**, **CLK_SW**, **LAT_SW** must be shorted, jumper **DATA_SW2** must be opened.  
In some cases, you can need to separately control the anode drive registers (via SPI1 connector) and the cathode decoder register (via SPI2 connector). In this case, jumper **DATA_SW2** must be shorted, other jumpers must be opened.  
Also you can control the cathode decoder directly via X1-X8 connector. In this case, U14 should not be soldered, jumper **DATA_SW2** must be shorted, other jumpers must be opened.  
## 5v switch
In normal operation, pins 1-2 must be shorted. In this case, IMG-1-02 logic powered by driver board LDO and U1-U14 powered via SPI1 connector.  
When pins 2-3 shorted, IMG-1-02 logic and U1-U14 powered via SPI1 connector.  
When pins 1-2-3 shorted, IMG-1-02 logic and U1-U14 powered by driver board LDO.   
