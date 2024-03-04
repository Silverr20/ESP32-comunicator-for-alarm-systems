# ESP32 comunicator for alarm systems
 Esp32 linked to a DSC alarm system to offer alerts and communication to and from telegram
Here are some basic instructions:
You need to power the board with 5V, you can use the 12 volts from the alarm system with a step down module.
Also since the alarm is powered with 12 volts and the esp board is working with lower tension ,  you need to use some relays for the 3 pins used.
On the DSC board you can use PNG1 for alarm, PNG2 for Status monitoring, and one of the input zones for arming/disarming. Use your specific DSC programming manual, but since they are mostly consistent you can use option 22 for zone [Momentary Keyswitch Arm - Arm or Disarm the system when violated]; PGM1  use option 01 [Fire and Burglary]; PGM2 option 17 [Away Armed Status].
