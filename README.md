
  # cs104_emulator
Smart meter emulator with IEC60870-5-104 protocol provides sending single point and measurement data. It using comma separated values files with one measure value per 10 minutes for measurement and timestamp with value for the single point. A linear approximation using to generate every second's value for analog data.

# External libs
lib60870-5-101/104 from MZ-Automation http://libiec61850.com/libiec61850/

# Compiling

    1. Download and extract lib60870-5-101/104 from https://www.mz-automation.de/communication-protocols/iec-60870-5-101-104-c-source-code-library/ 
    2. Compile library
    3. Create ditectory lib60870-C/examples/cs104_emulator and put all files from this repository into one
    4. make
    5. Extract emulator_data.zip
    6. ./cs104_emulator <path to directory from step #5>
    7. Connect with any lib60870-5-104 client or SCADA

# Run

    cs104_emulator <path to directory with csv files> [-p ip_port] [-d]
    	-d start as Unix daemon
    	-p ip port, default 2404

  
