//  Arf4 Utils  //
#include <cmath>
#include <Arf4.h>
using namespace std;
using namespace Ar;

/* Ease Funcs */
/* Derivations related to the Partial Ease:
 *
 * [1] from = actualFrom + (actualTo - actualFrom) * fci
 *     actualTo * fci = from - actualFrom * (1-fci)
 * [2] to = actualFrom + (actualTo - actualFrom) * fce
 *     actualTo * fce = to - actualFrom * (1-fce)
 *
 * [1]*fce - [2]*fci:
 *     actualFrom = (from * fce - to * fci) / (fce - fci)
 * [1]*(1-fce) - [2]*(1-fci):
 *     actualTo = ( to * (1-fci) - from * (1-fce) ) / (fce - fci)
 * Let actualDelta = actualTo - actualFrom:
 *     actualDelta = (to - from) / (fce - fci)
 *
 * [Params] from, type, to, ratio, curve_init, curve_end
 * [Return] actualFrom + actualDelta * f( curve_init + (curve_end-curve_init)*ratio )
 */
#include <constants.h>
inline float calculateEasedRatio(FloatInDetail r, const uint8_t type) noexcept {
	switch(type) {
		default:
		case STATIC:	return .0f;
		case LINEAR:	return r.f;
		case INSINE:	return inSine [( r.e+=10, (uint16_t)r.f )];
		case OUTSINE:	return outSine[( r.e+=10, (uint16_t)r.f )];
		case INQUAD:	return r.f * r.f;
		case OUTQUAD:	return r.f * (2.0f - r.f);
	}
}

Duo Ar::GetSinCosByDegree(FloatInDetail d) noexcept {
	switch( d.s ) {
		default:
		case 0: {
			uint64_t deg16  = (d.e+=4, d.f);		const uint64_t deg16div1440 = deg16 / 1440;
					 deg16 -= deg16div1440 * 1440;
			switch( deg16div1440 & 0b11 ) {
				default:
				case 0: return { .a =  degreeSin[     deg16], .b =  degreeSin[1440-deg16] };   // 0~90
				case 1: return { .a =  degreeSin[1440-deg16], .b = -degreeSin[     deg16] };   // 90~180
				case 2: return { .a = -degreeSin[     deg16], .b = -degreeSin[1440-deg16] };   // 180~270
				case 3: return { .a = -degreeSin[1440-deg16], .b =  degreeSin[     deg16] };   // 270~360
			}
		}
		case 1: {   // d.f < 0, sin(-x) = -sin(x), cos(-x) = cos(x)
			uint64_t deg16  = (d.e+=4, -d.f);		const uint64_t deg16div1440 = deg16 / 1440;
					 deg16 -= deg16div1440 * 1440;
			switch( deg16div1440 & 0b11 ) {
				default:
				case 0: return { .a = -degreeSin[     deg16], .b =  degreeSin[1440-deg16] };
				case 1: return { .a = -degreeSin[1440-deg16], .b = -degreeSin[     deg16] };
				case 2: return { .a =  degreeSin[     deg16], .b = -degreeSin[1440-deg16] };
				case 3: return { .a =  degreeSin[1440-deg16], .b =  degreeSin[     deg16] };
			}
		}
	}
}

