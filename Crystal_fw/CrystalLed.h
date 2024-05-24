/*
 * CrystalLed.h
 *
 *  Created on: 30 нояб. 2021 г.
 *      Author: layst
 */

#ifndef CRYSTALLED_H__
#define CRYSTALLED_H__

#include "ChunkTypes.h"

#define LED_CNT     4

namespace CrystalLeds {

void Init();
void SetHsvNow(ColorHSV_t hsv);
void SetHsvSmoothly(ColorHSV_t hsv);

} // namespace

#endif //CRYSTALLED_H__
