//  Aerials Fumen Player v4  //
#pragma once
#include <unordered_map>
#include <dmsdk/sdk.h>

namespace Arf4 {
	struct	Wish;
	#define	VCIT( T )	std::vector<T>::const_iterator
	#define	Ar32(...)	union{ struct{__VA_ARGS__}; int32_t whole; };
	#define	Ar64(...)	union{ struct{__VA_ARGS__}; int64_t whole; };
	using	H6416 =		std::unordered_map<uint64_t, uint16_t>;

	enum	TableIndex	{		WGO = 3, HGO, EGO, EHGO, AGOL, AGOR, WTINT, HTINT, ETINT, ATINT			 };
	enum	EaseType	{	 STATIC = 0, LINEAR, INSINE, OUTSINE, INQUAD, OUTQUAD,
						  CLOCKWISE = 5, COUNTERCLIOCKWISE = 10, CPOSITIVE = 5, CNEGATIVE = 10			 };
	enum	Status		{	NJUDGED = 0, NJUDGED_LIT, EARLY, EARLY_LIT, HIT, HIT_LIT, LATE, LATE_LIT,
							AUTO, LOST, SPECIAL, SPECIAL_LIT, SPECIAL_AUTO, SPECIAL_LOST, PENDING = -127 };

	// Primitive
	struct Point {
		float						x, y, t;
		Ar32(
			uint32_t				ce:10 = 0x3FF, ease:4 = LINEAR;
			uint32_t				ci:10;				// 8bit Padding here
		)
	};
	struct Echo {
		float						fromX, fromY, fromT, toX, toY, toT;
		Ar32(
			uint32_t				ce:8 = 0xFF, ease:4, status:4, ci:8;
			int32_t					deltaMs:8;
		)
	};
	union Hint {
		struct {
			float					x, y;
			Ar32(
				uint32_t			status:4, ms:20;	// Max 1048575ms -> 17.47625 mins
				int32_t				deltaMs:8;			// Set deltaMs = PENDING to init it
			)
		};
		struct {
			uint64_t				pWish:63, isSpecial:1;
			float					relT;
		};
	};

	// DeltaTime & WishChild
	struct DeltaNode {
		double						base;
		float						init;
		float						value;				// for SC: BPM * Scale / 15000 -> {-16, 16, 1/131072}
	};
	struct DeltaGroup {
		std::vector<DeltaNode>		nodes;
		VCIT(DeltaNode)				it;
		double						dt;
	};
	struct WishChild {
		double						dt;
		int16_t						fromDegree = 90, toDegree = 90;
		float						initRadius = 7.0f;
	};

	// Misc
	union FloatInDetail {								// Little Endian Only
		float						f;
		struct					  { uint32_t m:23, e:11, s:1; };
	};
	struct Duo {
		Ar64(
			union				  { float a;  uint32_t aa;  };
			union				  { float b;  uint32_t bb;  };
		)
	};
	struct Idx {
		std::vector<uint32_t>		wIdx;
		std::vector<uint16_t>		hIdx, eIdx;
	};

	// Fumen
	struct Wish {
		uint64_t					deltaGroup;
		std::vector<WishChild>		wishChilds;
		std::vector<Point>			nodes;
		VCIT(WishChild)				cIt;
		VCIT(Point)					pIt;
	};
	struct Fumen {
		std::vector<Idx>			idxGroups;
		std::vector<DeltaGroup>		deltaGroups;
		std::vector<Wish>			wish;
		std::vector<Hint>			hint;
		std::vector<Echo>			echo;
		Ar64(
			uint64_t				before:20, objectCount:15;
			uint64_t				wgoRequired:10 = 1023, hgoRequired:9, egoRequired:10;
		)
		//------------------------//
		uint64_t					msTime:20;
		uint64_t					hHit:15, eHit:15, early:15, late:15, lost:15, spJudged:9;
		uint64_t					daymode:1, judgeRange:7 = 37;
		int64_t						minDt:8, maxDt:8;
		/*------------------------*/
		float						objectSizeX = 360, objectSizeY = 360;
		float						xScale = 1, yScale = 1, xDelta, yDelta, rotSin, rotCos = 1;
		float						boundL = -36, boundR = 1836, boundU = 1116, boundD = -36;
		/*------------------------*/
		H6416						lastWgo, lastEhgo;
		std::vector<Duo>			blocked;
		/*------------------------*/
		#ifdef AR_BUILD_VIEWER							// Use new to create Helpers, and use
			struct Tempo {								//   delete to free them.
				float				init, beatBase, toneBase;
				uint16_t			a, b;				// Time Signature a / b
			};
			float					sinceTone;
			uint32_t				verseWidx:17, verseEidx:15;
			std::vector<Tempo>		tempoList;			std::vector<DeltaNode>		beatToMs;
			VCIT(Tempo)				tempoIt;			VCIT(DeltaNode)				beatIt;
		#endif
	};
}

namespace Ar {
	using namespace Arf4;

	/* Build */
	 int  NewBuild(lua_State* L);
	 int  NewDeltaGroup(lua_State* L);
	 int  NewVerse(lua_State* L);
	 int  MirrorLR(lua_State* L);
	 int  MirrorUD(lua_State* L);
	 int  NewHelper(lua_State* L);
	 int  NewChild(lua_State* L);
	 int  NewWish(lua_State* L);
	 int  NewHint(lua_State* L);
	 int  NewEcho(lua_State* L);
	 int  Move(lua_State* L);

	/* Internal */
	void  JudgeArfSweep() noexcept;
	 Duo  InterpolateEcho(const Echo& echo, float t) noexcept;
	 Duo  InterpolatePoint(Point thisPn, Point nextPn, float t) noexcept;
	 Duo  GetSinCosByDegree(FloatInDetail d) noexcept;

	/* Operation */
	 int  LoadArf(lua_State* L);
	 int  ExportArf(lua_State* L);
	 int  OrganizeArf(lua_State* L);
	 int  UpdateArf(lua_State* L);
	 int  JudgeArf(lua_State* L);

	/* Runtime Utils */
	 int  SetCam(lua_State* L);
	 int  SetBound(lua_State* L);
	 int  SetDaymode(lua_State* L);
	 int  SetObjectSize(lua_State* L);
	 int  SetJudgeRange(lua_State* L);
	 int  GetJudgeStat(lua_State* L);

	/* Other Utils */
	 int  NewTable(lua_State* L);
	 int  PushNullPtr(lua_State* L);
	 int  DoHapticFeedback(lua_State* L);
	 int  SetInputDelta(lua_State* L);
	 int  TransformStr(lua_State* L);
	 int  PartialEase(lua_State* L);
	 int  Ease(lua_State* L);
}

extern  Arf4::Fumen  Arf;
extern  int8_t		 InputDelta;