Duo Ar::InterpolatePoint(const Point thisPn, const Point nextPn) noexcept {
	if( thisPn.ease == 0 )   return { .a = thisPn.x, .b = thisPn.y };
	const float deltaX = nextPn.x - thisPn.x,
				deltaY = nextPn.y - thisPn.y;

	if( thisPn.ease < 6 ) {   // Simple Path
		const float ratio = calculateEasedRatio( { .f =
							(Arf.msTime - thisPn.t) / (nextPn.t - thisPn.t) }, thisPn.ease );
		return {
			.a = thisPn.x + deltaX * ratio,
			.b = thisPn.y + deltaY * ratio
		};
	}
	if( thisPn.ease < 11 ) {   // Clockwise when dx * dy > 0   -->   x: INSINE  y: OUTSINE
		FloatInDetail r = { .f = calculateEasedRatio( { .f =
					  (Arf.msTime - thisPn.t) / (nextPn.t - thisPn.t) }, thisPn.ease - 5 ) };
		if( !thisPn.ci  &&  thisPn.ce == 0x3FF ) {
			const uint16_t Rx1024 = ( r.e+=10, r.f );
			return { .a = thisPn.x + deltaX * inSine [Rx1024] ,
						.b = thisPn.y + deltaY * outSine[Rx1024] };
		}
		const uint16_t Ax1024 = thisPn.ci + (thisPn.ce - thisPn.ci) * r.f;   // Actual x 1024

		Duo cice, result;
		result.a = 1.0f / (( cice.b=inSine [thisPn.ce] ) - ( cice.a=inSine [thisPn.ci] ));
		result.a = ( thisPn.x * cice.b - nextPn.x * cice.a + deltaX * inSine [Ax1024] ) * result.a;
		result.b = 1.0f / (( cice.b=outSine[thisPn.ce] ) - ( cice.a=outSine[thisPn.ci] ));
		result.b = ( thisPn.y * cice.b - nextPn.y * cice.a + deltaY * outSine[Ax1024] ) * result.b;
		return result;
	}
	/* Default */ {   // Clockwise when dx * dy < 0   -->   x: OUTSINE  y: INSINE
		FloatInDetail r = { .f = calculateEasedRatio( { .f =
					  (Arf.msTime - thisPn.t) / (nextPn.t - thisPn.t) }, thisPn.ease - 10 ) };
		if( !thisPn.ci  &&  thisPn.ce == 0x3FF ) {
			const uint16_t Rx1024 = ( r.e+=10, r.f );
			return { .a = thisPn.x + deltaX * outSine[Rx1024] ,
						.b = thisPn.y + deltaY * inSine [Rx1024] };
		}
		const uint16_t Ax1024 = thisPn.ci + (thisPn.ce - thisPn.ci) * r.f;

		Duo cice, result;
		result.a = 1.0f / (( cice.b=outSine[thisPn.ce] ) - ( cice.a=outSine[thisPn.ci] ));
		result.a = ( thisPn.x * cice.b - nextPn.x * cice.a + deltaX * outSine[Ax1024] ) * result.a;
		result.b = 1.0f / (( cice.b=inSine [thisPn.ce] ) - ( cice.a=inSine [thisPn.ci] ));
		result.b = ( thisPn.y * cice.b - nextPn.y * cice.a + deltaY * inSine [Ax1024] ) * result.b;
		return result;
	}
}

Duo Ar::InterpolateEcho(const Echo& echo) noexcept {
	if( echo.toT  <= Arf.msTime )	return { .a = echo.toX, .b = echo.toY };
	if( echo.ease == 0 )			return { .a = echo.fromX, .b = echo.fromY };
	const float deltaX = echo.toX - echo.fromX,
				deltaY = echo.toY - echo.fromY;

	if( echo.ease < 6 ) {   // Simple Path
		const float ratio = calculateEasedRatio( { .f =
							(Arf.msTime - echo.fromT) / (echo.toT - echo.fromT) }, echo.ease );
		return {
			.a = echo.fromX + deltaX * ratio,
			.b = echo.fromY + deltaY * ratio
		};
	}
	if( echo.ease < 11 ) {   // Clockwise when dx * dy > 0   -->   x: INSINE  y: OUTSINE
		FloatInDetail r = { .f = calculateEasedRatio( { .f =
					  (Arf.msTime - echo.fromT) / (echo.toT - echo.fromT) }, echo.ease - 5 ) };
		if( !echo.ci  &&  echo.ce == 0xFF ) {
			const uint16_t Rx1024 = ( r.e+=10, r.f );
			return { .a = echo.fromX + deltaX * inSine [Rx1024] ,
						.b = echo.fromY + deltaY * outSine[Rx1024] };
		}
		const uint16_t CI = echo.ci<<2, CE = echo.ce<<2;
		const uint16_t Ax1024 = CI + (CE-CI) * r.f;   // Actual x 1024

		Duo cice, result;
		result.a = 1.0f / (( cice.b=inSine [CE] ) - ( cice.a=inSine [CI] ));
		result.a = ( echo.fromX * cice.b - echo.toX * cice.a + deltaX * inSine [Ax1024] ) * result.a;
		result.b = 1.0f / (( cice.b=outSine[CE] ) - ( cice.a=outSine[CI] ));
		result.b = ( echo.fromY * cice.b - echo.toY * cice.a + deltaY * outSine[Ax1024] ) * result.b;
		return result;
	}
	/* Default */ {   // Clockwise when dx * dy < 0   -->   x: OUTSINE  y: INSINE
		FloatInDetail r = { .f = calculateEasedRatio( { .f =
					  (Arf.msTime - echo.fromT) / (echo.toT - echo.fromT) }, echo.ease - 10 ) };
		if( !echo.ci  &&  echo.ce == 0xFF ) {
			const uint16_t Rx1024 = ( r.e+=10, r.f );
			return { .a = echo.fromX + deltaX * outSine[Rx1024] ,
						.b = echo.fromY + deltaY * inSine [Rx1024] };
		}
		const uint16_t CI = echo.ci<<2, CE = echo.ce<<2;
		const uint16_t Ax1024 = CI + (CE-CI) * r.f;

		Duo cice, result;
		result.a = 1.0f / (( cice.b=outSine[CE] ) - ( cice.a=outSine[CI] ));
		result.a = ( echo.fromX * cice.b - echo.toX * cice.a + deltaX * outSine[Ax1024] ) * result.a;
		result.b = 1.0f / (( cice.b=inSine [CE] ) - ( cice.a=inSine [CI] ));
		result.b = ( echo.fromY * cice.b - echo.toY * cice.a + deltaY * inSine [Ax1024] ) * result.b;
		return result;
	}
}


