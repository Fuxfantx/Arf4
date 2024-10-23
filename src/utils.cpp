//  Arf4 Utils  //
#include <cmath>
#include <Arf4.h>
using namespace Ar;
using namespace std;

/* Ease Constants */
double sin(double), cos(double);
constexpr struct EaseConstants {
	float ratioSin[1025] = {}, ratioCos[1025] = {}, degreeSin[901] = {}, degreeCos[901] = {};
	constexpr EaseConstants() {
		constexpr double PI = 3.141592653589793238462643383279502884197169399375105820974944592307816406286;
		for ( uint16_t i = 0; i < 1025; i++ ) {
			const double currentArc = PI * i / 2048.0;
			ratioSin[i] = sin(currentArc);
			ratioCos[i] = cos(currentArc);
		}
		for ( uint16_t i = 0; i < 901; i++ ) {
			const double currentArc = PI * i / 1800.0;
			degreeSin[i] = sin(currentArc);
			degreeCos[i] = cos(currentArc);
		}
	}
} C;

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
inline float calculateEasedRatio(float ratio, const uint8_t type) {
	switch(type) {
		case LINEAR:	return ratio;
		case INSINE:	return C.ratioSin[(uint16_t)(ratio * 1024)];
		case OUTSINE:	return C.ratioCos[(uint16_t)(ratio * 1024)];
		case INQUAD:	return ratio * ratio;
		case OUTQUAD:	return ratio = 1.0f - ratio, 1.0f - ratio * ratio;
		default:		return 0;
	}
}

template <typename T> void precalculate(T& object) {
	if( object.ease > 5 ) {
		if( object.ease > 10 ) {   // Clockwise when dx * dy < 0   -->   x: OUTSINE  y: INSINE
			object.xFci = C.ratioCos[(uint16_t)(object.ci * 1024)];
			object.xFce = C.ratioCos[(uint16_t)(object.ce * 1024)];
			object.xDnm = 1.0 / (object.xFce - object.xFci);
			object.yFci = C.ratioSin[(uint16_t)(object.ci * 1024)];
			object.yFce = C.ratioSin[(uint16_t)(object.ce * 1024)];
			object.yDnm = 1.0 / (object.yFce - object.yFci);
		}
		else {   // Clockwise when dx * dy > 0   -->   x: INSINE  y: OUTSINE
			object.xFci = C.ratioSin[(uint16_t)(object.ci * 1024)];
			object.xFce = C.ratioSin[(uint16_t)(object.ce * 1024)];
			object.xDnm = 1.0 / (object.xFce - object.xFci);
			object.yFci = C.ratioCos[(uint16_t)(object.ci * 1024)];
			object.yFce = C.ratioCos[(uint16_t)(object.ce * 1024)];
			object.yDnm = 1.0 / (object.yFce - object.yFci);
		}
	}
}

inline Duo getSinCosByDegree(const double degree) {   // Redundant "else" keywords are remained,
	if( degree >= 0 ) {								  //   to make the control flow easier to understand.
		const uint64_t deg = (uint64_t)(degree * 10.0) % 3600;
		if(deg > 1800) {
			if(deg > 2700)	return { .a = -C.degreeSin[3600-deg], .b =  C.degreeCos[3600-deg] };
			else			return { .a = -C.degreeSin[deg-1800], .b = -C.degreeCos[deg-1800] };
		}
		else {
			if(deg > 900)	return { .a =  C.degreeSin[1800-deg], .b = -C.degreeCos[1800-deg] };
			else			return { .a =  C.degreeSin[     deg], .b =  C.degreeCos[     deg] };
		}
	}
	else {   // sin(-x) = -sin(x), cos(-x) = cos(x)
		const uint64_t deg = (uint64_t)(-degree * 10.0) % 3600;
		if(deg > 1800) {
			if(deg > 2700)	return { .a =  C.degreeSin[3600-deg], .b =  C.degreeCos[3600-deg] };
			else			return { .a =  C.degreeSin[deg-1800], .b = -C.degreeCos[deg-1800] };
		}
		else {
			if(deg > 900)	return { .a = -C.degreeSin[1800-deg], .b = -C.degreeCos[1800-deg] };
			else			return { .a = -C.degreeSin[     deg], .b =  C.degreeCos[     deg] };
		}
	}
}

