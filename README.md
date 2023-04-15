# ARCS_Arduino-Random-Control-voltage-Sequencer

Arduino code for an 8 step 3 tracks sequencer following Eurorack specifications. This sequencer is providing gate output signal with a duty defined by the user, and 3 distinct Control Voltages Channels.
This code is part of an academic student research about modular systems and ATMega implementation.

This work is licensed under the GNU General Public Licence wich allows any type of use of this code except distributing closed source versions.

Visit the other component of the project available on a linked repository ( https://github.com/Matfayt/autonomusberry ), concerning the use of Raspberry Pi and PureData as an input of a modular sequencer.

The schematic of the sequencer are available in Autodesk Fusion 360 file format or in pdf.

---------------

  09.01.2023 First edit of the code
  
  01.02.2023 Update of the code for Arduino MEGA 2560 board
  
  15.02.2023 First upload of the code on a public repository

---------------

  This code requires Arduino Mega 2560 to be functionnal.

  Hardware :  
  A 16 channels multiplexer/demultiplexer CD74HC4067
  
  A KY - 040 rotary button
  
  A NeoPixel Ring 16
  
  8 10k potentiometers
  
  5 push buttons

---------------

This project wouldn't be born without the documentation and tutorials available on :

https://www.locoduino.org/

http://www.gammon.com.au

https://www.youtube.com/@Afrorack
