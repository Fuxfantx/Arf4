// /* Nemesis, the Aerials Fumen Compiler. */
// #include <map>
// #include <Arf4.h>
// 
// struct BPMNode {
// 	lua_Number initBar, bpm, baseMs;
// };
// std::vector<BPMNode> BpmList = { {0, 170, 0} };
// std::vector<BPMNode>::const_iterator BpmListIt;
// 
// int nemesis_load_bpmlist(lua_State* L) {
// 	/* Usage:
// 	 * Time {
// 	 *     offset = 0,
// 	 *     0, 170,
// 	 *     2, 144
// 	 * }
// 	 */
// 	std::map<lua_Number, lua_Number> inputBpmList;
// 
// 	const size_t inputLen = lua_objlen(L, 1);
// 	for( size_t i=1; i<inputLen; i+=2 ) {
// 		lua_Number currentBar = ( lua_rawgeti(L, 1, i), luaL_checknumber(L, -1) );
// 				   currentBar = currentBar > 0 ? currentBar : 0;
// 		lua_Number currentBpm = ( lua_rawgeti(L, 1, i+1), luaL_checknumber(L, -1) );
// 				   currentBpm = currentBpm > 0 ? currentBpm : 0;
// 		inputBpmList[currentBar] = currentBpm;
// 		lua_pop(L, 2);
// 	}
// 
// 	BpmList.clear();
// 	BpmList.resize( inputBpmList.size() );
// 	for( const auto pair : inputBpmList )
// 		BpmList.push_back({
// 			.initBar = pair.first,
// 			.bpm = pair.second
// 		});
// 
// 	if( BpmList.empty() )
// 		BpmList.push_back( {0, 170, 0} );
// 	else {
// 		BpmList[0] = {
// 			.initBar = 0,
// 			.bpm = BpmList[0].bpm,
// 			.baseMs = ( lua_getfield(L, 1, "offset"), luaL_checknumber(L, -1) )
// 		};
// 
// 		const size_t bpmListSize = BpmList.size();
// 		for( size_t i=1; i<bpmListSize; i++ )
// 			BpmList[i].baseMs = BpmList[i-1].baseMs + 240000 / BpmList[i-1].bpm * (BpmList[i].initBar - BpmList[i-1].initBar);
// 	}
// 	BpmListIt = BpmList.cbegin();
// 
// 	return 0;
// }
// 
// int nemesis_query_bpm(lua_State* L) {
// 	const lua_Number bar = luaL_checknumber(L, 1);
// 	const auto lastBpmNode = BpmList.cend() - 1;
// 
// 	if( bar >= lastBpmNode->bar )
// 		return lua_pushnumber( L, lastBpmNode->baseMs + (bar - lastBpmNode->bar) * 240000 / lastBpmNode->bpm ), 1;
// 
// 	while( BpmListIt != BpmList.cbegin()  &&  bar < BpmListIt->bar )
// 		--BpmListIt;
// 	while( BpmListIt != lastBpmNode  &&  bar >= (BpmListIt+1)->bar )
// 		++BpmListIt;
// 	return lua_pushnumber( L, BpmListIt->baseMs + (bar - BpmListIt->bar) * 240000 / BpmListIt->bpm ), 1;
// }
// 
// 
// #ifdef AR_BUILD_VIEWER
// int NewBuild(lua_State* L);
// int NewDeltaGroup(lua_State* L);
// int NewVerse(lua_State* L);
// int MirrorLR(lua_State* L);
// int MirrorUD(lua_State* L);
// int NewHelper(lua_State* L);
// int NewChild(lua_State* L);
// int NewWish(lua_State* L);
// int NewHint(lua_State* L);
// int NewEcho(lua_State* L);
// int Move(lua_State* L);
// int OrganizeArf(lua_State* L);
// #endif