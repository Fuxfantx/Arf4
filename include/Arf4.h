//  Aerials Fumen Player v4  //
#pragma once
#define Arf4_API int Ar::
#include <unordered_map>
#include <dmsdk/sdk.h>

namespace Arf4 {
	enum TableIndex		{		WGO = 2, HGO, EGO, EHGO, AGOL, AGOR, WTINT, HTINT, ETINT, ATINT			};
	enum EaseType		{	 STATIC = 0, LINEAR, INSINE, OUTSINE, INQUAD, OUTQUAD,
						  CLOCKWISE = 5, COUNTERCLIOCKWISE = 10, CPOSITIVE = 5, CNEGATIVE = 10			};
	enum Status			{	NJUDGED = 0, NJUDGED_LIT, EARLY, EARLY_LIT, HIT, HIT_LIT, LATE, LATE_LIT,
							AUTO, SPECIAL, SPECIAL_LIT, SPECIAL_AUTO, LOST, PENDING = -127				};

	// Primitive
	struct PosNode {
		int32_t		ms:28/*7*/;
		uint32_t	ease:4;
		float		x, y, ci, ce = 1;							// Center (cdx:0, cdy:0)
		/*--------------------------------*/					// Some Partial-Ease calculation results
		float		xFci, xFce, xDnm;							//   are cached here to improve the ease
		float		yFci, yFce, yDnm;							//   performance.
	};
	struct Hint {
		float		x, y;
		uint32_t	ms:20, status:4;							// Max 1048575ms -> 17.47625 mins
		int32_t		deltaMs:8 = PENDING;
	};
	struct Echo {
		float		fromX, fromY, toX, toY, ci, ce = 1;
		uint32_t	fromMs:27/*7*/, ease:4, isReal:1;
		uint32_t	toMs:20, status:4;
		int32_t		deltaMs:8;
		/*--------------------------------*/
		float		xFci, xFce, xDnm;
		float		yFci, yFce, yDnm;
	};

	// DeltaTime
	struct DeltaNode {
		double									baseDt;
		uint32_t								initMs;
		float									ratio;			// BPM * Scale / 15000 -> {-16, 16, 1/131072}
	};
	struct DeltaGroup {
		std::vector<DeltaNode>					nodes;
		std::vector<DeltaNode>::iterator		it;
		double									dt;
	};

	// WishChild
	struct AngleNode {
		uint16_t	distance:13 = 7 << 9;						// [0,16)
		uint16_t	ease:3;
		int16_t		degree;
	};
	struct WishChild {
		std::vector<AngleNode>					aNodes;
		std::vector<AngleNode>::iterator		aIt;
		double									dt;
	};

	// Fumen
	union Duo {
		uint64_t								whole;
		struct {
			union							  { float a;  uint32_t aa;  };
			union							  { float b;  uint32_t bb;  };
		};
	};
	struct Idx  { std::vector<uint32_t>			wIdx, hIdx, eIdx;										};
	struct Wish {
		std::vector<PosNode>					nodes;
		std::vector<WishChild>					wishChilds;
		/*--------------------------------*/
		std::vector<PosNode>::iterator			pIt;
		std::vector<WishChild>::iterator		cIt;
		/*--------------------------------*/
		uint64_t								deltaGroup;
	};
	struct Fumen {
		std::vector<Idx>						idxGroups;
		std::vector<DeltaGroup>					deltaGroups;
		std::vector<Wish>						wish;
		std::vector<Hint>						hint;
		std::vector<Echo>						echo;
		//--------------------------------//
		uint64_t								msTime:20;
		uint64_t								before:20;
		uint64_t								objectCount:15;
		uint64_t								wgoRequired:9 = 511, hgoRequired:8, egoRequired:9;
		//--------------------------------//
		uint64_t								hHit:15, eHit:15, early:15, late:15, lost:15, spJudged:12;
		uint64_t								daymode:1, judgeRange:7 = 37;
		int64_t									minDt:8, maxDt:8;
		//--------------------------------//
		float									objectSizeX = 360, objectSizeY = 360;
		float									xScale = 1, yScale = 1, xDelta, yDelta, rotSin, rotCos = 1;
		float									boundL = -36, boundR = 1836, boundU = 1116, boundD = -36;
		//--------------------------------//
		std::unordered_map<uint64_t, uint16_t>	lastWgo;
		std::vector<Duo>						blocked;
	};
}
extern  Arf4::Fumen  Arf;
extern  int8_t		 InputDelta;

namespace Ar {
	using namespace Arf4;

	/* Internal */
	 Duo  InterpolateEcho(const Echo& echo, uint32_t currentMs);
	 Duo  InterpolatePosNode(const PosNode& currentPn, uint32_t currentMs, const PosNode& nextPn);
	void  PrecalculatePosNode(PosNode& currentPn);
	void  PrecalculateEcho(Echo& currentPn);
	void  JudgeArfSweep();

	/* Fumen Operations */
	 int  LoadArf(lua_State* L);
	 int  ExportArf(lua_State* L);
	 int  OrganizeArf(lua_State* L);
	 int  UpdateArf(lua_State* L);
	 int  JudgeArf(lua_State* L);

	/* Fumen Utils */
	 int  SetCam(lua_State* L);
	 int  SetBound(lua_State* L);
	 int  SetDaymode(lua_State* L);
	 int  SetObjectSize(lua_State* L);
	 int  SetJudgeRange(lua_State* L);
	 int  GetJudgeStat(lua_State* L);

	/* Other Utils */
	 int  NewTable(lua_State* L);
	 int  PushNullPtr(lua_State* L);
	 int  SimpleEaseLua(lua_State* L);
	 int  PartialEaseLua(lua_State* L);
	 int  DoHapticFeedback(lua_State* L);
	 int  SetInputDelta(lua_State* L);
}