// Nemesis, the Aerials Fumen Compiler. //
#ifdef AR_BUILD_VIEWER
#include <algorithm>
#include <Arf4.h>
#include <map>

constexpr uint8_t WHEN_DXDY_ABOVEZERO[] = { 0,
	Arf4::LINEAR, Arf4::INSINE, Arf4::OUTSINE, Arf4::INQUAD, Arf4::OUTQUAD, Arf4::LINEAR + Arf4::CPOSITIVE,
	Arf4::INSINE + Arf4::CPOSITIVE, Arf4::OUTSINE + Arf4::CPOSITIVE, Arf4::INQUAD + Arf4::CPOSITIVE,
	Arf4::OUTQUAD + Arf4::CPOSITIVE, Arf4::LINEAR + Arf4::CNEGATIVE, Arf4::INSINE + Arf4::CNEGATIVE,
	Arf4::OUTSINE + Arf4::CNEGATIVE, Arf4::INQUAD + Arf4::CNEGATIVE, Arf4::OUTQUAD + Arf4::CNEGATIVE
};
constexpr uint8_t WHEN_DXDY_UNDERZERO[] = { 0,
	Arf4::LINEAR, Arf4::INSINE, Arf4::OUTSINE, Arf4::INQUAD, Arf4::OUTQUAD, Arf4::LINEAR + Arf4::CNEGATIVE,
	Arf4::INSINE + Arf4::CNEGATIVE, Arf4::OUTSINE + Arf4::CNEGATIVE, Arf4::INQUAD + Arf4::CNEGATIVE,
	Arf4::OUTQUAD + Arf4::CNEGATIVE, Arf4::LINEAR + Arf4::CPOSITIVE, Arf4::INSINE + Arf4::CPOSITIVE,
	Arf4::OUTSINE + Arf4::CPOSITIVE, Arf4::INQUAD + Arf4::CPOSITIVE, Arf4::OUTQUAD + Arf4::CPOSITIVE
};


/* Internal Fns */
static float barToTone(const float bar) noexcept {
	while( Arf.tempoIt != Arf.tempoList.cbegin()  &&  bar < (Arf.tempoIt-1)->init )
		--Arf.tempoIt;
	const auto lastIt = Arf.tempoList.cend() - 1;
	while( Arf.tempoIt != lastIt ) {
		if( bar < (Arf.tempoIt+1)->init )
			return Arf.tempoIt->toneBase + (bar - Arf.tempoIt->init) * Arf.tempoIt->a / Arf.tempoIt->b;
		++Arf.tempoIt;
	}
	if( Arf.tempoIt == lastIt )
		return lastIt->toneBase + (bar - lastIt->init) * lastIt->a / lastIt->b;
	return 0;
}

static float toneToBar(const float tone) noexcept {
	while( Arf.tempoIt != Arf.tempoList.cbegin()  &&  tone < (Arf.tempoIt-1)->toneBase )
		--Arf.tempoIt;
	const auto lastIt = Arf.tempoList.cend() - 1;
	while( Arf.tempoIt != lastIt ) {
		if( tone < (Arf.tempoIt+1)->toneBase )
			return Arf.tempoIt->init + (tone - Arf.tempoIt->toneBase) * Arf.tempoIt->b / Arf.tempoIt->a;
		++Arf.tempoIt;
	}
	if( Arf.tempoIt == lastIt )
		return lastIt->init + (tone - lastIt->toneBase) * lastIt->b / lastIt->a;
	return 0;
}

static float barToBeat(const float bar) noexcept {
	while( Arf.tempoIt != Arf.tempoList.cbegin()  &&  bar < (Arf.tempoIt-1)->init )
		--Arf.tempoIt;
	const auto lastIt = Arf.tempoList.cend() - 1;
	while( Arf.tempoIt != lastIt ) {
		if( bar < (Arf.tempoIt+1)->init )
			return Arf.tempoIt->beatBase + (bar - Arf.tempoIt->init) * Arf.tempoIt->a;
		++Arf.tempoIt;
	}
	if( Arf.tempoIt == lastIt )
		return lastIt->beatBase + (bar - lastIt->init) * lastIt->a;
	return 0;
}

static float beatToMs(const float beat) noexcept {
	while( Arf.beatIt != Arf.beatToMs.cbegin()  &&  beat < (Arf.beatIt-1)->init )
		--Arf.beatIt;
	const auto lastIt = Arf.beatToMs.cend() - 1;
	while( Arf.beatIt != lastIt ) {
		if( beat < (Arf.beatIt+1)->init )
			return Arf.beatIt->base + (beat - Arf.beatIt->init) * Arf.beatIt->value;
		++Arf.beatIt;
	}
	if( Arf.beatIt == lastIt )
		return lastIt->base + (beat - lastIt->init) * lastIt->value;
	return 0;
}

