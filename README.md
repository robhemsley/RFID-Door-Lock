RFID-Door-Lock
==============

Arduino sketch to provide user access control for DIY RFID locks. Code allows users to be easily added and removed via a serial interface which allows changes to be saved back to EEPROM.  
The code can be used with the MIT Media Lab door lock created by Valentin Heun (http://colorsaregood.com/door/)  

## Installation ##

* Install the following Library Dependencies:  
	CapSense - http://www.arduino.cc/playground/Main/CapSense  
	SoftwareSerial - http://arduino.cc/hu/Reference/SoftwareSerial  
	Servo - http://arduino.cc/it/Reference/Servo  

* Restart Arduino IDE
* Open RFID_DOOR.pde
* Set the following variables between lines 7-14:  
	STATIC_ID_LOAD 	- Comma seperated RFID IDs for loading known cards  
	SERVO_PIN 	- The logic pin the servo is connected to  
	SOFT_RX		- The RX pin serial is connected to  
	SOFT_TX		- The TX pin serial is connected to  
	ADMIN_TIMEOUT	- The number of milliseconds till admin mode starts  

* Upload sketch and tap the first users card against the reader
* Users can be managed via the serial monitor (Type h for cmd listing)

hello at robhemsley.co.uk
