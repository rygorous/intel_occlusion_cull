//--------------------------------------------------------------------------------------
// Copyright 2013 Intel Corporation
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
#include "TaskMgrTBB.h"

enum SOC_TYPE
{
	SCALAR_TYPE,
	SSE_TYPE,
};

extern SOC_TYPE gSOCType;
extern float gOccluderSizeThreshold;
extern float gOccludeeSizeThreshold;
extern UINT  gDepthTestTasks;

extern TASKSETHANDLE gInsideViewFrustum[2];
extern TASKSETHANDLE gTooSmall[2];
extern TASKSETHANDLE gActiveModels[2];
extern TASKSETHANDLE gXformMesh[2];
extern TASKSETHANDLE gBinMesh[2];
extern TASKSETHANDLE gSortBins[2];
extern TASKSETHANDLE gRasterize[2];
extern TASKSETHANDLE gAABBoxDepthTest[2];

extern LARGE_INTEGER glFrequency;

#define PI 3.1415926535f

const int SCREENW = 1280;
const int SCREENH = 720;

const int TILE_WIDTH_IN_PIXELS   = SCREENW/4;
const int TILE_HEIGHT_IN_PIXELS  = SCREENH/8;

const int SCREENW_IN_TILES = SCREENW/TILE_WIDTH_IN_PIXELS;
const int SCREENH_IN_TILES = SCREENH/TILE_HEIGHT_IN_PIXELS;

const int NUM_XFORMVERTS_TASKS = 16;

const int NUM_TILES = (SCREENW/TILE_WIDTH_IN_PIXELS) * (SCREENH/TILE_HEIGHT_IN_PIXELS);

// depending upon the scene the max #of tris in the bin should be changed.
const int MAX_TRIS_IN_BIN_MT = 1024 * 16;
const int MAX_TRIS_IN_BIN_ST = 1024 * 16;

const int YOFFSET1_ST = SCREENW_IN_TILES;
const int XOFFSET1_ST = 1;

const int YOFFSET2_ST = SCREENW_IN_TILES * MAX_TRIS_IN_BIN_ST;
const int XOFFSET2_ST = MAX_TRIS_IN_BIN_ST;

const int YOFFSET1_MT = SCREENW_IN_TILES;
const int XOFFSET1_MT = 1;
const int TOFFSET1_MT = SCREENW_IN_TILES * SCREENH_IN_TILES;

const int YOFFSET2_MT = SCREENW_IN_TILES * NUM_XFORMVERTS_TASKS * MAX_TRIS_IN_BIN_MT;
const int XOFFSET2_MT = NUM_XFORMVERTS_TASKS * MAX_TRIS_IN_BIN_MT;

const int SSE = 4;

const int AABB_VERTICES = 8;
const int AABB_INDICES  = 36;
const int AABB_TRIANGLES = 12;

const float4x4 viewportMatrix(
    0.5f*(float)SCREENW,                 0.0f,  0.0f, 0.0f,
                   0.0f, -0.5f*(float)SCREENH,  0.0f, 0.0f,
                   0.0f,                 0.0f,  1.0f, 0.0f,
    0.5f*(float)SCREENW,  0.5f*(float)SCREENH,  0.0f, 1.0f
);

const int OCCLUDER_SETS = 2;
const int OCCLUDEE_SETS = 4;

const int AVG_COUNTER = 10;

const int NUM_DT_TASKS = 50;
#endif