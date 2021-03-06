#include<Wire.h>
#include "Sparkfun_APDS9301_Library.h"
#include <Arduino.h>

APDS9301::APDS9301()
{
  // Don't do squat here. Normal constructor functions are deferred to
  //  .begin().
}

APDS9301::status APDS9301::begin(uint8_t address)
{
  this->address = address;
  powerEnable(POW_ON);
  //APDS9301::status result = setGain(HIGH_GAIN);
  //if (setIntegrationTime(INT_TIME_402_MS) != SUCCESS) result = I2C_FAILURE;
  //return result;
  return SUCCESS;
}

APDS9301::status APDS9301::powerEnable(APDS9301::powEnable powEn)
{
  if (powEn == POW_OFF) return setRegister(CONTROL_REG, 0);
  else                  return setRegister(CONTROL_REG, 3);
}

APDS9301::status APDS9301::setGain(APDS9301::gain gainLevel)
{
  uint8_t regVal = getRegister(TIMING_REG);
  if (gainLevel == LOW_GAIN) regVal &= ~0x10;
  else                       regVal |= 0x10;
  return setRegister(TIMING_REG, regVal);
}

APDS9301::status APDS9301::setIntegrationTime(APDS9301::intTime integrationTime)
{
  uint8_t regVal = getRegister(TIMING_REG);
  regVal &= ~0x03;
  if (integrationTime == INT_TIME_13_7_MS)      regVal |= 0x00;
  else if (integrationTime == INT_TIME_101_MS)  regVal |= 0x01;
  else if (integrationTime == INT_TIME_402_MS)  regVal |= 0x02;
  return setRegister(TIMING_REG, regVal);
}

APDS9301::status APDS9301::enableInterrupt(APDS9301::interruptEnable intMode)
{
  uint8_t regVal = getRegister(INTERRUPT_REG);
  // This is not a typo- OFF requires bits 4&5 to be cleared, but
  //  ON only requires bit 4 to be set. I dunno why.
  if (intMode == INT_OFF) regVal &= ~0x30;
  else                    regVal |= 0x10;
  return setRegister(INTERRUPT_REG, regVal);
}

APDS9301::status APDS9301::clearIntFlag()
{
  Wire.beginTransmission(address);
  Wire.write(0xC0);
  APDS9301::status retVal;
  if (Wire.endTransmission() == 0) return SUCCESS;
  else return I2C_FAILURE;
}

APDS9301::status APDS9301::setCyclesForInterrupt(uint8_t cycles)
{
  uint8_t regVal = getRegister(INTERRUPT_REG);
  regVal &= ~0x0F; // clear lower four bits of register
  cycles &= ~0xF0; // ensure top four bits of data are clear
  regVal |= cycles; // Sets any necessary bits in regVal.
  return setRegister(INTERRUPT_REG, regVal);
}

APDS9301::status APDS9301::setLowThreshold(unsigned int threshold)
{
  int retVal = setTwoRegisters(THRESHLOWLOW_REG, threshold);
  if (retVal == 0) return SUCCESS;
  else             return I2C_FAILURE;
}

APDS9301::status APDS9301::setHighThreshold(unsigned int threshold)
{
  int retVal = setTwoRegisters(THRESHHILOW_REG, threshold);
  if (retVal == 0) return SUCCESS;
  else             return I2C_FAILURE;
}

uint8_t APDS9301::getIDReg()
{
  return getRegister(ID_REG);
}

APDS9301::gain APDS9301::getGain()
{
  uint8_t regVal = getRegister(TIMING_REG);
  regVal &= 0x10;
  if (regVal != 0) return HIGH_GAIN;
  else             return LOW_GAIN;
}

APDS9301::intTime APDS9301::getIntegrationTime()
{
  uint8_t regVal = getRegister(TIMING_REG);
  regVal &= 0x03;
  if (regVal == 0x00)       return INT_TIME_13_7_MS;
  else if (regVal == 0x01)  return INT_TIME_101_MS;
  else                      return INT_TIME_402_MS;
}

