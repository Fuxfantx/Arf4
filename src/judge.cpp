//  Arf4 Judge  //
#include <Arf4.h>
#ifndef AR_BUILD_VIEWER

using namespace Ar;
/* Judge Behaviors:
 * [1] Tap Behavior
 * 	   Earliest Path / Anmitsu Path / Pierce Path
 * [2] Echo Behavior
 *     Catch Path / Drag Path
 * [3] Lost Behavior
 */

inline bool hasTouchNear(const float x, const float y, const Duo* validTouches) noexcept {
	const float l = 900.0f + (x*Arf.rotCos - y*Arf.rotSin) * Arf.xScale + Arf.xDelta - Arf.objectSizeX*0.5f;
	const float d = 540.0f + (x*Arf.rotSin + y*Arf.rotCos) * Arf.yScale + Arf.yDelta - Arf.objectSizeY*0.5f;
	const float r = l + Arf.objectSizeX, u = d + Arf.objectSizeY;

	uint8_t whichTouch = 0;
	while(~ validTouches[whichTouch].whole ) {   // Using {.a=NaN, .b=NaN} as the ending identifier
		const float touchX = validTouches[whichTouch].a;
		if( touchX >= l  &&  touchX <= r ) {
			const float touchY = validTouches[whichTouch].b;
			if( touchY >= d  &&  touchY <= u )
				return true;
		}
		++whichTouch;
	}

	return false;
}

inline bool testAnmitsuSafety(const float x, const float y) noexcept {
	const float l = x - Arf.objectSizeX, d = y - Arf.objectSizeY;
	const float r = x + Arf.objectSizeX, u = y + Arf.objectSizeY;

	// Iterate blocked blocks
	for(const auto i : Arf.blocked)
		if( i.a > l && i.a < r )
			if( i.b > d && i.b < u )
				return false;

	// Register "Safe when Anmitsu" Objects
	Arf.blocked.push_back({ .a=x, .b=y });   // I hate the copying here.
	return true;
}

inline void scanHint(Hint& currentHint, const Duo* const validTouches) noexcept {
	switch( currentHint.status ) {
		case NJUDGED:
		case NJUDGED_LIT:
			currentHint.status = hasTouchNear(currentHint.x, currentHint.y, validTouches) ?
								 NJUDGED_LIT : NJUDGED ;
			break;
		case SPECIAL:
		case SPECIAL_LIT:
			currentHint.status = hasTouchNear(currentHint.x, currentHint.y, validTouches) ?
								 SPECIAL_LIT : SPECIAL ;
			break;
		case EARLY_LIT:
			if(! hasTouchNear(currentHint.x, currentHint.y, validTouches) )
				currentHint.status = EARLY;
			break;
		case HIT_LIT:
			if(! hasTouchNear(currentHint.x, currentHint.y, validTouches) )
				currentHint.status = HIT;
			break;
		case LATE_LIT:
			if(! hasTouchNear(currentHint.x, currentHint.y, validTouches) )
				currentHint.status = LATE;
			break;
		default:;
	}
}

inline void scanEcho(Echo& currentEcho, const int32_t deltaMs, const Duo* const validTouches) noexcept {
	switch( currentEcho.status ) {
		case NJUDGED:
			if( hasTouchNear(currentEcho.toX, currentEcho.toY, validTouches) )
				currentEcho.status = NJUDGED_LIT;
			break;
		case SPECIAL:
			if( hasTouchNear(currentEcho.toX, currentEcho.toY, validTouches) )
				currentEcho.status = SPECIAL_LIT;
			break;
		case HIT_LIT:
			currentEcho.status = hasTouchNear(currentEcho.toX, currentEcho.toY, validTouches) ?
								 HIT_LIT : HIT ;
			break;

		/* [2] Echo Behavior -- Drag Path */
		case NJUDGED_LIT:
			if(! hasTouchNear(currentEcho.toX, currentEcho.toY, validTouches) ) {
				if( deltaMs >= -100  &&  deltaMs <= 100 )
					currentEcho.status = HIT,			 currentEcho.deltaMs = deltaMs;
				else
					currentEcho.status = NJUDGED;
			}
			break;
		case SPECIAL_LIT:
			if(! hasTouchNear(currentEcho.toX, currentEcho.toY, validTouches) ) {
				if( deltaMs >= -100  &&  deltaMs <= 100 )
					currentEcho.status = HIT,			 currentEcho.deltaMs = deltaMs,
					Arf.eHit++;
				else
					currentEcho.status = SPECIAL;
			}
			break;
		default:;
	}
}