static float getX(Arf4::Wish* const o, const float t) noexcept {
	if( t <= o->nodes[0].t )
		return o->nodes[0].x;

	const auto lastNodeIt = o->nodes.cend() - 1;
	if( t >= lastNodeIt->t )
		return lastNodeIt->x;

	while( o->pIt != o->nodes.cbegin()  &&  t < o->pIt->t )
		-- o->pIt;
	while( o->pIt != lastNodeIt ) {
		const auto nextNodeIt = o->pIt + 1;
		if( t < nextNodeIt->t )
			return Ar::InterpolatePoint( *( o->pIt ), *nextNodeIt, t ).a;
		++ o-> pIt;
	}
	return 0;
}

static float getY(Arf4::Wish* const o, const float t) noexcept {
	if( t <= o->nodes[0].t )
		return o->nodes[0].y;

	const auto lastNodeIt = o->nodes.cend() - 1;
	if( t >= lastNodeIt->t )
		return lastNodeIt->y;

	while( o->pIt != o->nodes.cbegin()  &&  t < o->pIt->t )
		-- o->pIt;
	while( o->pIt != lastNodeIt ) {
		const auto nextNodeIt = o->pIt + 1;
		if( t < nextNodeIt->t )
			return Ar::InterpolatePoint( *( o->pIt ), *nextNodeIt, t ).b;
		++ o-> pIt;
	}
	return 0;
}


/* Lua State Utils */
static float checkBeat(lua_State* L, const int idx, const int n) {
	switch( lua_rawgeti(L,idx,n), lua_type(L,-1) ) {
		case LUA_TNUMBER: {
			float x = lua_tonumber(L, -1);
				  x = x<0 ? 0 : x;
			return lua_pop(L,1), barToBeat(
				toneToBar( Arf.sinceTone + (x<1 ? x*16 : x) )
			);
		}
		case LUA_TTABLE: {
			lua_rawgeti(L, -1, 1);
			lua_rawgeti(L, -2, 2);   // [-1] additionalTone  [-2] bar  [-3] inputTime

			float bar = luaL_checknumber(L, -2);
				  bar = bar<0 ? 0 : bar;
			Arf.sinceTone = barToTone(bar);

			float x = lua_isnumber(L, -1) ? lua_tonumber(L, -1) : 0;
				  x = x<0 ? 0 : x;
			return lua_pop(L,3), barToBeat(
				toneToBar( Arf.sinceTone + (x<1 ? x*16 : x) )
			);
		}
		default:
			return lua_pop(L,1), 0;
	}
}

static float checkX(lua_State* L, const int idx, const int n, const float t) {
	switch( lua_rawgeti(L,idx,n), lua_type(L,-1) ) {
		case LUA_TNUMBER: {
			float x = lua_tonumber(L, -1) * 112.5 - 900.0;
				  x = x < -16200 ? -16200 : x > 16200 ? 16200 : x;
			return lua_pop(L,1), x;
		}
		case LUA_TSTRING: {   // Mirror
			float x = 900.0 - lua_tonumber(L, -1) * 112.5;
				  x = x < -16200 ? -16200 : x > 16200 ? 16200 : x;
			return lua_pop(L,1), x;
		}
		case LUA_TUSERDATA: {
			// Is it a Wish or a Helper?
			lua_getmetatable(L, -1);
			lua_rawgeti(L, -1, 0xADD8E6);   // [-1] Result  [-2] Metatable  [-3] Userdata
			if( lua_isnil(L, -1) )
				return lua_pop(L,3), 0;

			// Use this
			const auto pWish = (Arf4::Wish*)lua_touserdata(L, -3);
			return lua_pop(L,3), getX(pWish,t);
		}
		default:
			return lua_pop(L,1), 0;
	}
}

static float checkY(lua_State* L, const int idx, const int n, const float t) {
	switch( lua_rawgeti(L,idx,n), lua_type(L,-1) ) {
		case LUA_TNUMBER: {
			float y = lua_tonumber(L, -1) * 112.5 - 450.0;
				  y = y < -8100 ? -8100 : y > 8100 ? 8100 : y;
			return lua_pop(L,1), y;
		}
		case LUA_TSTRING: {   // Mirror
			float y = 450.0 - lua_tonumber(L, -1) * 112.5;
				  y = y < -8100 ? -8100 : y > 8100 ? 8100 : y;
			return lua_pop(L,1), y;
		}
		case LUA_TUSERDATA: {
			// Is it a Wish or a Helper?
			lua_getmetatable(L, -1);
			lua_rawgeti(L, -1, 0xADD8E6);   // [-1] Result  [-2] Metatable  [-3] Userdata
			if( lua_isnil(L, -1) )
				return lua_pop(L,3), 0;

			// Use this
			const auto pWish = (Arf4::Wish*)lua_touserdata(L, -3);
			return lua_pop(L,3), getY(pWish,t);
		}
		default:
			return lua_pop(L,1), 0;
	}
}

