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

#include "JTAG.h"

void JTAG_Walk(uint8_t numticks, uint16_t path)
{ for(int i=0; i<numticks; i++, path >>= 1) jtag_host_pulse_tck(path & 0x0001);
}

void JTAG_Write_DR_to_IR(uint16_t IR, uint8_t* data, uint32_t numbits)
{ JTAG_Walk(10, 0x00DF);
  jtag_host_pulse_tdio_instruction(10, (unsigned int)IR);
  JTAG_Walk(5, 0x0007);
  int NumBytes = numbits >> 3;
  int NumBits = numbits & 0x07;
  if (NumBits == 0)
     { NumBits = 8;
       NumBytes = NumBytes - 1;
     }
  if (NumBytes > 0) jtag_host_pulse_tdi(data, (size_t)(NumBytes));
  jtag_host_pulse_tdi_instruction(NumBits, (unsigned int)data[NumBytes]);
  JTAG_Walk(5, 0x001F);
}

void JTAG_Read_DR_from_IR(uint16_t IR, uint8_t* data, uint32_t numbits)
{ JTAG_Walk(10, 0x00DF);
  jtag_host_pulse_tdio_instruction(10, (unsigned int)IR);
  JTAG_Walk(5, 0x0007);
  if ((numbits >> 3) > 0) jtag_host_pulse_tdo(data, (size_t)(numbits >> 3));
  if ((numbits & 0x00000007) != 0) data[numbits >> 3] = jtag_host_pulse_tdio((int)(numbits & 0x00000007), 0);
  JTAG_Walk(5, 0x001F);
}

void JTAG_Write_VDR_to_VIR(uint8_t VIR, uint8_t* data, uint32_t numbits)
{ uint8_t dr[1];
  dr[0] = 0x10 | (VIR & 0x0F);
  JTAG_Write_DR_to_IR(0x00E, dr, 5);
  JTAG_Write_DR_to_IR(0x00C, data, numbits);
}
  
void JTAG_Read_VDR_from_VIR(uint8_t VIR, uint8_t* data, uint32_t numbits)
{ uint8_t dr[1];
  dr[0] = 0x10 | (VIR & 0x0F);
  JTAG_Write_DR_to_IR(0x00E, dr, 5);
  JTAG_Read_DR_from_IR(0x00C, data, numbits);
}
