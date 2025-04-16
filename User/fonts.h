/**
 * @author  Tilen Majerle
 * @email   tilen@majerle.eu
 * @website http://stm32f4-discovery.com
 * @link
 * @version v1.0
 * @ide     Keil uVision
 * @license GNU GPL v3
 * @brief   Fonts library for LCD libraries
 *
@verbatim
   ----------------------------------------------------------------------
    Copyright (C) Tilen Majerle, 2015

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
   ----------------------------------------------------------------------
@endverbatim
*/

#ifndef TM_FONTS_H
#define TM_FONTS_H 110

/* C++ detection */
#ifdef __cplusplus
extern C {
#endif

#include "ch32v00x.h"

typedef struct {
	uint8_t FontWidth;    /*!< Font width in pixels */
	uint8_t FontHeight;   /*!< Font height in pixels */
	const uint16_t *data; /*!< Pointer to data font data array */
} TM_FontDef_t;

extern TM_FontDef_t TM_Font_7x10;
extern TM_FontDef_t TM_Font_11x18;
extern TM_FontDef_t TM_Font_16x26;

/* C++ detection */
#ifdef __cplusplus
}
#endif
#endif

