/*******************************************************************************
 Copyright(c) 2020 Rick Bassham. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

#include "auxproto.h"

#include <libindi/indibasetypes.h>
#include <libindi/inditelescope.h>

class CelestronCommandHandler
{
public:
  CelestronCommandHandler() {}
  virtual ~CelestronCommandHandler() {}

  virtual bool HandleGetVersion(INDI_EQ_AXIS axis, char *version) = 0;
  virtual bool HandleGetPosition(INDI_EQ_AXIS axis, long steps) = 0;
  virtual bool HandleStartAlign(INDI_EQ_AXIS axis) = 0;
  virtual bool HandleAlignDone(INDI_EQ_AXIS axis, bool done) = 0;
  virtual bool HandleMoveNegative(INDI_EQ_AXIS axis) = 0;
  virtual bool HandleMovePositive(INDI_EQ_AXIS axis) = 0;
  virtual bool HandleGotoFast(INDI_EQ_AXIS axis) = 0;
  virtual bool HandleGotoSlow(INDI_EQ_AXIS axis) = 0;
  virtual bool HandleSetPosition(INDI_EQ_AXIS axis) = 0;
  virtual bool HandleTrack() = 0;
  virtual bool HandleSlewDone(INDI_EQ_AXIS axis, bool done) = 0;
  virtual bool HandleGetAutoguideRate(INDI_EQ_AXIS axis, uint8_t rate) = 0;
  virtual bool HandleSetAutoguideRate(INDI_EQ_AXIS axis) = 0;
  virtual bool HandleGuidePulse(INDI_EQ_AXIS axis) = 0;
  virtual bool HandleGuidePulseDone(INDI_EQ_AXIS axis, bool done) = 0;
};

class CelestronDriver
{
public:
  enum SlewRate
  {
    SLEW_STOP = 0x00,
    SLEW_MAX = 0x09,
    SLEW_FIND = 0x07,
    SLEW_CENTERING = 0x03,
    SLEW_GUIDE = 0x02,
  };

public:
  CelestronDriver();
  const char *getDeviceName();

  void SetPortFD(int portFD)
  {
    PortFD = portFD;
  }

  void SetHandler(CelestronCommandHandler *handler)
  {
    m_handler = handler;
  }

  void ReadPendingCommands();

  bool GetVersion(INDI_EQ_AXIS axis);
  bool StartAlign(INDI_EQ_AXIS axis);
  bool GetPosition(INDI_EQ_AXIS axis);
  bool GetAutoguideRate(INDI_EQ_AXIS axis);
  bool CheckGuideDone(INDI_EQ_AXIS axis);
  bool CheckAlignDone(INDI_EQ_AXIS axis);
  bool CheckSlewDone(INDI_EQ_AXIS axis);
  bool SetPosition(INDI_EQ_AXIS axis, long steps);
  bool MovePositive(INDI_EQ_AXIS axis, SlewRate rate);
  bool MoveNegative(INDI_EQ_AXIS axis, SlewRate rate);
  bool GuidePulse(INDI_EQ_AXIS axis, uint32_t ms, int8_t rate);
  bool GoToFast(INDI_EQ_AXIS axis, long steps);
  bool GoToSlow(INDI_EQ_AXIS axis, long steps);
  bool Track(bool enable, INDI::Telescope::TelescopeTrackMode trackMode);

  // TODO: Implement PEC
  bool PecSeekIndex();
  bool PecGetNumberOfBins();
  bool PecGetPecIndex();
  bool PecStartPlayback();
  bool PecStopPlayback();
  bool PecStartRecord();
  bool PecStopRecord();
  bool PecCheckRecordDone();
  bool PecGetValue(uint8_t index);
  bool PecSetValue(uint8_t index, int8_t value);

private:
  bool
  sendCmd(AUXCommand cmd);
  bool readCmd(int timeout = 1);
  bool handleCommand(AUXCommand cmd);

  int PortFD = -1;
  CelestronCommandHandler *m_handler;
};