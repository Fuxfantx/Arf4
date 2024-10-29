//  Arf4 Update  //
#include <Arf4.h>
using namespace Ar;

/* Tint Constants & Typedefs */
static constexpr auto H_EARLY_R = 0.275f, H_EARLY_G = 0.495f, H_EARLY_B = 0.5603125f;
static constexpr auto H_LATE_R = 0.5603125f, H_LATE_G = 0.3403125f, H_LATE_B = 0.275f;
static constexpr auto H_HIT_R = 0.73f, H_HIT_G = 0.6244921875f, H_HIT_B = 0.4591015625f;

static constexpr auto A_EARLY_R = 0.3125f, A_EARLY_G = 0.5625f, A_EARLY_B = 0.63671875f;
static constexpr auto A_LATE_R = 0.63671875f, A_LATE_G = 0.38671875f, A_LATE_B = 0.3125f;
static constexpr auto A_HIT_R = 1.0f, A_HIT_G = 0.85546875f, A_HIT_B = 0.62890625f;

typedef dmGameObject::HInstance GO;					using cfloat = const float;
typedef dmVMath::Vector3 v3i, *v3;					typedef dmVMath::Point3 p3;
typedef dmVMath::Vector4 v4i, *v4;					typedef dmVMath::Quat Qt;


/* Quat Utils & Render Methods */
static const Qt maxQuat(0.0f, 0.0f, 0.594822786751341f, 0.803856860617217f);
static Qt rotationToQuat(const double degree) {
		  Duo getSinCosByDegree(double);
	const Duo rads = getSinCosByDegree(degree);
	return Qt(0.0f, 0.0f, rads.a, rads.b);
}
static bool renderWish(lua_State* L, const Duo centerPos, Duo info, uint16_t wgoUsed) {
	// info.a -> zPos (0 or 0.02)   info.b -> tint
	Duo finalPos = {
		.a = 900.0f + (centerPos.a * Arf.rotCos - centerPos.b * Arf.rotSin) * Arf.xScale + Arf.xDelta
	};
	if( finalPos.a >= Arf.boundL  &&  finalPos.a <= Arf.boundR ) {
		finalPos.b = 540.0f + (centerPos.a * Arf.rotSin + centerPos.b * Arf.rotCos) * Arf.yScale + Arf.yDelta;
		if( finalPos.b >= Arf.boundD  &&  finalPos.b <= Arf.boundU ) {
			if(! Arf.lastWgo.count( finalPos.whole ) ) {
				/* Tint W */
				lua_pushnumber(L, info.b), lua_rawseti(L, WTINT, ++wgoUsed);
				info.b = 1.0f - info.b;

				/* Properties */
				const auto thisWgo = ( lua_rawgeti(L, WGO, wgoUsed),
												dmScript::CheckGOInstance(L, -1) );
				dmGameObject::SetPosition( thisWgo, p3(finalPos.a, finalPos.b, info.a) );
				dmGameObject::SetScale	 ( thisWgo, 0.637f + 0.437f * info.b * info.b );
				Arf.lastWgo[ finalPos.whole ] = wgoUsed;
				return lua_pop(L, 1), true;
			}

			/* Tint W */
			const uint16_t lastWgoLuaIdx = Arf.lastWgo[ finalPos.whole ];
						   lua_pushnumber(L, 1.0), lua_rawseti(L, WTINT, lastWgoLuaIdx);

			/* Properties */
			const auto lastWgo = ( lua_rawgeti(L, WGO, lastWgoLuaIdx),
											dmScript::CheckGOInstance(L, -1) );
			dmGameObject::SetPosition( lastWgo, dmGameObject::GetPosition(lastWgo)->setZ(info.a) );
			dmGameObject::SetScale	 ( lastWgo, 0.637f );
			lua_pop(L, 1);
		}
	}
	return false;
}
static bool renderAnim(lua_State* L, const Duo finalPos, const int16_t msPassed, const uint8_t status,
					   uint16_t agoUsed) {   // OutQuad: 1-(1-x)(1-x)   -->   1-(1-2x+x*x)   -->   x*(2-x)
	if( msPassed > 370 )		return false;
	const auto tint = ( lua_rawgeti(L, ATINT, ++agoUsed), dmScript::CheckVector4(L, -1) );
	const auto lAgo = ( lua_rawgeti(L, AGOL, agoUsed), dmScript::CheckGOInstance(L, -1) ),
			   rAgo = ( lua_rawgeti(L, AGOR, agoUsed), dmScript::CheckGOInstance(L, -1) );

	/* Tint XYZ */
	switch(status) {
		case HIT_LIT: default:
			switch(Arf.daymode) {
				case false: default:
					tint -> setX(A_HIT_R).setY(A_HIT_R).setZ(A_HIT_R);
					break;
				case true:   // break omitted
					tint -> setX(A_HIT_R).setY(A_HIT_G).setZ(A_HIT_B);
			}
			break;
		case EARLY_LIT:
			tint -> setX(A_EARLY_R).setY(A_EARLY_G).setZ(A_EARLY_B);
			break;
		case LATE_LIT:   // break omitted
			tint -> setX(A_LATE_R).setY(A_LATE_G).setZ(A_LATE_B);
	}

	/* Tint W */
	if( msPassed < 73 ) {
		float w = msPassed * 0.01f;
			  w = 0.637f * w * (2.0f - w);
		tint -> setW(w);
	}
	else {
		float w = (msPassed - 73) / 297.0f;
			  w = 0.637f * w * (2.0f - w);
		tint -> setW(0.637f - w);
	}

	/* Rotation & Scale */
	if( msPassed < 193 ) {
		const float leftRatio = msPassed / 193.0f;
		dmGameObject::SetRotation( lAgo, rotationToQuat(45.0f + 28.0f * leftRatio) );
		dmGameObject::SetScale( lAgo, 1.0f + 0.637f * leftRatio * (2.0f - leftRatio) );
	}
	else {
		dmGameObject::SetRotation(lAgo, maxQuat);
		dmGameObject::SetScale(lAgo, 1.637f);
	}
	const float rightRatio = msPassed / 370.0f;
	dmGameObject::SetRotation( rAgo, rotationToQuat(45.0f  -  8.0f * rightRatio) );
	dmGameObject::SetScale( rAgo, 1.0f + 0.637f * rightRatio * (2.0f - rightRatio) );

	return true;
}


