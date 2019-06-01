/*
 * This is a bitmap compressor for the Bornhack Badge 2019
 * Copyright (C) 2019  Emil Renner Berthing
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef _DISPLAY_H
#define _DISPLAY_H

#include <stdint.h>

typedef uint8_t dp_bitstream_data_t;

struct dp_cimage {
	uint8_t width;
	uint8_t height;
	dp_bitstream_data_t data[];
};

#endif
