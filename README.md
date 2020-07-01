# Li_ion_Cell_Charger_Tester_with_Arduino
Li-ion cell tester implementing CC and CV modes using a MOSFET with current feedback. 
Cell terminal voltage and current is logged to a memory card. 

## Cell tester schematic
Schematic of the cell tester 
* Use a MOSFET capable of dissipating the lost power as per your DC supply voltage
* Use any general purpose op-amp (LM324 used here)
* Use 1 ohm resistors for sensing and protection against accidental shorts
* Use a suitable differential amplifier circuit to sense cell voltage 

<img src = "https://github.com/rishab94/Li_ion_Cell_Charger_and_Tester_with_Arduino/blob/master/Cell_Tester.png" height="35%" width="35%">

## Cell tester top view 
<img src = "https://github.com/rishab94/Li_ion_Cell_Charger_and_Tester_with_Arduino/blob/master/CellTester_TopView.jpg" height="50%" width="50%">

## Cell tester connected to the DC supply 
<img src = "https://github.com/rishab94/Li_ion_Cell_Charger_and_Tester_with_Arduino/blob/master/CellTester_WithSuupply.jpg" height="50%" width="50%">

## Terminal voltage and current for a Li-ion cell charged using the tester
CC-CV charge profile executed on an 18650 Li-ion cell.
<img src = "https://github.com/rishab94/Li_ion_Cell_Charger_and_Tester_with_Arduino/blob/master/C2_C_2209_large_font.png"  height="75%" width="75%">

## SoC-OCV relation for a Li-ion cell obtained using the tester with a C/30 charge/discharge
Pseudo OCV curves for the same cell. The average of the charge and discharge curves can be taken to be the OCV.  
<img src = "https://github.com/rishab94/Li_ion_Cell_Charger_and_Tester_with_Arduino/blob/master/OCV_vs_SoC_final_large_font.png" height="75%" width="75%">