inline void judgeArfInternal(const Duo* const validTouches, const bool anyPressed, const bool anyReleased) noexcept {
	if(anyReleased)								// Only 1 Group will be iterated.
		Arf.blocked.clear();					// Pay attention when organizing an Arf4 Fumen.

	if(anyPressed) {							// Echo judging is previous than hint judging here
		uint32_t minJudgedMs = NULL;
		for( const auto i : Arf.idxGroups[Arf.msTime>>9].eIdx ) {						// Echo //
			auto& currentEcho = Arf.echo[i];
			const uint32_t currentEchoMs  = currentEcho.toT;
			const int32_t  currentDeltaMs = Arf.msTime - currentEchoMs;
			if( currentDeltaMs < -370 )		break;
			if( currentDeltaMs > +470 )		continue;

			scanEcho(currentEcho, currentDeltaMs, validTouches);
			if( currentEcho.status == NJUDGED_LIT  ||  currentEcho.status == SPECIAL_LIT ) {
				if( currentDeltaMs >= -100  &&  currentDeltaMs <= 100 ) {
					const bool safeToAnmitsu = testAnmitsuSafety(currentEcho.toX, currentEcho.toY);

					/* [1] Tap Behavior -- Earliest & Anmitsu Path */
					// No Pierce Path here; for Echoes, either Path of [2] should be applied instead.
					if( minJudgedMs ) {
						if( minJudgedMs != currentEchoMs )					 // Consider if maxDt < 0
							if( !safeToAnmitsu || currentDeltaMs < Arf.minDt || currentDeltaMs > Arf.maxDt )
								continue;
					}
					else
						minJudgedMs = currentEchoMs;

					if( currentEcho.status == SPECIAL_LIT )
						Arf.eHit++;

					currentEcho.deltaMs = currentDeltaMs;
					currentEcho.status = HIT_LIT;
				}
			}
		}
		for( const auto i : Arf.idxGroups[Arf.msTime>>9].hIdx ) {						// Hint //
			auto& currentHint = Arf.hint[i];
			const int32_t currentDeltaMs = Arf.msTime - currentHint.ms;
			if( currentDeltaMs < -370 )		break;
			if( currentDeltaMs > +470 )		continue;

			scanHint(currentHint, validTouches);
			if( currentHint.status == NJUDGED_LIT  ||  currentHint.status == SPECIAL_LIT ) {
				if( currentDeltaMs >= -100  &&  currentDeltaMs <= 100 ) {
					const bool safeToAnmitsu = testAnmitsuSafety(currentHint.x, currentHint.y);

					/* [1] Tap Behavior -- Earliest & Anmitsu Path */
					if( !minJudgedMs  ||  minJudgedMs >= currentHint.ms )
						minJudgedMs = currentHint.ms;
					else if( !safeToAnmitsu  ||  currentDeltaMs < Arf.minDt ) {
						if( currentDeltaMs <= Arf.maxDt )
							/* [1] Tap Behavior -- Pierce Path, Step I */
							/* |a| > |b|  -->  a^2 - b^2 > 0  -->  (a+b)*(a-b) > 0
							 *
							 *   (currentDeltaMs - deltaMs)
							 * = (msTime - hintMs) - (olderMsTime - hintMs)
							 * = (msTime - olderMsTime)  -->  Always larger than 0
							 *
							 * So we simply consider this:
							 *  · (currentDeltaMs + deltaMs) > 0  -->  |currentDeltaMs| > |deltaMs|
							 *  · (currentDeltaMs + deltaMs) < 0  -->  |currentDeltaMs| < |deltaMs|
							 */
							if( currentHint.deltaMs <-currentDeltaMs /* PENDING == -127, included */ )
								currentHint.deltaMs = currentDeltaMs;
						continue;
					}

					if( currentHint.status == SPECIAL_LIT )
						Arf.spJudged++;
					currentHint.deltaMs = currentDeltaMs;

					if( currentDeltaMs < Arf.minDt )
						++Arf.early, currentHint.status = EARLY_LIT;
					else if( currentDeltaMs <= Arf.maxDt )
						++Arf.hHit, currentHint.status = HIT_LIT;
					else
						++Arf.late, currentHint.status = LATE_LIT;
				}
			}
		}
	}
	else {
		for( const auto i : Arf.idxGroups[Arf.msTime>>9].eIdx ) {						// Echo //
			auto& currentEcho = Arf.echo[i];
			const int32_t currentDeltaMs = Arf.msTime - (uint32_t)currentEcho.toT;
			if( currentDeltaMs < -370 )		break;
			if( currentDeltaMs > +470 )		continue;
			scanEcho(currentEcho, currentDeltaMs, validTouches);
		}
		for( const auto i : Arf.idxGroups[Arf.msTime>>9].hIdx ) {						// Hint //
			auto& currentHint = Arf.hint[i];
			const int32_t currentDeltaMs = Arf.msTime - currentHint.ms;
			if( currentDeltaMs < -370 )		break;
			if( currentDeltaMs > +470 )		continue;
			scanHint(currentHint, validTouches);
		}
	}
}

