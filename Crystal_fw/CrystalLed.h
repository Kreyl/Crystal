/*
 * CrystalLed.h
 *
 *  Created on: 30 нояб. 2021 г.
 *      Author: layst
 */

#pragma once

#include "color.h"
#include "ChunkTypes.h"

namespace Leds {

void Init();
void SetAllHsv(ColorHSV_t hsv);
void StartSeq(LedRGBChunk_t *lsq);
void Off();
bool AreOff();

} // namespace
