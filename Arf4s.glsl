#[compute]
#version 450
#extension GL_ARB_gpu_shader_int64 : require
#extension GL_EXT_scalar_block_layout : require

/* Compute Shader for Aerials Player
 * Copyright (c) 2025- 1dealGas Project
 */
#define FUMEN(x, T, N, size)    layout(set = 0, binding = x, std430)  restrict uniform _##N {T v[size];} N;
#define FUMENV(x, T, N, size)   layout(set = 0, binding = x, std430)  restrict buffer  _##N {T v[size];} N;
#define RUNTIME(x, T, N, size)  layout(set = 1, binding = x, std430)  restrict buffer  _##N {T v[size];} N;

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
FUMEN(0, uint, PointMs, 16384)
FUMEN(1, uint, Point,   16384)

/* Child (dt)
 * [5] radius: [0,8) 1/4
 * [6] initLoop: [0,1) 1/64
 * [7] deltaLoop: [-2,2) 1/32
 */
FUMEN(2, uint, ChildDt, 16384)   // (18: ms/4) (13: absScale 1/1024 [0,8)
FUMEN(3, uint, Child,   16384)

/* Wish
 * [6] pointCount
 * [14] pointSince
 * [1] isSpecial
 * [10] childCount
 * [14] childSince
 */
FUMEN(4, uint64_t, Wish, 8192)

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
FUMENV(5, uint64_t, Hint, 8192)
FUMENV(6, uint64_t, Echo, 8192)

/* Packed
 * [1] isDaymode
 * [1] isFullScreen
 * [1] anyTouchPressed
 * [4] validTouchCount
 * [8] frameDeltaMs
 * [8] minDt
 * [8] maxDt
 */
layout(push_constant, std430) restrict uniform Ain {
	uint   msTime, deltaTime, Packed, ordBase, wishIdx, hintIdx, echoIdx;   // (10 count) (13 since)
	float  xScale, yScale, rotSin, rotCos;
	vec2   validTouches[10];
} In;

layout(set = 1, binding = 0, std430) restrict buffer Aout {
	uint  visibleWishes, visibleHints, visibleEchoes, visibleAnims;
	uint  hHit, eHit, spJudged, early, late, lost;
} Out;

//-------------------------//

struct VisibleWish {
	float x, y, a, scale;
	uint64_t ord;
};
struct VisibleHint {
	float x, y, r, g, b;
	uint64_t ord;
};
struct VisibleEcho {
	float x, y, a, scale, hx, hy, r, g, b;
	uint64_t ord;
};
struct Anim {
	float x, y, r, g, b, w, lsc, rsc, ld, rd;
	uint64_t ord;
};

shared vec4 wishOut;
RUNTIME(1, VisibleWish, WishOut, 1024)
RUNTIME(2, VisibleHint, HintOut, 1024)
RUNTIME(3, VisibleEcho, EchoOut, 1024)
RUNTIME(4, Anim, AnimOut, 2048)

//-------------------------//

#define SORT(T, ARR, SIZE) {                                                               \
	uint N = SIZE - 1;                                                                     \
	N |= N >> 1;    N |= N >> 2;    N |= N >> 4;    N |= N >> 8;                           \
	const uint elemPerIvc = (++N + 63) >> 6;                                               \
	for (uint k = 2; k <= N; k <<= 1) {                                                    \
		for (uint j = k >> 1; j > 0; j >>= 1) {                                            \
			for (uint t = 0; t < elemPerIvc; ++t) {                                        \
				const uint i = gl_LocalInvocationIndex + (t << 6), partner = i ^ j;        \
				if (i >= N || partner <= i)  continue;                                     \
				if (( ARR .v[i].ord  >  ARR .v[partner].ord ) == bool( i & (k >> 1) )) {   \
					T _ = ARR .v[i];                                                       \
					ARR .v[i] = ARR .v[partner];                                           \
					ARR .v[partner] = _;                                                   \
				}                                                                          \
			}                                                                              \
			barrier();                                                                     \
		}                                                                                  \
	}                                                                                      \
}   /* Implements a Bitonic Sort */

//-------------------------//

layout(local_size_x = 8, local_size_y = 8) in;
void main() {
	switch( gl_WorkGroupID.x ) {
		/* Hint */ case 0: {
			// Judge
			// Output
			barrier();

			// Sort
			if( Out.visibleHints > 0 )
				SORT(VisibleHint, HintOut, Out.visibleHints)
			break;
		}

		/* Echo */ case 1: {
			// Judge
			// Output
			barrier();

			// Sort
			if( Out.visibleEchoes > 0 )
				SORT(VisibleEcho, EchoOut, Out.visibleEchoes)
			break;
		}

		/* Wish */ default: {
			// Node
			barrier();

			// Child
			barrier();

			// Sort
			if( gl_WorkGroupID.x == 2  &&  Out.visibleWishes > 0 )
				SORT(VisibleWish, WishOut, Out.visibleWishes)
		}
	}
}