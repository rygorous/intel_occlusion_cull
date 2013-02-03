//--------------------------------------------------------------------------------------
// Copyright 2011 Intel Corporation
// All Rights Reserved
//
// Permission is granted to use, copy, distribute and prepare derivative works of this
// software for any purpose and without fee, provided, that the above copyright notice
// and this statement appear in all copies.  Intel makes no representations about the
// suitability of this software for any purpose.  THIS SOFTWARE IS PROVIDED "AS IS."
// INTEL SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, AND ALL LIABILITY,
// INCLUDING CONSEQUENTIAL AND OTHER INDIRECT DAMAGES, FOR THE USE OF THIS SOFTWARE,
// INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PROPRIETARY RIGHTS, AND INCLUDING THE
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  Intel does not
// assume any responsibility for any errors which may appear in this software nor any
// responsibility to update it.
//--------------------------------------------------------------------------------------

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "CPUTMath.h"

#define PI 3.1415926535f

const int SCREENW = 1280;
const int SCREENH = 720;

const int TILE_WIDTH_IN_PIXELS   = 256;
const int TILE_HEIGHT_IN_PIXELS  = 128;

const int SCREENW_IN_TILES = (SCREENW + TILE_WIDTH_IN_PIXELS-1)/TILE_WIDTH_IN_PIXELS;
const int SCREENH_IN_TILES = (SCREENH + TILE_HEIGHT_IN_PIXELS-1)/TILE_HEIGHT_IN_PIXELS;

const int NUM_XFORMVERTS_TASKS = 16;

const int NUM_TILES = SCREENW_IN_TILES * SCREENH_IN_TILES;

// depending upon the scene the max #of tris in the bin should be changed.
const int MAX_TRIS_IN_BIN_MT = 1024 * 16;
const int MAX_TRIS_IN_BIN_ST = 1024 * 16;

const int YOFFSET1_ST = SCREENW_IN_TILES;
const int XOFFSET1_ST = 1;

const int YOFFSET2_ST = SCREENW_IN_TILES * MAX_TRIS_IN_BIN_ST;
const int XOFFSET2_ST = MAX_TRIS_IN_BIN_ST;

const int YOFFSET1_MT = SCREENW_IN_TILES * NUM_XFORMVERTS_TASKS;
const int XOFFSET1_MT = NUM_XFORMVERTS_TASKS;

const int YOFFSET2_MT = SCREENW_IN_TILES * NUM_XFORMVERTS_TASKS * MAX_TRIS_IN_BIN_MT;
const int XOFFSET2_MT = NUM_XFORMVERTS_TASKS * MAX_TRIS_IN_BIN_MT;

const int SSE = 4;

const int AABB_VERTICES = 8;
const int AABB_INDICES  = 36;
const int AABB_TRIANGLES = 12;

const float4x4 viewportMatrix(
    0.5f*(float)SCREENW,                 0.0f,  0.0f, 0.0f,
                   0.0f, -0.5f*(float)SCREENH,  0.0f, 0.0f,
                   0.0f,                 0.0f, -1.0f, 0.0f,
    0.5f*(float)SCREENW,  0.5f*(float)SCREENH,  1.0f, 1.0f
);

const int OCCLUDER_SETS = 3;
const int OCCLUDEE_SETS = 4;

const int AVG_COUNTER = 10;

// ---- Block size specific

#if 1

const int SWIZ_SIZE = 2;

static UINT EncodePosX(UINT x)
{
	return (x >> 1) * 4 + (x & 1);
}

static UINT EncodePosY(UINT y)
{
	return (y >> 1)*SCREENW*2 + ((y & 1) << 1);
}

static UINT StepX2(UINT x)
{
	return x + 4;
}

static UINT StepX2n(UINT x, UINT n)
{
	return x + 4*n;
}

static UINT StepY2(UINT y)
{
	return y + SCREENW*2;
}

static UINT StepY2n(UINT y, UINT n)
{
	return y + SCREENW*2*n;
}

#else

const int SWIZ_SIZE = 4;

// blk|y1|x1|y0|x0

static UINT EncodePosX(UINT x)
{
	UINT offs = (x & ~3) << 2;
	offs |= (x & 2) << 1;
	offs |= (x & 1) << 0;
	return offs;
}

static UINT EncodePosY(UINT y)
{
	UINT offs = (y >> 2) * SCREENW*SWIZ_SIZE;
	offs |= (y & 2) << 2;
	offs |= (y & 1) << 1;
	return offs;
}

static UINT StepX2(UINT x)
{
	static const UINT xinc = ~0xb; // increment x1, blk
	return (x - xinc) & xinc;
}

static UINT StepY2(UINT y)
{
	static const UINT ylomask = 0x8;
	static const UINT yinc = ylomask | ~0xf;

	if ((y & ylomask) != ylomask)
		return (y - yinc) & yinc;
	else
		return (y & ~ylomask) + SCREENW*SWIZ_SIZE;
}

#endif

#endif