inline void PrecalculateEcho(Echo& echo) {
	return precalculate(echo);
}
inline void PrecalculatePosNode(PosNode& currentPn) {
	return precalculate(currentPn);
}

inline Duo InterpolateEcho(const Echo& echo, const uint32_t currentMs) {
	if( echo.ease == 0 )   return { .a = echo.fromX, .b = echo.fromY };
	const float deltaX = echo.toX - echo.fromX,
				deltaY = echo.toY - echo.fromY;

	if( echo.ease < 6 ) {   // Simple Path
		const float ratio = calculateEasedRatio( (double)(currentMs - echo.fromMs) / (echo.toMs - echo.fromMs)
											   , echo.ease );
		return {
			.a = echo.fromX + deltaX * ratio,
			.b = echo.fromY + deltaY * ratio
		};
	}
	if( echo.ease < 11 ) {   // Clockwise when dx * dy > 0   -->   x: INSINE  y: OUTSINE
		float ratio = calculateEasedRatio( (double)(currentMs - echo.fromMs) / (echo.toMs - echo.fromMs)
										 , echo.ease - 5 );
		if( echo.ci == 0  &&  echo.ce == 1 )
			return {
				.a = echo.fromX + deltaX * C.ratioSin[(uint16_t)(ratio * 1024)],
				.b = echo.fromY + deltaY * C.ratioCos[(uint16_t)(ratio * 1024)]
			};
		ratio = echo.ci + (echo.ce - echo.ci) * ratio;

		const float actualFromX = (echo.fromX * echo.xFce - echo.toX * echo.xFci) * echo.xDnm;
		const float actualFromY = (echo.fromY * echo.yFce - echo.toY * echo.yFci) * echo.yDnm;
		return {
			.a = actualFromX + deltaX * echo.xDnm * C.ratioSin[(uint16_t)(ratio * 1024)],
			.b = actualFromY + deltaY * echo.yDnm * C.ratioCos[(uint16_t)(ratio * 1024)]
		};
	}
	/* Default */ {   // Clockwise when dx * dy < 0   -->   x: OUTSINE  y: INSINE
		float ratio = calculateEasedRatio( (double)(currentMs - echo.fromMs) / (echo.toMs - echo.fromMs)
										 , echo.ease - 10 );
		if( echo.ci == 0  &&  echo.ce == 1 )
			return {
				.a = echo.fromX + deltaX * C.ratioCos[(uint16_t)(ratio * 1024)],
				.b = echo.fromY + deltaY * C.ratioSin[(uint16_t)(ratio * 1024)]
			};
		ratio = echo.ci + (echo.ce - echo.ci) * ratio;

		const float actualFromX = (echo.fromX * echo.xFce - echo.toX * echo.xFci) * echo.xDnm;
		const float actualFromY = (echo.fromY * echo.yFce - echo.toY * echo.yFci) * echo.yDnm;
		return {
			.a = actualFromX + deltaX * echo.xDnm * C.ratioCos[(uint16_t)(ratio * 1024)],
			.b = actualFromY + deltaY * echo.yDnm * C.ratioSin[(uint16_t)(ratio * 1024)]
		};
	}
}