void Ar::JudgeArfSweep() noexcept {
	for( const auto i : Arf.idxGroups[Arf.msTime>>9].eIdx ) {							// Echo //
		auto& currentEcho = Arf.echo[i];
		const int32_t currentDeltaMs = Arf.msTime - (int32_t)currentEcho.toT;
		if( currentDeltaMs > 255 ) {}
		else if( currentDeltaMs > 100 )											   /* [3] Lost Behavior */
			switch( currentEcho.status ) {
				case SPECIAL: case SPECIAL_LIT:		currentEcho.status = SPECIAL_LOST;	Arf.lost++;  break;
				case NJUDGED: case NJUDGED_LIT:		currentEcho.status = LOST;
				default:;   // break omitted
			}
		else if( currentDeltaMs >= 0 )								 /* [2] Echo Behavior -- Catch Path */
			switch( currentEcho.status ) {
				case SPECIAL_LIT:
					Arf.eHit++;   // No break here
				case NJUDGED_LIT:
					currentEcho.status = HIT_LIT, currentEcho.deltaMs = currentDeltaMs;
				default:;   // break omitted
			}
		else break;
	}
	for( const auto i : Arf.idxGroups[Arf.msTime>>9].hIdx ) {							// Hint //
		auto& currentHint = Arf.hint[i];
		const int32_t currentDeltaMs = Arf.msTime - currentHint.ms;
		if( currentDeltaMs > 255 ) {}
		else if( currentDeltaMs > 100 )											   /* [3] Lost Behavior */
			switch( currentHint.status ) {
				case SPECIAL: case SPECIAL_LIT:		currentHint.status = SPECIAL_LOST;	Arf.lost++;  break;
				case NJUDGED: case NJUDGED_LIT:		currentHint.status = LOST;			Arf.lost++;
				default:;   // break omitted
			}
		else if( currentDeltaMs >= Arf.maxDt ) {			/* [1] Tap Behavior -- Pierce Path, Step II */
			if( currentHint.deltaMs != PENDING )
				switch( currentHint.status ) {
					case SPECIAL:
						Arf.spJudged++;   // No break here
					case NJUDGED:
						currentHint.deltaMs < Arf.minDt ? ( ++Arf.early, currentHint.status = EARLY ) :
														  (  ++Arf.hHit, currentHint.status = HIT   ) ;
						break;
					case SPECIAL_LIT:
						Arf.spJudged++;   // No break here
					case NJUDGED_LIT:
						currentHint.deltaMs < Arf.minDt ? ( ++Arf.early, currentHint.status = EARLY_LIT ) :
														  (  ++Arf.hHit, currentHint.status = HIT_LIT   ) ;
					default:;   // break omitted
				}
		}
		else break;
	}
}

int Ar::JudgeArf(lua_State* L) {
	/* Usage:
	 * Arf4.JudgeArf(x_set, y_set, mask)
	 *
	 * Mask Design:
	 * [0b00] Invalid, [0b01] Pressed, [0b10] OnScreen, [0b11] Released
	 */
	lua_Integer mask = luaL_checkinteger(L, 3);
	uint8_t whichTouch = 0, anyPressed = false, anyReleased = false;
	Duo validTouches[27];   // lua_Number -> double float, 52bits for mantissas -> mask for 26 touches

	while(mask) {
		switch( mask & 0b11 ) {
			case 0b01:
				anyPressed = true;   // No break here
			case 0b10:
				validTouches[whichTouch].a = ( lua_rawgeti(L,1, whichTouch+1), lua_tonumber(L,-1) );
				validTouches[whichTouch].b = ( lua_rawgeti(L,2, whichTouch+1), lua_tonumber(L,-1) );
				lua_pop(L, 2);
				break;
			case 0b11:
				anyReleased = true;
			default:;   // break omitted
		}
		++whichTouch;
		mask >>= 2;
	}
	validTouches[whichTouch].whole = -1;

	return judgeArfInternal(validTouches, anyPressed, anyReleased), 0;
}
#endif