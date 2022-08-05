/**********************************************************************************
 * JTAG -                                                                         *
 *                                                                                *
 * This library is free software; you can redistribute it and/or                  *
 * modify it under the terms of the GNU Lesser General Public                     *
 * License as published by the Free Software Foundation; either                   *
 * version 2.1 of the License, or (at your option) any later version.             *
 *                                                                                *
 * This library is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of                 *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU              *
 * Lesser General Public License for more details.                                *
 *                                                                                *
 * You should have received a copy of the GNU Lesser General Public               *
 * License along with this library; if not, write to the Free Software            *
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA *
 **********************************************************************************/

#ifndef _JTAG_H_
#define _JTAG_H_

extern "C" {
#include "jtag_host.h"
}

void JTAG_Walk(uint8_t numticks, uint16_t path);
void JTAG_Write_DR_to_IR(uint16_t IR, uint8_t* data, uint32_t numbits);
void JTAG_Read_DR_from_IR(uint16_t IR, uint8_t* data, uint32_t numbits);
void JTAG_Write_VDR_to_VIR(uint8_t VIR, uint8_t* data, uint32_t numbits);
void JTAG_Read_VDR_from_VIR(uint8_t VIR, uint8_t* data, uint32_t numbits);

#endif