/* Fumen Utils */
int Ar::SetCam(lua_State *L) {
	/* Usage:
	 * Arf4.SetCam(xscale, yscale, xdelta, ydelta, rotdeg)
	 */
	const Duo sinCos = GetSinCosByDegree({ .f = (float)luaL_checknumber(L, 5) });
	Arf.xScale = luaL_checknumber(L, 1);		Arf.yScale = luaL_checknumber(L, 2);
	Arf.xDelta = luaL_checknumber(L, 3);		Arf.yDelta = luaL_checknumber(L, 4);
	return Arf.rotSin = sinCos.a, Arf.rotCos = sinCos.b, 0;
}

#ifndef AR_BUILD_VIEWER
int Ar::SetBound(lua_State* L) {
	/* Usage:
	 * Arf4.SetBound(l, r, u, d)
	 */
	Arf.boundL = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : -36 ;
	Arf.boundR = lua_isnumber(L, 2) ? lua_tonumber(L, 2) : 1836 ;
	Arf.boundU = lua_isnumber(L, 3) ? lua_tonumber(L, 3) : 1116 ;
	Arf.boundD = lua_isnumber(L, 4) ? lua_tonumber(L, 4) : -36 ;
	if( Arf.boundL > Arf.boundR )		Arf.boundL = -36, Arf.boundR = 1836;
	if( Arf.boundD > Arf.boundU )		Arf.boundD = -36, Arf.boundU = 1116;
	return 0;
}

int Ar::SetDaymode(lua_State* L) {
	/* Usage:
	 * Arf4.SetDaymode(is_daymode)
	 */
	return Arf.daymode = lua_toboolean(L, 1), 0;
}

int Ar::SetInputDelta(lua_State* L) {
	/* Usage:
	 * Arf4.SetInputDelta(ms)   -- [-63,63]
	 */
	const int8_t inputDeltaParam = luaL_checkinteger(L, 1);
	InputDelta = inputDeltaParam > 63 ? 63 : inputDeltaParam < -63 ? -63 : inputDeltaParam;

	Arf.minDt = InputDelta - Arf.judgeRange;		Arf.minDt = Arf.minDt < -100 ? -100 : Arf.minDt ;
	Arf.maxDt = InputDelta + Arf.judgeRange;		Arf.maxDt = Arf.maxDt >  100 ?  100 : Arf.maxDt ;
	return 0;
}

int Ar::SetJudgeRange(lua_State* L) {
	/* Usage:
	 * Arf4.SetJudgeRange(ms)   -- [1,100]
	 */
	const uint8_t rangeParam = luaL_checkinteger(L, 1);
	Arf.judgeRange = rangeParam > 99 ? 100 : rangeParam < 1 ? 1 : rangeParam;

	Arf.minDt = InputDelta - Arf.judgeRange;		Arf.minDt = Arf.minDt < -100 ? -100 : Arf.minDt ;
	Arf.maxDt = InputDelta + Arf.judgeRange;		Arf.maxDt = Arf.maxDt >  100 ?  100 : Arf.maxDt ;
	return 0;
}

