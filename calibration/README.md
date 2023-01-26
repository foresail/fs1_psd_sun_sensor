# Calibration tools for PSD Sun Sensors

- `psd.py` has implementation to command the PSD Sun Sensor over FTDI 232H cable
   and also command line utility perform certain tasks from the command line.
- `meas.py` has calibration measurement routine. For sensors.
- `plot.py` has scripts to plot calibration measurements.
- `fit.py` has script to calculate calibration values from the measurements.
- `lut.py` has script to generate a tangent lookup table.


## PSD Test Tool

```
usage: psd.py [-h] [--addr ADDR] [--rate RATE] [--raw] [--point] [--vector] [--angles] [--all] [--temp] [--scan] [--calib] [--set_offset SET_OFFSET SET_OFFSET SET_OFFSET] [--set_temp SET_TEMP] [--set_addr SET_ADDR]

PSD test tool

optional arguments:
  -h, --help            show this help message and exit
  --addr ADDR, -a ADDR  Sensor I2C address
  --rate RATE, -r RATE  Sampling rate
  --raw                 Sample raw data
  --point               Sample point data
  --vector              Sample vector data
  --angles              Sample angle data
  --all                 Sample all data types
  --temp                Sample temperature types
  --scan                Scan responsive sensors
  --calib               Get calibration values
  --set_offset SET_OFFSET SET_OFFSET SET_OFFSET
                        Set position offset and height
  --set_temp SET_TEMP   Set temperature offset
  --set_addr SET_ADDR   Set sensor I2C address
```

**Examples:**
Scan the whole I2C address space for PSD Sun Sensors
```
$ ./psd.py --scan
```

Read sensor temperature
```
$ ./psd.py --temp
23.6 °C
24.0 °C
24.0 °C
24.0 °C
```

Read light point position
```
$ ./psd.py --point --adrr 0x4C
1303 -1303     2
1024 -2048     2
1024 -1365     3
1303 -1303     2
1024 -1024     3
1228 -1228     2
1592 -1592     2
1303 -1303     2
```

Change PSD I2C address
```
$ ./psd.py --addr [old] --set_addr [new]
```
