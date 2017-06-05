# CS120B Door Security System
CS120B Project by: Justin Mac

Introduction:
This project is a door security system, that you can control via bluetooth. The user is able to input a passcode. If the user does not input the correct passcode after 3 attempts, the alarm goes off. The alarm also goes off if the door is locked and the PIR sensor detects motion.

Materials:
- Bluetooth module (HM-10)
- Atmega1284
- Piezo buzzer
- LightBlue iPhone app
- PIR Sensor (Passive Infrared Sensor)
- LCD screen

Build upons:
- Bluetooth module
- USART
- PIR sensor 

PIN-OUT:
A1 = PIR Sensor
B6 = Speaker/Piezo buzzer
C0-C7 = LCD Screen
D0 = Bluetooth TX
D1 = Bluetooth RX
D3 = Lock LED
D4 = Unlock LED
D5 = Motion LED
D6 = LCD RS
D7 = LCD Enable

Known bugs:
- PIR Sensor needs its own PORT, cannot have any other add-ons in same port

Sources:
- https://www.exploreembedded.com/wiki/PIR_motion_Sensor_interface_with_Atmega128


