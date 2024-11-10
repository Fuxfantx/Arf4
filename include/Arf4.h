//  Aerials Fumen Player v4  //
#pragma once
#define Arf4_API int Ar::
#include <unordered_map>
#include <dmsdk/sdk.h>

namespace Arf4 {
	#define Args32(...) union{ struct{__VA_ARGS__}; int32_t whole; };
	#define Args64(...) union{ struct{__VA_ARGS__}; int64_t whole; };
	struct  Idx			{ std::vector<uint32_t>  wIdx, hIdx, eIdx; };

	enum    TableIndex	{		WGO = 3, HGO, EGO, EHGO, AGOL, AGOR, WTINT, HTINT, ETINT, ATINT			};
	enum	EaseType	{	 STATIC = 0, LINEAR, INSINE, OUTSINE, INQUAD, OUTQUAD,
						  CLOCKWISE = 5, COUNTERCLIOCKWISE = 10, CPOSITIVE = 5, CNEGATIVE = 10			};
	enum	Status		{	NJUDGED = 0, NJUDGED_LIT, EARLY, EARLY_LIT, HIT, HIT_LIT, LATE, LATE_LIT,
							AUTO, SPECIAL, SPECIAL_LIT, SPECIAL_AUTO, LOST, PENDING = -127				};

	struct Point {
		float									x, y, t;
		Args32(
			uint32_t							ce:14 = 0x3FFF;
			uint32_t							ease:4 = LINEAR;
			uint32_t							ci:14;
		)
		//--------------------------------//
		float									xFci, xFce, xDnm;
		float									yFci, yFce, yDnm;
	};
	struct Echo {
		float									fromX, fromY, fromT, toX, toY, toT;
		Args32(
			uint32_t							ce:8 = 0xFF, ease:4, status:4, ci:8;
			int32_t								deltaMs:8;
		)
		//--------------------------------//
		float									xFci, xFce, xDnm;
		float									yFci, yFce, yDnm;
	};
	struct Hint {
		float									x, y;
		Args32(
			uint32_t							ms:20, status:4;	// Max 1048575ms -> 17.47625 mins
			int32_t								deltaMs:8;			// Set deltaMs = PENDING to init it
		)
	};

	struct DeltaNode {
		double									baseDt;
		uint32_t								initMs;
		float									ratio;			// BPM * Scale / 15000 -> {-16, 16, 1/131072}
	};
	struct DeltaGroup {
		std::vector<DeltaNode>					nodes;
		std::vector<DeltaNode>::const_iterator	it;
		double									dt;
	};

	struct Duo {
		Args64(
			union							  { float a;  uint32_t aa;  };
			union							  { float b;  uint32_t bb;  };
		)
	};
	struct ChildParam {
		Args32(
			int16_t								fromDegree:12 = 90;
			uint16_t							initRadius:8 = 0x7F;
			int16_t								toDegree:12 = 90;
		)
	};

	struct Wish {
		std::vector<Point>						nodes;
		std::vector<double>						childDts;
		std::vector<ChildParam>					childParams;
		//--------------------------------//
		std::vector<Point>::const_iterator		pIt;
		uint32_t								childIdx;
		//--------------------------------//
		uint32_t								deltaGroup;
	};
	struct Fumen {
		std::vector<Idx>						idxGroups;
		std::vector<DeltaGroup>					deltaGroups;
		std::vector<Wish>						wish;
		std::vector<Hint>						hint;
		std::vector<Echo>						echo;
		/*--------------------------------*/
		Args64(
			uint64_t							before:20;
			uint64_t							objectCount:15;
			uint64_t							wgoRequired:10 = 1023, hgoRequired:9, egoRequired:10;
		)
		//--------------------------------//
		uint64_t								msTime:20;
		uint64_t								hHit:15, eHit:15, early:15, late:15, lost:15, spJudged:9;
		uint64_t								daymode:1, judgeRange:7 = 37;
		int64_t									minDt:8, maxDt:8;
		/*--------------------------------*/
		float									objectSizeX = 360, objectSizeY = 360;
		float									xScale = 1, yScale = 1, xDelta, yDelta, rotSin, rotCos = 1;
		float									boundL = -36, boundR = 1836, boundU = 1116, boundD = -36;
		/*--------------------------------*/
		std::unordered_map<uint64_t, uint16_t>	lastWgo, lastEhgo;
		std::vector<Duo>						blocked;
	};
}
extern  Arf4::Fumen  Arf;
extern  int8_t		 InputDelta;

namespace Ar {
	using namespace Arf4;

	/* Internal */
	Duo  InterpolateEcho(const Echo& echo);
	Duo  InterpolatePosNode(const Point& currentPn, const Point& nextPn);
	void  PrecalculatePosNode(Point& currentPn);
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
	int  TransformStr(lua_State* L);
}