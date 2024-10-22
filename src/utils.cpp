// Arf4 Utils //
#include <cmath>
#include <Arf4.h>
using namespace Ar;
using namespace std;

/* Ease Constants */
double sin(double), cos(double);
constexpr struct EaseConstants {
	float ratioSin[1001] = {}, ratioCos[1001] = {}, degreeSin[901] = {}, degreeCos[901] = {};
	constexpr EaseConstants() {
		constexpr double PI = 3.141592653589793238462643383279502884197169399375105820974944592307816406286;
		for ( uint16_t i = 0; i < 1001; i++ ) {
			const double currentArc = PI * i / 2000.0;
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
 *     actualTo = from - (1-fci) * actualFrom
 * [2] to = actualFrom + (actualTo - actualFrom) * fce
 *     actuaoTo = to - (1-fce) * actualFrom
 *
 * -- [1]*fce - [2]*fci :
 *     actualFrom = (to * fce - from * fci) / (fce - fci)
 * -- [1]*(1-fce) - [2]*(1-fci) :
 *     actualTo = ( to * (1-fci) - from * (1-fce) ) / (fce - fci)
 * -- Let actualDelta = actualTo - actualFrom
 *     actualDelta = (to - from) * (1 - fci - fce) / (fce - fci)
 *
 * [Params] from, type, to, ratio, curve_init, curve_end
 * [Return] actualFrom + (actualTo - actualFrom) * f( curve_init + (curve_end-curve_init)*ratio )
 */
inline float calculateEasedRatio(float ratio, const uint8_t type) {
	switch(type) {
		case LINEAR:
			return ratio;
		case INSINE:
			return C.ratioSin[(uint16_t)(ratio * 10.0f)];
		case OUTSINE:
			return C.ratioCos[(uint16_t)(ratio * 10.0f)];
		case INQUAD:
			return ratio*ratio;
		case OUTQUAD:
			ratio = 1.0f - ratio;
			return  1.0f - ratio * ratio;
		default:
			return 0;
	}
}
template <typename T> void precalculate(T& object) {
	switch( object.ease % 5 ) {
		case 0:
			if( object.ease >> 1 ) {   // Exclude STATIC & LINEAR
				object.xFci = object.yFci = calculateEasedRatio(object.ci, object.ease);
				object.xFce = object.yFce = calculateEasedRatio(object.ce, object.ease);
				object.xDnm = object.yDnm = object.xFce - object.xFci;
			}
			break;
		case 1:   // Clockwise when dx * dy > 0   -->   x: INSINE  y: OUTSINE
			object.xFci = calculateEasedRatio(object.ci, INSINE);
			object.xFce = calculateEasedRatio(object.ce, INSINE);
			object.xDnm = 1.0 / (object.xFce - object.xFci);
			object.yFci = calculateEasedRatio(object.ci, OUTSINE);
			object.yFce = calculateEasedRatio(object.ce, OUTSINE);
			object.yDnm = 1.0 / (object.yFce - object.yFci);
			break;
		case 2:   // Clockwise when dx * dy < 0   -->   x: OUTSINE  y: INSINE
			object.xFci = calculateEasedRatio(object.ci, OUTSINE);
			object.xFce = calculateEasedRatio(object.ce, OUTSINE);
			object.xDnm = 1.0 / (object.xFce - object.xFci);
			object.yFci = calculateEasedRatio(object.ci, INSINE);
			object.yFce = calculateEasedRatio(object.ce, INSINE);
			object.yDnm = 1.0 / (object.yFce - object.yFci);
		default:;   // break omitted
	}
}

inline Duo GetSinCosByDegree(const double degree) {   // Redundant "else" keywords are remained,
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
inline void PrecalculatePosNode(PosNode& currentPn) {
	return precalculate(currentPn);
}
inline void PrecalculateEcho(Echo& echo) {
	return precalculate(echo);
}

int SimpleEaseLua(lua_State* L) {
	/* Usage:
	 * local result = Arf4.SimpleEase(from, type, to, ratio)
	 */
	const lua_Number from = luaL_checknumber(L, 1);
	const lua_Number delta = luaL_checknumber(L, 3) - from;

	lua_Number ratio = luaL_checknumber(L, 4);
	if( ratio < 0 )				ratio = 0;
	else if( ratio > 1 )		ratio = 1;

	const uint8_t type = luaL_checkinteger(L, 2);
	return lua_pushnumber(L, from + delta * calculateEasedRatio(ratio, type) ), 1;
}
int PartialEaseLua(lua_State* L) {
	/* Usage:
	 * local result = Arf4.PartialEase(from, type, to, ratio, curve_init, curve_end)
	 */
	const lua_Number from = luaL_checknumber(L, 1);
	const lua_Number to = luaL_checknumber(L, 3);

	lua_Number curve_init = luaL_checknumber(L, 5);
	if( curve_init < 0 )		curve_init = 0;
	else if( curve_init > 1 )	curve_init = 1;

	lua_Number curve_end = luaL_checknumber(L, 6);
	if( curve_end < 0 )			curve_end = 0;
	else if( curve_end > 1 )	curve_end = 1;

	lua_Number ratio = curve_init + (curve_end - curve_init) * luaL_checknumber(L, 4);
	if( ratio < 0 )				ratio = 0;
	else if( ratio > 1 )		ratio = 1;

	const uint8_t type = luaL_checkinteger(L, 2);
	const float fci = calculateEasedRatio(curve_init, type);
	const float fce = calculateEasedRatio(curve_end, type);

	/* Reuse */ curve_init  = 1.0 / (fce - fci);
	/* Reuse */ curve_end   = 1.0 - (fce + fci);
	const float actualFrom  = (to * fce - from * fci) * curve_init;
	const float actualDelta = (to - from) * curve_end * curve_init;
	return lua_pushnumber(L, actualFrom + actualDelta * calculateEasedRatio(ratio, type) ), 1;
}

/* Fumen Utils */
/* Other Utils */