uint8_t APDS9301::getCyclesForInterrupt()
{
  uint8_t regVal = getRegister(INTERRUPT_REG);
  regVal &= ~0x0F;
  return regVal;
}

unsigned int APDS9301::getLowThreshold()
{
  unsigned int retVal = getRegister(THRESHLOWHI_REG)<<8;
  retVal |= getRegister(THRESHLOWLOW_REG);
  return retVal;
}

unsigned int APDS9301::getHighThreshold()
{
  unsigned int retVal = getRegister(THRESHHIHI_REG)<<8;
  retVal |= getRegister(THRESHHILOW_REG);
  return retVal;
}

unsigned int APDS9301::readCH0Level()
{
  return getTwoRegisters(DATA0LOW_REG);
}

unsigned int APDS9301::readCH1Level()
{
  return getTwoRegisters(DATA1LOW_REG);
}

float APDS9301::readLuxLevel()
{
  unsigned int ch1Int = readCH1Level();
  unsigned int ch0Int = readCH0Level();
  float ch0 = (float)readCH0Level();
  float ch1 = (float)readCH1Level();
  switch (getIntegrationTime())
  {
    case INT_TIME_13_7_MS:
    
    if ((ch1Int >= 5047) || (ch0Int >= 5047)) 
    {
      return 1.0/0.0;
    }
    break;
    case INT_TIME_101_MS:
    if ((ch1Int >= 37177) || (ch0Int >= 37177)) 
    {
      return 1.0/0.0;
    }
    break;
    case INT_TIME_402_MS:
    if ((ch1Int >= 65535) || (ch0Int >= 65535))
    { 
      return 1.0/0.0;
    }
    break;
  }
  float ratio = ch1/ch0;
  switch (getIntegrationTime())
  {
    case INT_TIME_13_7_MS:
    ch0 *= 1/0.034;
    ch1 *= 1/0.034;
    break;
    case INT_TIME_101_MS:
    ch0 *= 1/0.252;
    ch1 *= 1/0.252;
    break;
    case INT_TIME_402_MS:
    ch0 *= 1;
    ch1 *= 1;
    break;
  }

  if (getGain() == LOW_GAIN) 
  {
    ch0 *= 16;
    ch1 *= 16;
  }
  
  float luxVal = 0.0;
  if (ratio <= 0.5)
  {
    luxVal = (0.0304 * ch0) - ((0.062 * ch0) * (pow((ch1/ch0), 1.4)));
  }  
  else if (ratio <= 0.61)
  {
    luxVal = (0.0224 * ch0) - (0.031 * ch1);
  }
  else if (ratio <= 0.8)
  {
    luxVal = (0.0128 * ch0) - (0.0153 * ch1);
  }
  else if (ratio <= 1.3)
  {
    luxVal = (0.00146 * ch0) - (0.00112*ch1);
  }

  return luxVal;
  return 0;
}

uint8_t APDS9301::getRegister(uint8_t regAddress)
{
  Wire.beginTransmission(address);
  Wire.write(regAddress);
  Wire.endTransmission(false);
  Wire.requestFrom(address, (uint8_t)1);
  return Wire.read();
}

uint16_t APDS9301::getTwoRegisters(uint8_t regAddress)
{
  Wire.beginTransmission(address);
  Wire.write(0x20 | regAddress);
  Wire.endTransmission(false);
  Wire.requestFrom(address, (uint8_t)2);
  uint16_t regVal = Wire.read();
  return regVal | (Wire.read()<<8);
}

APDS9301::status APDS9301::setRegister(uint8_t regAddress, uint8_t newVal)
{
  Wire.beginTransmission(address);
  Wire.write(regAddress);
  Wire.write(newVal);
  if (Wire.endTransmission() == 0) return SUCCESS;
  else return I2C_FAILURE;

}

APDS9301::status APDS9301::setTwoRegisters(uint8_t regAddress, uint16_t newVal)
{
  Wire.beginTransmission(address);
  Wire.write(0x20 | regAddress);
  Wire.write((uint8_t)newVal);
  Wire.write((uint8_t)(newVal>>8));
  if (Wire.endTransmission() == 0) return SUCCESS;
  else return I2C_FAILURE;
}
