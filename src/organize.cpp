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

static double beatToDt(double t, const uint16_t whichDeltaGroup) {
	if( t = beatToMs(t), t < 0 )
		return 0;
	auto& deltaGroup = Arf.deltaGroups[whichDeltaGroup];
	while( deltaGroup.it != deltaGroup.nodes.cbegin()  &&  t < deltaGroup.it->init )
		--deltaGroup.it;

	const auto lastIt = deltaGroup.nodes.cend() - 1;
	while( deltaGroup.it != lastIt ) {
		if( t < (deltaGroup.it+1)->init ) {
			deltaGroup.dt = deltaGroup.it->base + (t - deltaGroup.it->init) * deltaGroup.it->value;
			return deltaGroup.dt;
		}
		++deltaGroup.it;
	}

	if( deltaGroup.it == lastIt )
		deltaGroup.dt = lastIt->base + (t - lastIt->init) * lastIt->value;
	return deltaGroup.dt;
}

static Arf4::Duo getXY(Arf4::Wish* const o, const float t) noexcept {
	const auto init = o->nodes[0], last = o->nodes.back();
	if( t <= init.t )		return { .a = init.x, .b = init.y };
	if( t >= last.t )		return { .a = last.x, .b = last.y };

	const auto lastNodeIt = o->nodes.cend() - 1;
	while( o->pIt != o->nodes.cbegin()  &&  t < o->pIt->t )
		-- o->pIt;
	while( o->pIt != lastNodeIt ) {
		const auto nextNodeIt = o->pIt + 1;
		if( t < nextNodeIt->t )
			return Ar::InterpolatePoint( *( o->pIt ), *nextNodeIt, t );
		++ o-> pIt;
	}
	return {};
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
		default:;
	}
	return lua_pop(L,1), 0;
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
		case LUA_TTABLE: {   // Wish(Time) -> {x, y}
			lua_rawgeti(L, -1, 1);
			float x = lua_tonumber(L, -1) * 112.5 - 900.0;
				  x = x < -16200 ? -16200 : x > 16200 ? 16200 : x;
			return lua_pop(L,2), x;
		}
		case LUA_TUSERDATA: {
			// Is it a Wish or a Helper?
			lua_getmetatable(L, -1);
			lua_rawgeti(L, -1, 0xADD8E6);   // [-1] Result  [-2] Metatable  [-3] Userdata
			if( lua_isnil(L, -1) )
				return lua_pop(L,3), 0;

			// Use this
			const auto pWish = (Arf4::Wish*)lua_touserdata(L, -3);
			return lua_pop(L,3), getXY(pWish, t).a;
		}
		default:;
	}
	return lua_pop(L,1), 0;
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
		case LUA_TTABLE: {   // Wish(Time) -> {x, y}
			lua_rawgeti(L, -1, 2);
			float y = lua_tonumber(L, -1) * 112.5 - 450.0;
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
			return lua_pop(L,3), getXY(pWish, t).b;
		}
		default:;
	}
	return lua_pop(L,1), 0;
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
			const uint8_t type = luaL_checkinteger(L, -3);
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
		default:
			*easeType = Arf4::LINEAR;
	}
	lua_pop(L, 1);
	return { .aa = 0, .bb = 0 };
}

