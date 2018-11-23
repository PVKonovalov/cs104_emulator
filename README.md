
  # cs104_emulator
Smart meter emulator with IEC60870-5-104 protocol provides sending single point and measurement data. It using comma separated values files with one measure value per 10 minutes for measurement and timestamp with value for the single point. A linear approximation using to generate every second's value for analog data.

# external libs
lib60870-5-101/104 from MZ-Automation http://libiec61850.com/libiec61850/

# compiling

    make

# run

    cs104_emulator <path to directory with csv files> [-p ip_port] [-d]
    	-d start as Unix daemon
    	-p ip port, default 2404

  
