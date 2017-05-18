#pragma once

class Tmp75
{
public:
	enum resolution : int8_t { res9bit = 1, res10bit, res11bit, res12bit };

	Tmp75(int i2c_address, resolution res) : _tmp75_address(i2c_address), _res(res)	{}
	void begin()	
	{
		delay(100);
		Wire.begin();                                // Join the I2C bus as a master 
		Wire.beginTransmission(_tmp75_address);      // Address the TMP75 sensor
		Wire.write(configReg);                       // Address the Configuration register 
		Wire.write(_res << 5);                       // Set the temperature resolution 
		Wire.endTransmission();                      // Stop transmitting
		Wire.beginTransmission(_tmp75_address);      // Address the TMP75 sensor
		Wire.write(tempReg);                         // Address the Temperature register 
		Wire.endTransmission();                      // Stop transmitting 	
	}
	int32_t get_temp()
	{
		Wire.requestFrom(_tmp75_address, 2);         // Address the TMP75 and set number of bytes to receive
		byte data[2] = { Wire.read(), Wire.read() };
		return	(((data[0] << 8) | data[1]) >> 4) >> _res;
	}
private:
	const byte tempReg   = 0x00;        // Address of Temperature Register
	const byte configReg = 0x01;        // Address of Configuration Register
	int _tmp75_address;
	resolution _res = res12bit;
};
