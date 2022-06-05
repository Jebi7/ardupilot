/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "AP_ThrustSensor_BotaSys.h"

#include <AP_HAL/AP_HAL.h>
#include <ctype.h>
#include <GCS_MAVLink/GCS.h>

extern const AP_HAL::HAL& hal;

#define LIGHTWARE_DIST_MAX_CM           10000
#define LIGHTWARE_OUT_OF_RANGE_ADD_CM   100

// read - return last value measured by sensor
bool AP_ThrustSensor_BotaSys::get_reading(float &reading_m)
{
    switch(sensorComm.readFrame())
    {
        case BotaForceTorqueSensorComm::VALID_FRAME:
            if (sensorComm.frame.data.status.val>0)
            {
                // the measurements are invalid 
                gcs().send_text(MAV_SEVERITY_CRITICAL, "status: %x", sensorComm.frame.data.status.val);
            }
            else
            {
                // Do something with the good measurements
                state.force_n = sensorComm.frame.data.forces[2];
                reading_m = state.force_n ;
                if (state.offset_flag) {
                    //gcs().send_text(MAV_SEVERITY_CRITICAL, "offset: %5.3f", (double)(state.offset_n));
                    //reading_m -= state.offset_n;
                }
                //gcs().send_text(MAV_SEVERITY_CRITICAL, "reading_m: %5.3f", reading_m);
                /*static uint8_t counter = 0;
                counter++;
                if (counter > 1) {
                counter = 0;
                gcs().send_text(MAV_SEVERITY_CRITICAL, "get_reading %5.3f", (double)(reading_m));
                }*/   
                return true;
            }
        break;
        case BotaForceTorqueSensorComm::NOT_VALID_FRAME:
            gcs().send_text(MAV_SEVERITY_CRITICAL, "No valid frame. crc count: %d", sensorComm.get_crc_count());
        break;
        case BotaForceTorqueSensorComm::NOT_ALLIGNED_FRAME:
            gcs().send_text(MAV_SEVERITY_CRITICAL, "lost sync, trying to reconnect");
        break;
        case BotaForceTorqueSensorComm::NO_FRAME:
        break;
    }
    return false;
}


BotaForceTorqueSensorComm::BotaForceTorqueSensorComm()
{
  _synced = false;
}

static uint16_t BotaForceTorqueSensorComm::crc16_mcrf4xx(uint8_t *data, size_t len)
{
    uint16_t crc = 0xffff;
    while (len--) {
        crc ^= *data++;
        for (int i=0; i<8; i++)
            crc = crc & 0x0001 ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }
    return crc;
}

static uint16_t BotaForceTorqueSensorComm::crc16_ccitt_false(uint8_t* data, size_t len)
{
    uint16_t crc = 0xffff;
    while (len--) {
        crc ^= *data++ << 8;
        for (int i=0; i < 8; i++)
            crc = crc & 0x8000 ? (crc << 1) ^ 0x1021 : crc << 1;
    }
    return crc;
}

static uint16_t BotaForceTorqueSensorComm::crc16_x25(uint8_t* data, size_t len)
{
    uint16_t crc = 0xffff;
    while (len--) {
        crc ^= *data++ << 0;
        for (int i=0; i < 8; i++)
            crc = crc & 0x0001 ? (crc >> 1) ^ 0x8408 : crc >> 1;
    }
    return crc ^ 0xffff;
}

bool BotaForceTorqueSensorComm::isCrcOk()
{
  if(crc16_x25(frame.data.bytes, sizeof(frame.data)) == frame.crc)
  {
    return true;
  }
  else
  {
    _crc_err_count += 1; 
    return false;
  }  
}

bool BotaForceTorqueSensorComm::checkSync()
{
  if (_synced)
  {
    _synced = (frame.header == 0xAA);
  }
  else
  {
    _crc_err_count = 0;
    _synced = (frame.header == 0xAA) & isCrcOk();
  }
  return _synced;
}

BotaForceTorqueSensorComm::ReadFrameRes BotaForceTorqueSensorComm::readFrame() // {VALID_DATA, NOT_VALID_DATA, NOT_ALLIGNED_DATA, NO_DATA}
{
  ReadFrameRes err = NO_FRAME;
  if(serialAvailable()>=sizeof(frame))
  {
    serialReadBytes(frame.bytes, sizeof(frame));
    if(checkSync())
    {
      if (isCrcOk())
      {
        err = VALID_FRAME;
      }
      else
      {
        err = NOT_VALID_FRAME;
      }
    }
    else
    {
      err = NOT_ALLIGNED_FRAME;
      //read one dummy bytes to regain sync
      if (serialAvailable())
      {
        uint8_t dummy;
        serialReadBytes(&dummy, sizeof(dummy));
      }
    }
  }
  return err; //sucess 
}