static Arf4::Duo checkEase(lua_State* L, const int idx, const int n, uint8_t* easeType) {
	switch( lua_rawgeti(L,idx,n), lua_type(L,-1) ) {
		case LUA_TNUMBER: {
			const uint8_t type = lua_tointeger(L, -1);
					 *easeType = type > 15 ? Arf4::LINEAR : type;
			break;
		}
		case LUA_TTABLE: {
			lua_rawgeti(L, -1, 1);
			lua_rawgeti(L, -2, 2);
			lua_rawgeti(L, -3, 3);   // [-1] ce  [-2] ci  [-3] type  [-4] ease table
			const uint8_t type = luaL_checkinteger(L, -1);
					 *easeType = type > 15 ? Arf4::LINEAR : type;

			Arf4::FloatInDetail ci, ce;
			ci.f = lua_isnumber(L, -2) ? (float)lua_tonumber(L, -2) : 0.0f;
			ce.f = lua_isnumber(L, -1) ? (float)lua_tonumber(L, -1) : 1.0f;
			ci.f = ci.f < 0.0f ? 0.0f : ci.f > 1.0f ? 1.0f : ci.f;
			ce.f = ce.f < 0.0f ? 0.0f : ce.f > 1.0f ? 1.0f : ce.f;
			if( ci.f > ce.f ) {
				const float temp = ci.f;
				ci.f = ce.f, ce.f = temp;
			}
			lua_pop(L, 4);
			return { .aa = (ci.e+=10, (uint32_t)ci.f), .bb = (ce.e+=10, (uint32_t)ce.f) };
		}
		default:;
	}
	lua_pop(L, 1);
	return { .aa = 0, .bb = 0 };
}

static int helperGcMethod(lua_State* L) {
	( (Arf4::Wish*)lua_touserdata(L,1) ) -> ~Wish();
	return 0;
}


/* APIs */
int Ar::NewBuild(lua_State* L) {
	/* Usage:
	 * Time {					-- For 4/4-only tracks
	 *     Offset = 0,			-- Beat 0 starts from 0ms
	 *     0, 170,				-- Bar, BPM
	 *     ···
	 * }
	 * Time {					-- For tracks with Tempo Variations
	 *     Offset = 0,
	 *     Tempo = {
	 *         0, 4, 4,			-- Bar, Beat Count of a Bar, How many Beats are equal in length to an Tone
	 *         1, 3, 4,
	 *         25, 4, 4
	 *     },
	 *     0, 0, 201,			-- Bar(to be converted to Beat), Additional Beats, BPM
	 *     ···
	 * }
	 */
	double offset = 0;
	if( lua_getfield(L, 1, "Offset"), lua_isnumber(L, -1) )
		offset = lua_tonumber(L, -1);
	lua_pop(L, 1);

	// Input: Tempo
	std::map<float, Fumen::Tempo> tempoMap;
	if( size_t tempoInputLen = ( lua_getfield(L, 1, "Tempo"), 0 );
		lua_istable(L,-1)  &&  ( tempoInputLen = lua_objlen(L,-1) ) > 2
	) for( size_t i = 1; i < tempoInputLen; i+=3 ) {   // [1] Args Table  [2] Tempo Table
		float bar = ( lua_rawgeti(L, 2, i), luaL_checknumber(L, -1) );
			  bar = bar<0 ? 0 : bar;
		uint16_t a = ( lua_rawgeti(L, 2, i+1), luaL_checkinteger(L, -1) );
				 a = a<1 ? 1 : a;
		uint16_t b = ( lua_rawgeti(L, 2, i+2), luaL_checkinteger(L, -1) );
				 b = b<1 ? 1 : b;
		tempoMap[bar] = { .init = bar, .a = a, .b = b };
		lua_pop(L, 3);
	}
	lua_pop(L, 1);

	// Input: BPM
	std::vector<float> bpmInputs;
	const size_t bpmInputLen = lua_objlen(L, 1);
	bpmInputs.reserve( bpmInputLen );
	for( size_t i = 1; i <= bpmInputLen; ++i )
		bpmInputs.push_back(( lua_rawgeti(L, 1, i), luaL_checknumber(L, -1) )),
		lua_pop(L, 1);

	Arf = {};
	if( tempoMap.empty() ) {
		Arf.tempoList.push_back({ .init = 0, .a = 4, .b = 4 });
		Arf.tempoIt = Arf.tempoList.cbegin();

		if( bpmInputLen > 1  &&  bpmInputLen % 2 == 0 ) {
			Arf.beatToMs.reserve( bpmInputLen / 2 );
			for( size_t i = 0; i < bpmInputLen; ++i ) {
				float beat = bpmInputs[i] * 4;
					  beat = beat < 0 ? 0 : beat;
				float bpm = bpmInputs[++i];
					  bpm = bpm > 0 ? bpm : 170;
				Arf.beatToMs.push_back({ .init = beat, .value = 60000.0f / bpm });
			}
		}
	}
	else {
		const size_t tempoCount = tempoMap.size();
		Arf.tempoList.reserve(tempoCount);

		for( const auto [_, tempo] : tempoMap )
			Arf.tempoList.push_back(tempo);
		Arf.tempoList[0].init = 0;

		for( size_t i = 1; i < tempoCount; ++i ) {
			const auto  lastTempo = Arf.tempoList[i-1];
				  auto& thisTempo = Arf.tempoList[i];
			const float deltaBar = thisTempo.init - lastTempo.init;
			thisTempo.toneBase = lastTempo.toneBase + deltaBar * lastTempo.a / lastTempo.b;
			thisTempo.beatBase = lastTempo.beatBase + deltaBar * lastTempo.a;
		}
		Arf.tempoIt = Arf.tempoList.cbegin();

		if( bpmInputLen > 2  &&  bpmInputLen % 3 == 0 ) {
			Arf.beatToMs.reserve( bpmInputLen / 3 );
			for( size_t i = 0; i < bpmInputLen; ++i ) {
				float bar = bpmInputs[i];
					  bar = bar < 0 ? 0 : bar;
				float abt = bpmInputs[++i];
					  abt = abt < 0 ? 0 : abt;
				float bpm = bpmInputs[++i];
					  bpm = bpm > 0 ? bpm : 170;
				Arf.beatToMs.push_back({ .init = barToBeat(bar) + abt, .value = 60000.0f / bpm });
			}
		}
	}

	switch( Arf.beatToMs.size() ) {
		case 0:
			Arf.beatToMs.push_back({ .base = offset, .init = 0, .value = 60000 / 170.0f });
			break;
		case 1:
			Arf.beatToMs[0].init = 0;
			Arf.beatToMs[0].base = offset;
			break;
		default: {
			std::map<float, DeltaNode> beatToMsMap;
			for( const auto node : Arf.beatToMs )
				beatToMsMap[ node.init ] = node;
			Arf.beatToMs.clear();

			for( const auto [_, bpm] : beatToMsMap )
				Arf.beatToMs.push_back(bpm);
			Arf.beatToMs[0].init = 0;
			Arf.beatToMs[0].base = offset;

			const size_t bpmCount = Arf.beatToMs.size();
			for( size_t i = 1; i < bpmCount; ++i ) {
				const auto  lastBpm = Arf.beatToMs[i-1];
					  auto& thisBpm= Arf.beatToMs[i];
				thisBpm.base = lastBpm.base + (thisBpm.init - lastBpm.init) * lastBpm.value;
			}
		}
	}
	Arf.beatIt = Arf.beatToMs.cbegin();
	return 0;
}