inline Duo InterpolatePosNode(const PosNode& currentPn, const uint32_t currentMs, const PosNode& nextPn) {
	if( currentPn.ease == 0 )   return { .a = currentPn.x, .b = currentPn.y };
	const float deltaX = nextPn.x - currentPn.x,
				deltaY = nextPn.y - currentPn.y;

	if( currentPn.ease < 6 ) {   // Simple Path
		const float ratio = calculateEasedRatio( (double)(currentMs-currentPn.ms) / (nextPn.ms-currentPn.ms)
											   , currentPn.ease );
		return {
			.a = currentPn.x + deltaX * ratio,
			.b = currentPn.y + deltaY * ratio
		};
	}
	if( currentPn.ease < 11 ) {   // Clockwise when dx * dy > 0   -->   x: INSINE  y: OUTSINE
		float ratio = calculateEasedRatio( (double)(currentMs-currentPn.ms) / (nextPn.ms-currentPn.ms)
										 , currentPn.ease - 5 );
		if( currentPn.ci == 0  &&  currentPn.ce == 1 )
			return {
				.a = currentPn.x + deltaX * C.ratioSin[(uint16_t)(ratio * 1024)],
				.b = currentPn.y + deltaY * C.ratioCos[(uint16_t)(ratio * 1024)]
			};
		ratio = currentPn.ci + (currentPn.ce - currentPn.ci) * ratio;

		const float actualFromX = (currentPn.x * currentPn.xFce - nextPn.x * currentPn.xFci) * currentPn.xDnm;
		const float actualFromY = (currentPn.y * currentPn.yFce - nextPn.y * currentPn.yFci) * currentPn.yDnm;
		return {
			.a = actualFromX + deltaX * currentPn.xDnm * C.ratioSin[(uint16_t)(ratio * 1024)],
			.b = actualFromY + deltaY * currentPn.yDnm * C.ratioCos[(uint16_t)(ratio * 1024)]
		};
	}
	/* Default */ {   // Clockwise when dx * dy < 0   -->   x: OUTSINE  y: INSINE
		float ratio = calculateEasedRatio( (double)(currentMs-currentPn.ms) / (nextPn.ms-currentPn.ms)
										 , currentPn.ease - 10 );
		if( currentPn.ci == 0  &&  currentPn.ce == 1 )
			return {
				.a = currentPn.x + deltaX * C.ratioCos[(uint16_t)(ratio * 1024)],
				.b = currentPn.y + deltaY * C.ratioSin[(uint16_t)(ratio * 1024)]
			};
		ratio = currentPn.ci + (currentPn.ce - currentPn.ci) * ratio;

		const float actualFromX = (currentPn.x * currentPn.xFce - nextPn.x * currentPn.xFci) * currentPn.xDnm;
		const float actualFromY = (currentPn.y * currentPn.yFce - nextPn.y * currentPn.yFci) * currentPn.yDnm;
		return {
			.a = actualFromX + deltaX * currentPn.xDnm * C.ratioCos[(uint16_t)(ratio * 1024)],
			.b = actualFromY + deltaY * currentPn.yDnm * C.ratioSin[(uint16_t)(ratio * 1024)]
		};
	}
}

Arf4_API SimpleEaseLua(lua_State* L) {
	/* Usage:
	 * local result = Arf4.SimpleEase(from, type, to, ratio)
	 */
	const lua_Number from  = luaL_checknumber(L, 1);
	const lua_Number delta = luaL_checknumber(L, 3) - from;
		  lua_Number ratio = luaL_checknumber(L, 4);
	if		(ratio < 0)		ratio = 0;
	else if	(ratio > 1)		ratio = 1;
	return lua_pushnumber(L,
		from + delta * calculateEasedRatio( ratio, luaL_checkinteger(L, 2) )
	), 1;
}

Arf4_API PartialEaseLua(lua_State* L) {
	/* Usage:
	 * local result = Arf4.PartialEase(from, type, to, ratio, curve_init, curve_end)
	 */
	const uint32_t type = luaL_checkinteger(L, 2);
	const double from = luaL_checknumber(L, 1), to = luaL_checknumber(L, 3);

	// Calculate fci & fce
	lua_Number curve_init = luaL_checknumber(L, 5);
	lua_Number curve_end = luaL_checknumber(L, 6);
	if		(curve_init < 0)			curve_init = 0;
	else if (curve_init > 1)			curve_init = 1;
	if		(curve_end < 0)				curve_end = 0;
	else if (curve_end > 1)				curve_end = 1;
	if		(curve_init >= curve_end)	curve_init = 0,
										curve_end = 1;
	const float fci = calculateEasedRatio(curve_init, type);
	const float fce = calculateEasedRatio(curve_end, type);

	// Return the Ease Result
	lua_Number actualRatio = curve_init + (curve_end - curve_init) * luaL_checknumber(L, 4);
	if		(actualRatio < 0)			actualRatio = 0;
	else if (actualRatio > 1)			actualRatio = 1;

	const float dnmFceFci   = 1.0 / (fce - fci);
	const float actualFrom  = (to * fce - from * fci) * dnmFceFci;
	const float actualDelta = (to - from) * dnmFceFci;
	return lua_pushnumber(L, actualFrom + actualDelta * calculateEasedRatio(actualRatio, type) ), 1;
}

/* Fumen Utils */
/* Other Utils */