/*
 *   byteorder.h
 *   Copyright (C) 2025 David García Goñi <dagargo@gmail.com>
 *
 *   This file is part of emu3bm.
 *
 *   emu3bm is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   emu3bm is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with emu3bm.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BYTEORDER_H
#define BYTEORDER_H

#include "../config.h"

#if defined(HAVE_ENDIAN_H)

#include <endian.h>

#elif defined(HAVE_SYS_ENDIAN_H)

//The BSDs provide the same macros from a different header.
#include <sys/endian.h>

#elif defined(HAVE_LIBKERN_OSBYTEORDER_H)

//macOS has no endian.h, so the macros are mapped onto its own byte swapping.
#include <libkern/OSByteOrder.h>

#define htole16(x) OSSwapHostToLittleInt16 (x)
#define le16toh(x) OSSwapLittleToHostInt16 (x)
#define htole32(x) OSSwapHostToLittleInt32 (x)
#define le32toh(x) OSSwapLittleToHostInt32 (x)
#define htobe16(x) OSSwapHostToBigInt16 (x)
#define be16toh(x) OSSwapBigToHostInt16 (x)
#define htobe32(x) OSSwapHostToBigInt32 (x)
#define be32toh(x) OSSwapBigToHostInt32 (x)

#else

#error "No byte order conversion macros found."

#endif

#endif
