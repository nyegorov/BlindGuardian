#pragma once


class Tmp75
{
public:
	enum resolution { res9bit, res10bit, res11bit, res12bit };

	Tmp75(int i2c_address, resolution res) : _tmp75_address(i2c_address), _res(res)	{}
	void begin()	
	{
		delay(100);
		 // Serial.println("Initializing TMP75");
		Wire.begin();                      // Join the I2C bus as a master 
		Wire.beginTransmission(_tmp75_address);      // Address the TMP75 sensor
		Wire.write(configReg);                       // Address the Configuration register 
		Wire.write(_res << 5);                       // Set the temperature resolution 
		Wire.endTransmission();                      // Stop transmitting
		Wire.beginTransmission(_tmp75_address);      // Address the TMP75 sensor
		Wire.write(rdOnly);                          // Address the Temperature register 
		Wire.endTransmission();                      // Stop transmitting 	
	}
	float get_temp()
	{
		// Now take a Temerature Reading
		Wire.requestFrom(_tmp75_address, 2);         // Address the TMP75 and set number of bytes to receive
		byte MostSigByte = Wire.read();              // Read the first byte this is the MSB
		byte LeastSigByte = Wire.read();             // Now Read the second byte this is the LSB

		// Being a 12 bit integer use 2's compliment for negative temperature values
		int TempSum = (((MostSigByte << 8) | LeastSigByte) >> 4); 

		// From Datasheet the TMP75 has a quantisation value of 0.0625 degreesC per bit
		float temp = TempSum * multiplier[_res];
		return temp;     
	}
private:
	const float multiplier [4] = { 0.5, 0.25, 0.125, 0.0625 };
	const byte configReg = 0x01;        // Address of Configuration Register
	const byte bitConv = B01100000;     // Set to 12 bit conversion
	const byte rdWr = 0x01;             // Set to read write
	const byte rdOnly = 0x00;           // Set to Read
	int _tmp75_address;
	resolution _res = res12bit;
};