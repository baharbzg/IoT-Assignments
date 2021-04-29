# IoT-Assignments

First Assignment Hands-On Tutorial : https://bbarzegar21.medium.com/first-try-fd66034ee7aa


1. What is the problem and why do you need IoT?

In this assignment, the application scenario is a Room Monitoring System, which we want to have a real-time monitoring of temperature and light density of the room. In addition, we want to notify the user about the specific events like High temperature and three various status about light density of room containing Low, Normal and High status.
We are going to use NTC Thermistor and Photocell as sensors, simple and RGB LEDs as actuators.
The sensor values publish every 10 seconds in a periodic way. The values read by the ADC lines of the STM32 Nuclo-F401RE have a specific range. If the value read by thermistor is greater than 3400, the “Temp is High” alarm generates and the simple Red Led turn on. In addition if the value read by the photocell is greater than 4000, “Light is High”, less than 3000 and greater than 1000 “Light is Normal” and less than 1000 “Light is Low” alarms generate and a RGB LED will be Red, Green and Blue respectively.

***
2. What data are collected and by which sensors?

Two different analog data containing temperature and light density collect by Nucleo-F401RE using thermistor and photocell sensors.

***
3. What are the connected components, the protocols to connect them and the overall IoT architecture?

For the connected components at IoT device level, we have a STM32-Nucleo F401RE connected to sensors and actuators. The data publish to a local MQTT-SN protocol by RIOT which is a development OS for the board. As the AWS IoT Core does not support the MQTT-SN, we use a bridge, which connect the MQTT-SN protocol to MQTT protocol at the cloud level. Finally the data will be reached at AWS Core IoT Level and analyze using AWS Analytics Tool called Kinesis. Now the data is ready to visualize in any web-dashboard can connect to AWS IoT Core.

Overall IoT Architecture :

https://github.com/baharbzg/IoT-Assignments/blob/main/Images/Overall%20IoTArchitecture.jpg

