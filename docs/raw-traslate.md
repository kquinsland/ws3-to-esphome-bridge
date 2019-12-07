This is the RAW google translate text from the Aliexpress page for the WS3 module.
See: [Three Generations APRS WS1 WS3 Meteorological Station Module Matching Pressure, Temperature and Humidity RS485 MODBUS](https://www.aliexpress.com/item/4000016596586.html)

And it does look like there's a "newer" control board that has 24v PoE and some sort of web server / client built in to get the data
shipped off to various web services:

[Original IoT 51WS5 weather station compatible with APRS](https://www.aliexpress.com/item/4000149792753.html)

```
51WS3 weather interface module data storage and calculation instructions
 
 
Rainfall: The 51WS3 weather interface board has a built-in 1440-byte rain-specific data buffer that records the rain sample value per minute.
 
After rolling calculation, the rainfall value of the previous minute, the first hour, and the first 24 hours is obtained.
 
Wind speed: The 51WS3 weather interface board has a built-in 300-byte wind speed dedicated data buffer to record real-time wind speed samples per second.
 
After rolling calculation, the real-time wind speed, the average wind speed in the first 1 minute, and the highest wind speed in the first 5 minutes are obtained.
 
Temperature: Read temperature, humidity and pressure data every 5 seconds
 
note:
 
JP1 short circuit = ARS inch mode, JP1 open circuit = professional metric mode
 
JP2 short circuit = 2400 rate, JP2 open circuit = 9600 rate
 
Output one measurement data at intervals of 0.5 seconds.
 
APRS data output format (imperial, default):
 
C000s000g000t082r000p000h48b10022*3C
 
 
Output 38 bytes every 0.5 seconds, including newline at the end of the data (OD, OA)
 
data analysis:
 
C000: wind direction angle, unit: degree.
 
S000: wind speed in the first minute, unit: miles per hour
 
G000: the highest wind speed in the first 5 minutes, in miles per hour
 
T086: temperature (Fahrenheit)
 
R000: rainfall in the previous hour (0.01 inches)
 
P000: rainfall (0.01 inches) in the first 24 hours
 
H53: humidity (00% - 99%)
 
B10020: air pressure (0.1 hpa)
 
 
 
*20 checksum, all character XOR results between A and * (excluding *)
 
The interface board will automatically detect whether the air pressure gauge, temperature and humidity sensor are installed, and the sensor data that is not installed will display “...”.
 
For example, if the temperature and humidity sensor and the air pressure piece are not installed, the output data is:
 
C000s000g000t...r000p000h..b.....
 
Meteorological module supports automatic identification of particulate matter sensor PMS5003 PMS5003ST
 
 
 
Default data format with checksum data format: c000s000g000t086r000p000h45b09963*31
 
 
 
When it is detected that the PMS5003 is installed, the data is output:
 
 
 
Contains PM2.5 and check data format: c000s000g000t082r000p000h43b09985, 027,035,040,023,032,040,*14
 
Professional measurement data output format (metric):
 
Short JP1 is JP1=0 inch, open JP1, ie JP1=1 metric
 
 
 
After power-on, enter the professional measurement mode to facilitate secondary development.
 
 
 
DATA data interface 9600, 2400 rate
 
Professional measurement data format:
 
A4095B000C0000D0000E0000F0000G0000H0000I0000J0000K0000L0281M576N10024*27
 
Output 74 bytes every 0.5 seconds, including line breaks at the end of the data (OD, OA)
 
 
 
data analysis:
 
A0789: Wind direction measures AD value in real time (0-4095)
 
B000: Wind direction angle value (16 directions)
 
C0000: real-time wind speed frequency 1Hz
 
D0000: Real-time wind speed 0.1M/S
 
E0000: Average wind speed in the previous minute 0.1m/s
 
F0000: The highest wind speed in the first 5 minutes 0.1m/s
 
G0000: Real-time rain bucket, 0-9999, loop count
 
H0000: The number of rain buckets in the previous minute, 0-9999
 
I0000 : The first 1 minute of rainfall 0.1mm
 
J0000: The previous hour's rainfall 0.1mm
 
K0000: The first 24 hours of rainfall 0.1mm
 
L0209: Temperature (degrees Celsius), 0.1 degree, below zero, the first digit displays the symbol "-".
 
M703: Humidity 0.1 (0% = 99%)
 
N10233 air pressure (0.1 hpa)
 
 
 
*27 checksum, all character XOR results between A and * (excluding *)
 
51WS3 calibration adjustment function
 
 The 51WS3 supports the calibration adjustment data function, which can be used when the original data value needs to be adjusted in some special circumstances.
 
 
 
The factory default of 51WS3 is that the temperature, humidity and air pressure are all raw data output, that is, no increase or decrease.
 
 1. Connect the USB to TTL data cable (optional), connect 51WS3, open the universal serial port debugging software, and select the correct port number, the rate is 9600.
 
 
 
2, send the command AT+SET=T+000H+000B+0000 Enter, that is, the setting is completed.
 
Note: All setup commands must be + Enter (ENTER).
 
  
 
Command: AT+VER=? Enter
 
Description: Display the current firmware version number
 
 
 
 
 
Command: AT+SET=T+000H+000B+0000 Enter
 
Description: Addition and subtraction of temperature, humidity and barometric raw data
 
 
 
Detailed instructions
 
AT+SET=T+000H+000B+0000
 
T+000 means temperature +00.0 degrees (unit: Celsius), calibration range +-99.9
 
Note: This calibration value is metric Celsius.
 
H+000 means humidity +00.0%, calibration range +-99.9
 
When calibrating addition, if the measured humidity + calibration value exceeds 99.9%, it is fixed at 99.9%.
 
When calibrating subtraction, the subtraction is allowed if the humidity > correction value is measured.
 
B+000 means air pressure +000.0hpa, calibration range +-999.9hpa
 
 
 
Example:
 
Raw data: c000s000g000t082r000p000h49b09957
 
Settings: AT+SET=T+000H-080B-1000 Enter
 
New data: c000s000g000t082r000p000h41b08957
 
 
 
51WS3 weather interface module data storage and calculation instructions
 
Rainfall: The 51WS3 weather interface board has a built-in 1440-byte rain-specific data buffer that records the rain sample value per minute.
 
After rolling calculation, the rainfall value of the previous minute, the first hour, and the first 24 hours is obtained.
 
Wind speed: The 51WS3 weather interface board has a built-in 300-byte wind speed dedicated data buffer to record real-time wind speed samples per second.
 
After rolling calculation, the real-time wind speed, the average wind speed in the first 1 minute, and the highest wind speed in the first 5 minutes are obtained.
 
Temperature: Read temperature, humidity and pressure data every 5 seconds
```