static int scriptGetXY(lua_State* L) {
	/* Usage:
	 * local x1, y1 = table.unpack( myWish(0) )
	 * local x2, y2 = table.unpack( myWish{1,1/16} )
	 */
	const auto w = (Arf4::Wish*)lua_touserdata(L, -1);
	lua_rawseti(L, LUA_REGISTRYINDEX, 0xADD8E6);

	const float t = checkBeat(L, LUA_REGISTRYINDEX, 0xADD8E6);
	lua_pushnil(L), lua_rawseti(L, LUA_REGISTRYINDEX, 0xADD8E6);

	const Arf4::Duo xy = getXY(w, t);
	lua_createtable(L, 2, 0);   // [2]
	lua_pushnumber(L, xy.a), lua_rawseti(L, 2, 1);
	lua_pushnumber(L, xy.b), lua_rawseti(L, 2, 2);
	return 1;
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

static std::map<float, float> deltaMap;
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
	if( Arf.deltaGroups.size() > 65535 )
		return lua_pushinteger(L, 0), 1;
	deltaMap.clear();

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

static std::map<float, Arf4::Point>	nodeMap;
int Ar::NewWish(lua_State* L) {
	/* Usage:
	 * local myWish = Wish {	-- When failed, a nil will be returned.
	 *     Special = true,		-- false by default
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
	const bool isSpecial = ( lua_getfield(L, 1, "Special"), lua_toboolean(L, -1) );
	if( lua_getfield(L, 1, "DeltaGroup"), lua_isnumber(L, -1) ) {
		whichDeltaGroup = lua_tointeger(L, -1);
		whichDeltaGroup = whichDeltaGroup < Arf.deltaGroups.size() ? whichDeltaGroup : 0;
	}
	lua_pop(L, 2);

	nodeMap.clear();
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
		Arf.wish.push_back({ .isSpecial = isSpecial, .deltaGroup = whichDeltaGroup }),
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
	lua_pushlightuserdata(L, &newWish);															// [2]
	if( lua_getfield(L, LUA_REGISTRYINDEX, "AR_WISH_METATABLE"), lua_isnil(L, -1) ) {		// [3]
		lua_pop(L, 1);							lua_newtable(L);
		lua_pushboolean(L, true);				lua_rawseti(L, 3, 0xADD8E6);
		lua_pushcfunction(L, scriptGetXY);		lua_setfield(L, 3, "__call");
	}
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
	nodeMap.clear();
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

	const auto pNewHelper = new (lua_newuserdata(L, sizeof(Wish))) Wish{.whole = 0};			// [2]
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
	if( lua_getfield(L, LUA_REGISTRYINDEX, "AR_HELPER_METATABLE"), lua_isnil(L, -1) ) {   // [3]
		lua_pop(L, 1);							lua_newtable(L);
		lua_pushboolean(L, false);			lua_rawseti(L, 3, 0xADD8E6);
		lua_pushcfunction(L, scriptGetXY);		lua_setfield(L, 3, "__call");
		lua_pushcfunction(L, helperGcMethod);	lua_setfield(L, 3, "__gc");
	}
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
			fromDeg = fromDeg > 32587 ? 32587 : fromDeg < -32587 ? -32587 : fromDeg;
			toDeg = fromDeg;
			break;
		case LUA_TTABLE: {
			lua_rawgeti(L, 2, 1);
			lua_rawgeti(L, 2, 2);   // [2] Angle  [3] fromDeg  [4] toDeg
			fromDeg = lua_isnumber(L, 3) ? lua_tointeger(L, 3) : 90;
			fromDeg = fromDeg > 32587 ? 32587 : fromDeg < -32587 ? -32587 : fromDeg;
			toDeg = lua_isnumber(L, 4) ? lua_tointeger(L, 4) : 90;
			toDeg = toDeg > 32587 ? 32587 : toDeg < -32587 ? -32587 : toDeg;
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

	const size_t beatCnt = lua_objlen(L, 1);
	pWish->wishChilds.reserve(beatCnt);
	Arf.hint.reserve(beatCnt);

	for( size_t i = 1 ; i <= beatCnt; ++i ) {
		const float beat = checkBeat(L, 1, i);
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
	// pWish->cIt = pWish->wishChilds.cbegin();   /* Will be done in OrganizeArf */
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

	const size_t beatCnt = lua_objlen(L, 1);
	Arf.hint.reserve(beatCnt);

	for( size_t i = 1; i <= beatCnt; ++i )
		if( const float beat = checkBeat(L, 1, i);  beat >= minT  &&  beat <= maxT )
			Arf.hint.push_back( {
				.pWish = reinterpret_cast<uint64_t>(pWish),
				.isSpecial = isSpecial,
				.relT = beat - minT,
			});
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

	float beatDelta = 0xFFFFFF;
	for( size_t i = Arf.verseWidx, ws = Arf.wish.size(); i < ws; ++i ) {
		const float firstBeat = Arf.wish[i].nodes[0].t;
		beatDelta = beatDelta > firstBeat ? firstBeat : beatDelta;
	}
	for( size_t i = Arf.verseEidx, es = Arf.echo.size(); i < es; ++i ) {
		const auto& echo = Arf.echo[i];
		const float fromBeat = echo.fromT, toBeat = echo.toT;
		beatDelta = fromBeat != -0xADD8E6  &&  beatDelta > fromBeat  ?  fromBeat : beatDelta;
		beatDelta = beatDelta > toBeat ? toBeat : beatDelta;
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
int Ar::OrganizeArf(lua_State* L) noexcept {
	/* Usage:
	 * local before_or_false, objcnt, wgo_required, hgo_required, ego_required = Arf4.OrganizeArf()
	 */
	if( Arf.deltaGroups.empty() ) {
		auto& defaultGroup = ( Arf.deltaGroups.push_back({}), Arf.deltaGroups.back() );
			  defaultGroup.nodes.push_back({ .base = 0, .init = 0, .value = 170.0f / 15000 });
			  defaultGroup.it = defaultGroup.nodes.cbegin();
	}

	/* Echo
	 * -- Convert fromT and toT into ms.
	 * -- Sort them by toT, then fromT -> ···, Discard all minT<0 or toT>(1>>20)-470 ones.
	 */
	constexpr auto TIME_INVALID_ECHO = [](const Echo& echo) {
		float minT = echo.toT - 510;
			  minT = minT > echo.fromT ? echo.fromT : minT;
		return minT < 0  ||  echo.toT > 1048105 /* (1<<20) - 470 */ ;
	};
	constexpr auto PRED_ECHO = [](const Echo& l, const Echo& r) {
		if( l.toT != r.toT )		return l.toT < r.toT;
		if( l.fromT != r.fromT )	return l.fromT < r.fromT;
		if( l.toX != r.toX )		return l.toX < r.toX;
		if( l.toY != r.toY )		return l.toY < r.toY;
		if( l.fromX != r.fromX )	return l.fromX < r.fromX;
		if( l.fromY != r.fromY )	return l.fromY < r.fromY;
		if( l.ease != r.ease )		return l.ease < r.ease;
		if( l.ci != r.ci )			return l.ci < r.ci;
		if( l.ce != r.ce )			return l.ce < r.ce;
		return l.status >= r.status;
	};

	for( auto& echo : Arf.echo )
		echo.toT = beatToMs( echo.toT ),
		echo.fromT = (echo.fromT == -0xADD8E6) ? (echo.toT - 510.0f) : beatToMs( echo.fromT );
	std::erase_if( Arf.echo, TIME_INVALID_ECHO );
	std::sort( Arf.echo.begin(), Arf.echo.end(), PRED_ECHO );


	/* Hint
	 * -- Re-construct them, use a map to sort & unrepeat them.
	 * -- HintCnt + EchoCnt < 32768.
	 * -- Max 31 Special Hints.
	 */
	std::map< float, std::map<int64_t, Hint> > validHints;
	for( const auto hintProto : Arf.hint ) {
		const auto pWish = (Wish*)hintProto.pWish;
		float time = pWish->nodes[0].t + hintProto.relT;
		const Duo pos = getXY(pWish, time);

		if( time = beatToMs(time), time >= 510 && time < 1048106 /* (1<<20) - 470 */ ) {
			auto& inner = validHints.contains(time) ? validHints[time] : validHints[time]={};
			inner[ pos.whole ] = {
				.x = pos.a, .y = pos.b, .ms = (uint32_t)time, .deltaMs = 0,
				.status = hintProto.isSpecial ? (uint8_t)SPECIAL_AUTO : (uint8_t)AUTO
			};
		}
	}

	Arf.hint.clear();
	for( const auto& outer : validHints )
		for( auto [_, hint] : outer.second )
			hint.status = Arf.spJudged == 31 ? hint.status : AUTO,
			Arf.spJudged += hint.status == SPECIAL_AUTO,
			Arf.hint.push_back(hint);
	Arf.spJudged = 0;


	/* Wish
	 * -- Nodes: Sorted by beat, Unrepeated, All args within range except t.
	 *			 Convert all Time into ms; remove all t<0 nodes but the latest one.
	 * -- Childs: All args within range except dt.
	 *			  Calculate all dts, use a map to sort & unrepeat all dt>=0 objs.
	 * -- Remove Wishes with less than 2 Nodes; Max 65535 Nodes & 65535 Childs.
	 * -- Max 131071 Wishes; Sort them & Fix Iterators.
	 */
	struct ChildArgs {
		Ar32(
			int16_t		fromDegree, toDegree;
			float		initRadius;
		)
	};
	std::map< double, std::map<uint64_t, WishChild> > childMap;

	for( auto& wish : Arf.wish ) {
		int16_t delCnt = -1;
		for( auto& node : wish.nodes )
			node.t =  beatToMs( node.t ),
			delCnt += node.t < 0 ? 1 : 0;
		if( delCnt > 0 ) {
			const auto initIt = wish.nodes.begin();
			wish.nodes.erase( initIt, initIt + delCnt );
		}

		while( !wish.nodes.empty()  &&  (wish.nodes.back().t > 1048575  ||  wish.nodes.size() > 65535) )
			wish.nodes.pop_back();
		if( wish.nodes.empty() )
			continue;

		childMap.clear();
		for( const auto child : wish.wishChilds ) {
			const double dt = beatToDt( child.dt, wish.deltaGroup );
				  auto&  inner = childMap.contains(dt) ? childMap[dt] : childMap[dt]={};
			const ChildArgs a = { child.fromDegree, child.toDegree, child.initRadius };
			inner[ a.whole ] = { dt, a.fromDegree, a.toDegree, a.initRadius };
		}
		wish.wishChilds.clear();

		for( const auto& outer : childMap )
			for( const auto [_, child] : outer.second )
				wish.wishChilds.push_back(child);
		if( wish.wishChilds.size() > 65535 )
			wish.wishChilds.resize(65535);
	}
	std::sort( Arf.wish.begin(), Arf.wish.end(), [](const Wish& l, const Wish& r) {
		return l.nodes.size() >= r.nodes.size();
	});
	while( !Arf.wish.empty()  &&  Arf.wish.back().nodes.empty() )
		Arf.wish.pop_back();

	constexpr auto PRED_WISH = [](const Wish& l, const Wish& r) {
		const auto lFirstNode = l.nodes[0], lLastNode = l.nodes.back(),
				   rFirstNode = r.nodes[0], rLastNode = r.nodes.back();
		if( lFirstNode.t != rFirstNode.t )		return lFirstNode.t < rFirstNode.t;
		if( lLastNode.t != rLastNode.t )		return lLastNode.t < rLastNode.t;
		return (lLastNode.t - lFirstNode.t) <= (rLastNode.t - lFirstNode.t);
	};
	std::sort( Arf.wish.begin(), Arf.wish.end(), PRED_WISH );
	if( Arf.wish.size() > 131071 )
		Arf.wish.resize(131071);

	/* Index, Metadata
	 * -- Max [Before] 1048575ms, Max [HintCnt+EchoCnt] 32575.
	 */
	for( auto& g : Arf.deltaGroups )   // Fix Iterators
		g.it = g.nodes.cbegin();
	for( auto& w : Arf.wish )
		w.cIt = w.wishChilds.cbegin(),
		w.pIt = w.nodes.cbegin();
	Arf.idxGroups.reserve(2047);

	const size_t wSize = Arf.wish.size();
	for( size_t wi = 0; wi < wSize; ++wi ) {
		const auto& w = Arf.wish[wi];
		const float fms = w.nodes[0].t, lms = w.nodes.back().t;

		const uint16_t lidx = (uint32_t)lms >> 9;
		if( lidx >= Arf.idxGroups.size() )
			Arf.idxGroups.resize( lidx+1 );
		if( lms > Arf.before )
			Arf.before = lms;

		for( uint16_t i = fms < 0 ? 0 : (uint32_t)fms >> 9; i<=lidx; ++i )
			Arf.idxGroups[i].wIdx.push_back(wi);
	}

	size_t objCount = Arf.hint.size();
	for( size_t hi = 0; hi < objCount; ++hi ) {
		const auto& h = Arf.hint[hi];
		const int32_t fms = (int32_t)h.ms - 510, lms = h.ms + 470;

		const uint16_t lidx = lms >> 9;
		if( lidx >= Arf.idxGroups.size() )
			Arf.idxGroups.resize( lidx+1 );
		if( lms > Arf.before )
			Arf.before = lms;

		for( uint16_t i = fms < 0 ? 0 : (uint32_t)fms >> 9; i<=lidx; ++i )
			Arf.idxGroups[i].hIdx.push_back(hi);
	}

	const size_t eSize = Arf.echo.size();
	for( size_t ei = 0; ei < eSize; ++ei ) {
		const auto& e = Arf.echo[ei];
		const float lms = e.toT + 470;

		const uint16_t lidx = (uint32_t)lms >> 9;
		if( lidx >= Arf.idxGroups.size() )
			Arf.idxGroups.resize( lidx+1 );
		if( lms > Arf.before )
			Arf.before = lms;

		float fms = e.toT - 510;
			  fms = fms > e.fromT ? e.fromT : fms;
		for( uint16_t i = fms < 0 ? 0 : (uint32_t)fms >> 9; i<=lidx; ++i )
			Arf.idxGroups[i].eIdx.push_back(ei);
		objCount += (e.status == SPECIAL_AUTO);
	}
	if( eSize > 32767  ||  objCount > 32767 )
		return lua_pushboolean(L, false), 1;
	Arf.objectCount = objCount;

	std::map<double, int16_t> stepDeltas;
	for( const auto& idx : Arf.idxGroups ) {
		const size_t hr = idx.hIdx.size(), er = idx.eIdx.size();
		if( hr > 511  ||  er > 1023 )		return lua_pushboolean(L, false), 1;
		if( hr > Arf.hgoRequired )			Arf.hgoRequired = hr;
		if( hr > Arf.egoRequired )			Arf.egoRequired = er;

		size_t groupWgoRequired = 0;
		for( const size_t wi : idx.wIdx ) {
			int32_t stepRequired = 1, maxStepRequired = 1;
			stepDeltas.clear();

			for( const auto& c : Arf.wish[wi].wishChilds )   /* Add Steps */
				++stepDeltas[( c.dt - c.initRadius )], --stepDeltas[c.dt];    // map[k] will be 0 by default
			for( const auto [_, stepDelta] : stepDeltas )    /* Perform Steps */
				stepRequired += stepDelta,
				maxStepRequired = stepRequired > maxStepRequired ? stepRequired : maxStepRequired;
			groupWgoRequired += (maxStepRequired < 2 ? 1 : maxStepRequired);
		}
		if( groupWgoRequired > 1023 )				return lua_pushboolean(L, false), 1;
		if( groupWgoRequired > Arf.wgoRequired )	Arf.wgoRequired = groupWgoRequired;
	}

	return lua_pushnumber(L, Arf.before),		lua_pushnumber(L, Arf.objectCount),
		   lua_pushnumber(L, Arf.wgoRequired),	lua_pushnumber(L, Arf.hgoRequired),
		   lua_pushnumber(L, Arf.egoRequired),	5;
}
#endif