int Ar::NewDeltaGroup(lua_State* L) {
	/* Usage:
	 * local my_1st_group = DeltaGroup {	-- Returns the idx(0 here)
	 *     {0},			1,					-- Bar 0, Ratio: 1
	 *     {2, 1/32},	-1,					-- Bar 2, then 1/32 Tone, Ratio: -1
	 *     {2, 1},		0.9,				-- Bar 2, then 1/16 Tone, Ratio: 0.9
	 *     15,			1,					-- Bar 2(Cached), then 15/16 Tone, Ratio: 1
	 *     ···
	 * }
	 */
	std::map<float, float> deltaMap;
	const size_t inputLen = lua_objlen(L, 1);
	for( size_t i = 1; i < inputLen; i+=2 ) {
		const float ms = beatToMs( checkBeat(L, 1, i) );
		float ratio = ( lua_rawgeti(L, 1, i+1), luaL_checknumber(L, -1) ) * 170 / 15000;
			  ratio = ratio < -16 ? -16 : ratio > 16 ? 16 : ratio;
		deltaMap[ms] = ratio;
		lua_pop(L, 1);
	}
	if( deltaMap.empty() )
		deltaMap[0] = 170.0f / 15000;

	const size_t idx = Arf.deltaGroups.size();
	auto& thisGroup = ( Arf.deltaGroups.push_back({}), Arf.deltaGroups[idx] );
		  thisGroup.nodes.reserve( deltaMap.size() );
	for( const auto [ms, ratio] : deltaMap )
		thisGroup.nodes.push_back({ .init = ms, .value = ratio });
	thisGroup.nodes[0].init = 0;

	const size_t groupLen = thisGroup.nodes.size();
	for( size_t i = 1; i < groupLen; ++i ) {
		const auto  lastNode = thisGroup.nodes[i-1];
			  auto& thisNode = thisGroup.nodes[i];
		thisNode.base = lastNode.base + (thisNode.init - lastNode.init) * lastNode.value;
	}
	Arf.deltaGroups[idx].it = thisGroup.nodes.cbegin();

	lua_pushinteger(L, idx);
	return 1;
}