/* Main */
Arf4_API UpdateArf(lua_State* L) {
	/* Usage:
	 * local wgo_used, hgo_used, ego_used, ago_used, h_playhs, e_playhs = Arf4.UpdateArf(
	 *     ms, dt, wgos, hgos, egos, ehgos, algos, argos, wtints, htints, etints, atints)
	 */
	Arf.msTime = (uint32_t)luaL_checknumber(L, 1); {
		if( Arf.msTime < 2 )						Arf.msTime = 2;
		else if( Arf.msTime >= Arf.before )			return 0;
	}
	const uint32_t frameEndMs = Arf.msTime + (uint32_t)(luaL_checknumber(L, 2) * 1000);
		  uint16_t wgoUsed = 0, hgoUsed = 0, egoUsed = 0, agoUsed = 0;
	struct{uint8_t hint:4 = false, echo:4 = false;} playHitSoundThisFrame;
	JudgeArfSweep();

	/* DeltaGroups */
	for( auto& deltaGroup : Arf.deltaGroups ) {
		while( deltaGroup.it != deltaGroup.nodes.cbegin()  &&  Arf.msTime < deltaGroup.it->initMs )
			--deltaGroup.it;
		while( deltaGroup.it != deltaGroup.nodes.cend() ) {
			if( Arf.msTime < (deltaGroup.it+1)->initMs ) {
				deltaGroup.dt = deltaGroup.it->baseDt + (Arf.msTime - deltaGroup.it->initMs) * deltaGroup.it->ratio;
				goto NEXT_GROUP;
			}
			++deltaGroup.it;
		}
		if( deltaGroup.it == Arf.deltaGroups.cend() )
			deltaGroup.dt = deltaGroup.it->baseDt + (Arf.msTime - deltaGroup.it->initMs) * deltaGroup.it->ratio;
		NEXT_GROUP:;
	}

	/* Wish */
	for( const auto wi : Arf.idxGroups[ Arf.msTime>>9 ].wIdx ) {
		auto& currentWish = Arf.wish[wi];
		Duo   nodePos;

		// Nodes //
		if( Arf.msTime <  currentWish.nodes[0].ms )			break;
		if( Arf.msTime >= currentWish.nodes.cend()->ms )	continue;
		while( currentWish.pIt != currentWish.nodes.cbegin()  &&  Arf.msTime < currentWish.pIt->ms )
			--currentWish.pIt;
		while( currentWish.pIt != currentWish.nodes.cend() ) {
			if( Arf.msTime < (currentWish.pIt+1)->ms ) {
				const float existMs = Arf.msTime - currentWish.nodes[0].ms;
				wgoUsed += renderWish( L,
					nodePos = InterpolatePosNode( *currentWish.pIt, *(currentWish.pIt+1) ),
					{ .a = 0, .b = existMs >= 151 ? 1 : existMs / 151.0f } , wgoUsed );
				break;
			}
			++currentWish.pIt;
		}

		// WishChilds //
		const double currentDt = Arf.deltaGroups[ currentWish.deltaGroup ].dt;
		if( currentWish.wishChilds.empty()						||
			currentWish.wishChilds.cend()->dt <= currentDt		||
			currentWish.wishChilds.cbegin()->dt > currentDt + 16 )	 continue;
		if( currentWish.cIt != currentWish.nodes.cbegin()  &&  (currentWish.cIt-1)->dt > currentDt ) {
			do	 --currentWish.cIt;
			while( currentWish.cIt != currentWish.nodes.cbegin()  &&  (currentWish.cIt-1)->dt > currentDt );
		}
		else while( currentWish.cIt != currentWish.nodes.cend()  &&  currentWish.cIt->dt <= currentDt )
			++currentWish.cIt;

		// Rough Range of Distance (0, 16)
		for( auto cItLocal = currentWish.cIt; cItLocal <= currentWish.nodes.cend(); ++cItLocal ) {
			const auto distance = (uint16_t)( (cItLocal->dt - currentDt) * 512.0 );
			if( distance > 8192 /* 16<<9 */ )				break;		// So aNodes[0].distance is the
			if( distance > cItLocal->aNodes[0].distance )	continue;   //   "Max Visible Distance".

			Duo childSinCos, getSinCosByDegree(double);
			while( cItLocal->aIt != cItLocal->aNodes.cbegin()  &&  distance > cItLocal->aIt->distance )
				--cItLocal->aIt;
			while( cItLocal->aIt != cItLocal->aNodes.cend() ) {
				const auto currentAnIt = cItLocal->aIt, nextAnIt = currentAnIt + 1;
				const uint16_t nextAnDist = nextAnIt->distance;
				if( distance > nextAnDist ) {
					float calculateEasedRatio(float, uint8_t);
					const uint16_t currentAnDist = currentAnIt->distance;
					const int16_t  currentAnDeg  = currentAnIt->degree;

					childSinCos = getSinCosByDegree(
						currentAnDeg + (nextAnIt->degree - currentAnDeg) * calculateEasedRatio(
							(float)(distance-currentAnDist) / (nextAnDist-currentAnDist), currentAnIt->ease
						)
					);
					goto RENDER_WISHCHILD;
				}
				++cItLocal->aIt;
			}
			if( cItLocal->aIt == cItLocal->aNodes.cend() )
				childSinCos = getSinCosByDegree( (double)cItLocal->aIt->degree );

			RENDER_WISHCHILD:
			const float existDistRatio = 1.0f - distance / cItLocal->aNodes[0].distance;
			wgoUsed += renderWish( L,
				{ .a = nodePos.a + distance * childSinCos.b, .b = nodePos.b + distance * childSinCos.a },
				{ .a = 0.02f, .b = existDistRatio >= 0.237f ? 1 : existDistRatio / 0.237f }, wgoUsed
			);
		}
	}

	/* Hint, Echo */
	for( const auto hi : Arf.idxGroups[ Arf.msTime>>9 ].hIdx ) {
		auto& currentHint = Arf.hint[hi];

	}
	for( const auto ei : Arf.idxGroups[ Arf.msTime>>9 ].eIdx ) {
		auto& currentEcho = Arf.echo[ei];

	}

	return lua_pushinteger(L, wgoUsed), lua_pushinteger(L, hgoUsed), lua_pushinteger(L, egoUsed),
		   lua_pushinteger(L, agoUsed), lua_pushboolean(L, playHitSoundThisFrame.hint),
		   lua_pushboolean(L, playHitSoundThisFrame.echo), 6;
}