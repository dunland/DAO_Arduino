1. compile in Arduino IDE  
2. copy hex path from console output  
3. insert here and run from console: `avrdude -v -patmega328p -cstk500v2 -PCOM9 -Uflash:w:C:\Users\David\AppData\Local\Temp\arduino_build_950381/DAO_masterCode2.08.ino.hex:i`