int Ar::NewWish(lua_State* L) {
	/* Usage:
	 * local myWish = Wish {	-- When failed, a nil will be returned.
	 *     DeltaGroup = 1,		-- 0 by default
	 *     {1}, 4, 3, LINEAR,	-- Bar 1, X=4, Y=3, Linear Ease
	 *
	 *     -- Time k(Bar 1, then 12/16 Tone), X=getX(oldWish, k), Y=getY(oldWish, k),
	 *     --   Clockwise Circular Motion with an uniform angular velocity, CurveInit 0.4, CurveEnd 0.9
	 *     12, oldWish, oldWish, {LINEAR+CLOCKWISE, 0.4, 0.9},
	 *     ···
	 * }
	 */
	size_t whichDeltaGroup = 0;
	if( lua_getfield(L, 1, "DeltaGroup"), lua_isnumber(L, -1) ) {
		whichDeltaGroup = lua_tointeger(L, -1);
		whichDeltaGroup = whichDeltaGroup < Arf.deltaGroups.size() ? whichDeltaGroup : 0;
	}
	lua_pop(L, 1);

	std::map<float, Point> nodeMap;
	const size_t inputLen = lua_objlen(L, 1);
	for( size_t i = 1; i < inputLen; i+=4 ) {
		uint8_t easeType = LINEAR;
		const Duo   cice = checkEase(L, 1, i+3, &easeType);
		const float beat = checkBeat(L, 1, i);
		nodeMap[beat] = {
			.t = beat,
			.x = checkX(L, 1, i+1, beat),
			.y = checkY(L, 1, i+2, beat),
			.ease = easeType, .ci = cice.aa, .ce = cice.bb
		};
	}
	if( nodeMap.size() < 2 )
		return lua_pushnil(L), 1;

	auto& newWish = (
		Arf.wish.push_back({ .deltaGroup = whichDeltaGroup }),
		Arf.wish.back()
	);
	newWish.nodes.reserve( nodeMap.size() );
	for( const auto [_, node] : nodeMap )
		newWish.nodes.push_back(node);
	newWish.pIt = newWish.nodes.cbegin();

	// Manage EaseType
	const size_t lastNodeIdx = newWish.nodes.size() - 1;
	for( size_t i = 0; i < lastNodeIdx; ++i ) {
		const auto  nextNode = newWish.nodes[i+1];
			  auto& thisNode = newWish.nodes[i];
		if( (nextNode.x - thisNode.x) * (nextNode.y - thisNode.y) < 0 )
			thisNode.ease = WHEN_DXDY_UNDERZERO[ thisNode.ease ];
		else
			thisNode.ease = WHEN_DXDY_ABOVEZERO[ thisNode.ease ];
	}
	auto& lastNode = newWish.nodes[lastNodeIdx];
		  lastNode.ease = 0, lastNode.ci = 0, lastNode.ce = 1023;

	// Do Return
	lua_pushlightuserdata(L, &newWish);		// [2]
	lua_newtable(L);						// [3]
	lua_pushboolean(L, true);
	lua_rawseti(L, 3, 0xADD8E6);
	lua_setmetatable(L, 2);
	return 1;
}

int Ar::NewHelper(lua_State* L) {
	/* Usage:
	 * local myHelperOrNil = Helper {	-- For getX/getY usages only.
	 *     {1}, 4, 3, LINEAR,			-- Bar 1, X=4, Y=3, Linear Ease
	 *     12, 8, 9, LINEAR,			-- Bar 1 then 12/16 Tone, X=8, Y=9, Linear Ease
	 *     ···
	 * }
	 */
	std::map<float, Point> nodeMap;
	const size_t inputLen = lua_objlen(L, 1);
	for( size_t i = 1; i < inputLen; i+=4 ) {
		uint8_t easeType = LINEAR;
		const Duo   cice = checkEase(L, 1, i+3, &easeType);
		const float beat = checkBeat(L, 1, i);
		nodeMap[beat] = {
			.t = beat,
			.x = checkX(L, 1, i+1, beat),
			.y = checkY(L, 1, i+2, beat),
			.ease = easeType, .ci = cice.aa, .ce = cice.bb
		};
	}
	if( nodeMap.size() < 2 )
		return lua_pushnil(L), 1;

	const auto pNewHelper = new (lua_newuserdata(L, sizeof(Wish))) Wish;			// [2]
	pNewHelper->nodes.reserve( nodeMap.size() );
	for( const auto [_, node] : nodeMap )
		pNewHelper->nodes.push_back(node);
	pNewHelper->pIt = pNewHelper->nodes.cbegin();

	// Manage EaseType
	const size_t lastNodeIdx = pNewHelper->nodes.size() - 1;
	for( size_t i = 0; i < lastNodeIdx; ++i ) {
		const auto  nextNode = pNewHelper->nodes[i+1];
			  auto& thisNode = pNewHelper->nodes[i];
		if( (nextNode.x - thisNode.x) * (nextNode.y - thisNode.y) < 0 )
			thisNode.ease = WHEN_DXDY_UNDERZERO[ thisNode.ease ];
		else
			thisNode.ease = WHEN_DXDY_ABOVEZERO[ thisNode.ease ];
	}
	auto& lastNode = pNewHelper->nodes[lastNodeIdx];
		  lastNode.ease = 0, lastNode.ci = 0, lastNode.ce = 1023;

	// Do Return
	lua_newtable(L);																// [3]
	lua_pushboolean(L, false);
	lua_rawseti(L, 3, 0xADD8E6);
	lua_pushcfunction(L, helperGcMethod);
	lua_setfield(L, 3, "__gc");
	lua_setmetatable(L, 2);
	return 1;
}

