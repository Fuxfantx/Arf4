#[compute]
#version 450
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_EXT_scalar_block_layout : require

/* Compute Shader for Aerials Player
 * Copyright (c) 2025- 1dealGas Project
 */
#define FUMEN(x, T, N)    layout(set = 0, binding = x, std430)  restrict uniform _##N {T v[];} N;
#define FUMENV(x, T, N)   layout(set = 0, binding = x, std430)  restrict buffer  _##N {T v[];} N;
#define RUNTIME(x, T, N)  layout(set = 1, binding = x, std430)  restrict buffer  _##N {T v[];} N;

/* Easetype */
#define  STATIC   0
#define  LINEAR   1
#define  INSINE   2
#define  OUTSINE  3

/* Object Status */
#define  NJUDGED       0
#define  SPECIAL       1
#define  NJUDGED_HIT   2
#define  SPECIAL_LIT   3
#define  EARLY         4
#define  EARLY_LIT     5
#define  HIT           6
#define  HIT_LIT       7
#define  LATE          8
#define  LATE_LIT      9
#define  AUTO          10
#define  SPECIAL_AUTO  11
#define  LOST          12
#define  SPECIAL_LOST  13
#define  PENDING      -128

/* Point (ms)
 * [7] x: [0,16) 1/8
 * [6] y: [0,8) 1/8
 * [2] easeType: STATIC, LINEAR, INSINE, OUTSINE
 * [6] radius: [0,16) 1/4
 * [11] degree: [-2048, 2047] 2/1
 */
FUMEN(1, uint, PointTime)   /* 16384 */
FUMEN(2, uint, PointArg)    /* 16384 */

/* Child (dt)
 * [5] radius: [0,8) 1/4
 * [6] initLoop: [0,1) 1/64
 * [7] deltaLoop: [-2,2) 1/32
 */
FUMEN(3, uint, ChildDt)   /* 16384 */   // (18: ms/4) (13: absScale 1/1024 [0,8)
FUMEN(4, uint, Child)     /* 16384 */

/* Wish
 * [6] pointCount
 * [14] pointSince
 * [1] isSpecial
 * [10] childCount
 * [14] childSince
 */
FUMEN(5, uint64_t, Wish)   /* 8192 */

/* Object
 * [20] ms
 * [7] x: [0,16) 1/8
 * [6] y: [0,8) 1/8
 * [4] status
 * [8] deltaMs: [-128, 128)
 * [Echo 5] radius: [0,8) 1/4
 * [Echo 6] initLoop: [0,1) 1/64
 * [Echo 7] deltaLoop: [-2,2) 1/32
 */
FUMENV(6, uint64_t, Hint)   /* 8192 */
FUMENV(7, uint64_t, Echo)   /* 8192 */

/* Packed
 * [1] isDaymode
 * [1] anyPressed
 * [1] anyReleased
 * [4] validTouchCount
 * [8] minDt
 * [8] maxDt
 */
layout( push_constant, std430 ) restrict uniform _In {
	uint   msTime, deltaTime, Packed, wishIdx, hintIdx, echoIdx;   // (10 count) (13 since)
	float  xScale, yScale, rotSin, rotCos, sizeX, sizeY;
	vec2   validTouches[10];
} In;

layout( set = 1, binding = 1, std430 ) restrict buffer _Out {
	uint  visibleWishes, visibleHints, visibleEchoes, visibleEchoHelpers, visibleAnims;
	uint  hHit, eHit, spJudged, early, late, lost;
} Out;

RUNTIME(2, vec4,  WishOut)   /* 1024: (x, y, scale, tint.w) */
RUNTIME(3, float, HintOut)   /* 5120: [x, y, tint.r, tint.g, tint.b] */
RUNTIME(4, float, EchoOut)   /* 3072: [x, y, scale] */
RUNTIME(5, vec4,  EchoCol)   /* 1024: (tint.r, tint.g, tint.b, tint.w) */
RUNTIME(6, vec2,  EchoHlp)   /* 1024: (x, y) */
RUNTIME(7, float, AnimOut)   /* 8192: [x, y, scale, leftRotDeg, rightRotDeg] */
RUNTIME(8, vec4,  AnimCol)   /* 2048: (tint.r, tint.g, tint.b, tint.w) */

//-------------------------//

layout( local_size_x = 8, local_size_y = 8 ) in;

void main() {
	HintOut.v[1] = 1.0f;
}