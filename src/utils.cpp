// Arf4 Utils //
#include <cmath>
#include <Arf4.h>
using namespace Ar;
using namespace std;

/* Precalculate Ease Constants */
double sin(double), cos(double);
struct EaseConstants {
	double ratioSin[1001] = {}, ratioCos[1001] = {}, degreeSin[901] = {}, degreeCos[901] = {};
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