int Ar::NewChild(lua_State* L) {
	/* Usage:
	 * Child {
	 *     Wish = myWish,			-- The last Wish of the Fumen by default
	 *     Angle = 90,				-- degreeNum or {fromDeg, toDeg}, 90 by default
	 *     Radius = 7.0,			-- 7.0 by Default
	 *     Special = false,			-- False by default
	 *     {1, 1}, 2, 3, 4, ···		-- Times
	 * }
	 */
	if( Arf.wish.empty() )
		return 0;
	const bool isSpecial = ( lua_getfield(L, 1, "Special"), lua_toboolean(L, -1) );
	lua_pop(L, 1);

	float radius = 7.0f;
	if( lua_getfield(L, 1, "Radius"), lua_isnumber(L, -1) ) {
		radius = lua_tonumber(L, -1);
		radius = radius < 0 ? 7 : radius > 16 ? 7 : radius;
	}
	lua_pop(L, 1);

	int16_t fromDeg = 90, toDeg = 90;
	switch( lua_getfield(L, 1, "Angle"), lua_type(L, -1) ) {
		case LUA_TNUMBER:
			fromDeg = lua_tointeger(L, -1);
			fromDeg = fromDeg > 65355 ? 65355 : fromDeg < -65355 ? -65355 : fromDeg;
			toDeg = fromDeg;
			break;
		case LUA_TTABLE: {
			lua_rawgeti(L, 2, 1);
			lua_rawgeti(L, 2, 2);   // [2] Angle  [3] fromDeg  [4] toDeg
			fromDeg = lua_isnumber(L, 3) ? lua_tointeger(L, 3) : 90;
			fromDeg = fromDeg > 65355 ? 65355 : fromDeg < -65355 ? -65355 : fromDeg;
			toDeg = lua_isnumber(L, 4) ? lua_tointeger(L, 4) : 90;
			toDeg = toDeg > 65355 ? 65355 : toDeg < -65355 ? -65355 : toDeg;
			lua_pop(L, 2);
		}
		default:;
	}
	lua_pop(L, 1);

	Wish* pWish;
	if( lua_getfield(L, 1, "Wish"), lua_isuserdata(L, -1) ) {
		if( lua_getmetatable(L, -1), lua_rawgeti(L, -1, 0xADD8E6), lua_toboolean(L, -1) )
			// [2] Wish  [3] Wish Metatable  [4] isWish
			pWish = (Wish*)lua_touserdata(L, 2);
		else
			pWish = &Arf.wish.back();
		lua_pop(L, 3);
	}
	else {
		pWish = &Arf.wish.back();
		lua_pop(L, 1);
	}
	const auto minT = pWish->nodes[0].t, maxT = pWish->nodes.back().t;

	std::map<float, uint8_t> beats;
	size_t beatCnt = lua_objlen(L, 1);
	for( size_t i = 1; i <= beatCnt; ++i )
		beats[ checkBeat(L, 1, i) ] = 0;
	beatCnt = beats.size();

	Arf.hint.reserve(beatCnt);
	pWish->wishChilds.reserve(beatCnt);
	for( const auto [beat, _] : beats ) {
		if( beat >= minT  &&  beat <= maxT )
			Arf.hint.push_back( {
				.pWish = reinterpret_cast<uint64_t>(pWish),
				.isSpecial = isSpecial,
				.relT = beat - minT,
			});
		pWish->wishChilds.push_back({
			.dt = beat,   // A Interval Value
			.fromDegree = fromDeg, .toDegree = toDeg, .initRadius = radius
		});
	}
	pWish->cIt = pWish->wishChilds.cbegin();
	return 0;
}