int Ar::SetObjectSize(lua_State* L) {
	/* Usage:
	 * Arf4.SetObjectSize(size)   -- min 2.88
	 * Arf4.SetObjectSize(x, y)
	 */
	lua_Number scriptX = luaL_checknumber(L, 1);
	lua_Number scriptY = lua_isnumber(L, 2) ? lua_tonumber(L, 2) : scriptX;
	if( scriptX < 2.88 )		scriptX = 2.88;
	if( scriptY < 2.88 )		scriptY = 2.88;
	return Arf.objectSizeX = scriptX * 112.5, Arf.objectSizeY = scriptY * 112.5, 0;
}

int Ar::GetJudgeStat(lua_State* L) {
	/* Usage:
	 * local hint_hit, echo_hit, early, late, lost, count_of_judged_special_object, object_count
	 *     = Arf4.GetJudgeStat()
	 */
	return lua_pushinteger(L, Arf.hHit),  lua_pushinteger(L, Arf.eHit),  lua_pushinteger(L, Arf.early),
		   lua_pushinteger(L, Arf.late),  lua_pushinteger(L, Arf.lost),  lua_pushinteger(L, Arf.spJudged),
		   lua_pushinteger(L, Arf.objectCount), 7;
}


/* Other Utils */
int Ar::Ease(lua_State* L) {
	/* Usage:
	 * local result = Arf4.SimpleEase(from, type, to, ratio)
	 */
	const lua_Number from  = luaL_checknumber(L, 1),
					 delta = luaL_checknumber(L, 3) - from;
	FloatInDetail r = { .f = (float)luaL_checknumber(L, 4) };
	if		(r.f < 0)		  r.f = 0;
	else if (r.f > 1)		  r.f = 1;
	return lua_pushnumber(L,
		from + delta * calculateEasedRatio( r, luaL_checkinteger(L, 2) )
	), 1;
}

int Ar::PartialEase(lua_State* L) {
	/* Usage:
	 * local result = Arf4.PartialEase(from, type, to, ratio, curve_init, curve_end)
	 */
	const uint32_t type = luaL_checkinteger(L, 2);
	const double   from = luaL_checknumber(L, 1), to = luaL_checknumber(L, 3);

	FloatInDetail fci = { .f = (float)luaL_checknumber(L, 5) };
	FloatInDetail fce = { .f = (float)luaL_checknumber(L, 6) };
	if		(fci.f < 0)				   fci.f = 0;
	else if (fci.f > 1)				   fci.f = 1;
	if		(fce.f < 0)				   fce.f = 0;
	else if (fce.f > 1)				   fce.f = 1;
	if		(fci.f >= fce.f)		   fci.f = 0,
									   fce.f = 1;
	FloatInDetail actualRatio = { .f = fci.f + (fce.f - fci.f) * (float)luaL_checknumber(L, 4) };
	if		(actualRatio.f < 0)		   actualRatio.f = 0;
	else if (actualRatio.f > 1)		   actualRatio.f = 1;

	fci.f = calculateEasedRatio(fci, type);
	fce.f = calculateEasedRatio(fce, type);
	return lua_pushnumber(L,
		( to*fce.f - from*fci.f + (to-from) * calculateEasedRatio(actualRatio, type) ) / ( fce.f-fci.f )
	), 1;
}

int Ar::DoHapticFeedback(lua_State* L) {
	/* Usage:
	 * Arf4.DoHapticFeedback()
	 */
#if defined(DM_PLATFORM_IOS) || defined(DM_PLATFORM_ANDROID)
	void doHapticFeedback();
		 doHapticFeedback();
#endif
	return 0;
}

int Ar::PushNullPtr(lua_State* L) {
	/* Usage:
	 * local nullptr_userdata = Arf4.PushNullPtr()
	 */
	return lua_pushlightuserdata(L, nullptr), 1;
}
#endif

int Ar::NewTable(lua_State* L) {
	/* Usage:
	 * local preallocated_table = Arf4.NewTable(narr, nrec)
	 */
	return lua_createtable( L, luaL_checkinteger(L,1), luaL_checkinteger(L,2) ), 1;
}


/* Android Specific Stuff */
#ifdef DM_PLATFORM_ANDROID
	extern JavaVM* pVm;				extern jclass	 hapticClass;
	extern jobject pActivity;		extern jmethodID hapticMethod;

	void doHapticFeedback() {
		JNIEnv* pEnv;
		pVm  -> AttachCurrentThread(&pEnv, NULL);
		pEnv -> CallStaticVoidMethod(hapticClass, hapticMethod, pActivity);		pEnv -> ExceptionClear();
		pVm  -> DetachCurrentThread();
	}
#endif