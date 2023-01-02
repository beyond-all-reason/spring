/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _KEYMODIFIERS_H
#define _KEYMODIFIERS_H

class CKeyModifiers {
public:
   enum {
       CAMERA_MOVE_FWD =   0,  // forward
       CAMERA_MOVE_BCK =   1,  // back
       CAMERA_MOVE_LFT =   2,  // left
       CAMERA_MOVE_RGT =   3,  // right
       CAMERA_MOVE_UP  =   4,  // up
       CAMERA_MOVE_DWN =   5,  // down
       CAMERA_MOVE_TLT =   6,  // tilt
       CAMERA_MOVE_RST =   7,  // reset
       CAMERA_MOVE_RTT =   8,  // rotate
       CAMERA_STATE_FST =  9,  // fast
       CAMERA_STATE_SLW = 10,  // slow
   };

   bool state[10];
};

#endif // _KEYMODIFIERS_H