int Ar::NewHint(lua_State* L) {
	/* Usage:
	 * Hint {
	 *     Wish = myWish,						-- The last Wish of the Fumen by default
	 *     Special = false,						-- False by default
	 *     {1}, 1, 2, 3, 4, ···					-- Times
	 * }
	 */
	if( Arf.wish.empty() )
		return 0;
	const bool isSpecial = ( lua_getfield(L, 1, "Special"), lua_toboolean(L, -1) );
	lua_pop(L, 1);

	Wish* pWish;
	if( lua_getfield(L, 1, "Wish"), lua_isuserdata(L, -1) ) {
		if( lua_getmetatable(L, -1), lua_rawgeti(L, -1, 0xADD8E6), lua_toboolean(L, -1) )
			// [2] Wish  [3] Wish Metatable  [4] isWish
			pWish = (Wish*)lua_touserdata(L, 2);
		else
			pWish = &Arf.wish.back();
		lua_pop(L, 3);
	}
	else {
		pWish = &Arf.wish.back();
		lua_pop(L, 1);
	}
	const auto minT = pWish->nodes[0].t, maxT = pWish->nodes.back().t;

	std::map<float, uint8_t> beats;
	size_t beatCnt = lua_objlen(L, 1);
	for( size_t i = 1; i <= beatCnt; ++i )
		beats[ checkBeat(L, 1, i) ] = 0;
	beatCnt = beats.size();

	Arf.hint.reserve(beatCnt);
	pWish->wishChilds.reserve(beatCnt);
	for( const auto [beat, _] : beats ) {
		if( beat >= minT  &&  beat <= maxT )
			Arf.hint.push_back( {
				.pWish = reinterpret_cast<uint64_t>(pWish),
				.isSpecial = isSpecial,
				.relT = beat - minT,
			});
	}
	return 0;
}

int Ar::NewEcho(lua_State* L) {
	/* Usage:
	 * Echo {
	 *     Real = true,							-- False by Default
	 *     From = {_, 8, 7.5, LINEAR}			-- FromT, FromX, FromY, Ease. All of them are nilable.
	 *     {1}, 8, 0.5,							-- Time 1, ToX 1, ToY 1
	 *     12, 8, 0.5,							-- Time 2, ToX 2, ToY 2
	 *     ···
	 * }
	 */
	float fromX = 0, fromY = 0, fromT = -0xADD8E6;
	uint8_t hasFromX = 0, hasFromY = 0, easeType = LINEAR, status = AUTO;
	Duo cice = { .aa = 0, .bb = 1023 };

	if( lua_getfield(L, 1, "Real"), lua_toboolean(L, -1) )
		status = SPECIAL_AUTO;
	lua_pop(L, 1);

	if( lua_getfield(L, 1, "From"), lua_istable(L, -1) ) {   // [2] From Args
		if( lua_rawgeti(L, 2, 1), !lua_isnil(L, -1) )
			fromT = checkBeat(L, 2, 1);
		if( lua_rawgeti(L, 2, 2), !lua_isnil(L, -1) )
			fromX = checkX(L, 2, 2, fromT),   // Usually nodes[0].x when fromT is nil
			hasFromX = 1;
		if( lua_rawgeti(L, 2, 3), !lua_isnil(L, -1) )
			fromY = checkY(L, 2, 3, fromT),
			hasFromY = 2;
		if( lua_rawgeti(L, 2, 4), !lua_isnil(L, -1) )
			cice = checkEase(L, 2, 4, &easeType);
		cice.aa >>= 2, cice.bb >>= 2;
		lua_pop(L, 4);
	}
	lua_pop(L, 1);

	const size_t inputLen = lua_objlen(L, 1);
	switch( hasFromX + hasFromY ) {
		case 0:   // False, False
			for( size_t i = 1; i < inputLen; i+=3 ) {
				const float toT = checkBeat(L, 1, i);
				const float toX = checkX(L, 1, i+1, toT),
							toY = checkY(L, 1, i+2, toT);
				if( fromT <= toT )
					Arf.echo.push_back({
						.fromX = toX, .fromY = toY, .fromT = fromT, .toX = toX, .toY = toY, .toT = toT,
						.ease = easeType, .status = status, .ci = cice.aa, .ce = cice.bb, .deltaMs = 0
					});
				else
					Arf.echo.push_back({
						.fromX = toX, .fromY = toY, .fromT = toT, .toX = toX, .toY = toY, .toT = fromT,
						.ease = easeType, .status = status, .ci = cice.aa, .ce = cice.bb, .deltaMs = 0
					});
			}
			break;
		case 1:   // True, False
			for( size_t i = 1; i < inputLen; i+=3 ) {
				const float toT = checkBeat(L, 1, i);
				const float toX = checkX(L, 1, i+1, toT),
							toY = checkY(L, 1, i+2, toT);
				if( fromT <= toT )
					Arf.echo.push_back({
						.fromX = fromX, .fromY = toY, .fromT = fromT, .toX = toX, .toY = toY, .toT = toT,
						.ease = easeType, .status = status, .ci = cice.aa, .ce = cice.bb, .deltaMs = 0
					});
				else
					Arf.echo.push_back({
						.fromX = fromX, .fromY = toY, .fromT = toT, .toX = toX, .toY = toY, .toT = fromT,
						.ease = easeType, .status = status, .ci = cice.aa, .ce = cice.bb, .deltaMs = 0
					});
			}
			break;
		case 2:   // False, True
			for( size_t i = 1; i < inputLen; i+=3 ) {
				const float toT = checkBeat(L, 1, i);
				const float toX = checkX(L, 1, i+1, toT),
							toY = checkY(L, 1, i+2, toT);
				if( fromT <= toT )
					Arf.echo.push_back({
						.fromX = toX, .fromY = fromY, .fromT = fromT, .toX = toX, .toY = toY, .toT = toT,
						.ease = easeType, .status = status, .ci = cice.aa, .ce = cice.bb, .deltaMs = 0
					});
				else
					Arf.echo.push_back({
						.fromX = toX, .fromY = fromY, .fromT = toT, .toX = toX, .toY = toY, .toT = fromT,
						.ease = easeType, .status = status, .ci = cice.aa, .ce = cice.bb, .deltaMs = 0
					});
			}
			break;
		case 3:   // True, True
			for( size_t i = 1; i < inputLen; i+=3 ) {
				const float toT = checkBeat(L, 1, i);
				const float toX = checkX(L, 1, i+1, toT),
							toY = checkY(L, 1, i+2, toT);
				if( fromT <= toT )
					Arf.echo.push_back({
						.fromX = fromX, .fromY = fromY, .fromT = fromT, .toX = toX, .toY = toY, .toT = toT,
						.ease = easeType, .status = status, .ci = cice.aa, .ce = cice.bb, .deltaMs = 0
					});
				else
					Arf.echo.push_back({
						.fromX = fromX, .fromY = fromY, .fromT = toT, .toX = toX, .toY = toY, .toT = fromT,
						.ease = easeType, .status = status, .ci = cice.aa, .ce = cice.bb, .deltaMs = 0
					});
			}
		default:;
	}
	return 0;
}

int Ar::NewVerse(lua_State*) noexcept {
	/* Usage:
	 * NewVerse()
	 */
	Arf.verseWidx = Arf.wish.size();
	Arf.verseEidx = Arf.echo.size();
	return 0;
}

int Ar::MirrorLR(lua_State*) noexcept {
	/* Usage:
	 * MirrorLR()
	 */
	size_t sz = Arf.wish.size();
	for( size_t i = Arf.verseWidx; i < sz; ++i ) {
		for( auto& node : Arf.wish[i].nodes )
			node.x = -node.x;
		for( auto& child : Arf.wish[i].wishChilds )
			child.fromDegree = 180 - child.fromDegree,
			child.toDegree = 180 - child.toDegree;
	}

	sz = Arf.hint.size();
	for( size_t i = Arf.verseEidx; i < sz; ++i ) {
		auto& echo = Arf.echo[i];
			  echo.fromX = -echo.fromX, echo.toX = -echo.toX;
	}
	return 0;
}

int Ar::MirrorUD(lua_State*) noexcept {
	/* Usage:
	 * MirrorUD()
	 */
	size_t sz = Arf.wish.size();
	for( size_t i = Arf.verseWidx; i < sz; ++i ) {
		for( auto& node : Arf.wish[i].nodes )
			node.y = -node.y;
		for( auto& child : Arf.wish[i].wishChilds )
			child.fromDegree = -child.fromDegree,
			child.toDegree = -child.toDegree;
	}

	sz = Arf.echo.size();
	for( size_t i = Arf.verseEidx; i < sz; ++i ) {
		auto& echo = Arf.echo[i];
			  echo.fromY = -echo.fromY, echo.toY = -echo.toY;
	}
	return 0;
}

int Ar::Move(lua_State* L) {
	/* Usage:
	 * Move( {0} )			-- New Init Position(to be converted to Beat)
	 */
	lua_rawseti(L, LUA_REGISTRYINDEX, 0xADD8E6);
	const float newInitBeat = checkBeat(L, LUA_REGISTRYINDEX, 0xADD8E6);
	lua_pushnil(L), lua_rawseti(L, LUA_REGISTRYINDEX, 0xADD8E6);

	float beatDelta = 0xADD8E6;
	for( size_t i = Arf.verseWidx, ws = Arf.wish.size(); i < ws; ++i ) {
		const float firstBeat = Arf.wish[i].nodes[0].t;
		beatDelta = beatDelta > firstBeat ? firstBeat : beatDelta;
	}
	for( size_t i = Arf.verseEidx, es = Arf.echo.size(); i < es; ++i ) {
		const float fromBeat = Arf.echo[i].fromT;
		beatDelta = fromBeat != -0xADD8E6  &&  beatDelta > fromBeat  ?  fromBeat : beatDelta;
	}
	beatDelta = newInitBeat - beatDelta;

	for( size_t i = Arf.verseWidx, ws = Arf.wish.size(); i < ws; ++i ) {
		auto& wish = Arf.wish[i];
		for( auto& node : wish.nodes )
			node.t += beatDelta;
		for( auto& child : wish.wishChilds )
			child.dt += beatDelta;
	}
	for( size_t i = Arf.verseEidx, es = Arf.echo.size(); i < es; ++i ) {
		auto& echo = Arf.echo[i];
		echo.fromT += beatDelta, echo.toT += beatDelta;
	}
	return 0;
}


/* Fumen Operation Fn */
int Ar::OrganizeArf(lua_State*) noexcept {
	// NYI
	return 0;
}
#endif