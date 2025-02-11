@file:Suppress("NOTHING_TO_INLINE")
import kotlinx.serialization.Serializable
import kotlin.math.PI

/* EaseType
 * Enum classes are expensive here, therefore we simply state some const values.
 */
const val STATIC = 0L;      const val LINEAR = 1L
const val INSINE = 2L;      const val OUTSINE = 3L

/* Object Status */
const val NJUDGED = 0L;     const val NJUDGED_LIT = 2L;     const val PENDING = -128L
const val SPECIAL = 1L;     const val SPECIAL_LIT = 3L
const val EARLY = 4L;       const val EARLY_LIT = 5L
const val HIT = 6L;         const val HIT_LIT = 7L
const val LATE = 8L;        const val LATE_LIT = 9L
const val AUTO = 10L;       const val SPECIAL_AUTO = 11L
const val LOST = 12L;       const val SPECIAL_LOST = 13L


/* Common
 * [20] ms
 * [7] x: [0,16) 1/8
 * [6] y: [0,8) 1/8
 */
const val TWO_PI_PER_32 = 2 * PI / 32
inline fun Long.ms() = this and 0xfffff
inline fun Long.x() = (this shr 20 and 0x7f) * 0.125
inline fun Long.y() = (this shr 27 and 0x3f) * 0.125


/* Point
 * [33] ms/x/y
 * [2] easeType
 * [7] radius: [0,16) 1/8
 * [14] degree: [-8192, 8191] 1/1
 */
const val RAD_PER_DEG = PI / 360
inline fun Long.pointEase() = this ushr 33 and 0x3
inline fun Long.pointRadius() = (this ushr 35 and 0x7f) * 0.125
inline fun Long.pointRad() = ( (this ushr 42) - 8192 ) * RAD_PER_DEG


/* Delta
 * [18] time [0,1048575] ms/4
 * [13] absScale [0,8) 1/1024
 * [32] base (float)
 */
const val DELTA_PRECISION = 1.0 / 1024
inline fun Long.deltaTime() = this and 0x7ffff shl 2
inline fun Long.deltaAbsScale() = (this ushr 18 and 0x1fff) * DELTA_PRECISION
inline fun Long.deltaBase() = Float.fromBits( (this ushr 31).toInt() )


/* Child
 * [32] dt
 * [5] radius: [0,8) 1/4
 * [5] initLoop: [0,1) 1/32
 * [7] deltaLoop: [-2,2) 1/32
 */
inline fun Long.childDt() = Float.fromBits( this.toInt() )   // Just lower 32 bits
inline fun Long.childRadius() = (this ushr 32 and 0x1f) * 0.25
inline fun Long.childFromRad() = (this ushr 37 and 0x1f) * TWO_PI_PER_32
inline fun Long.childDeltaRad() = ( (this ushr 42) - 64 ) * TWO_PI_PER_32


/* Wish
 * [6] pointCount
 * [17] pointSince
 * [1] isSpecial
 * [10] childCount
 * [18] childSince
 * [12] deltaGroupId
 */
inline fun Long.pointCount() = this and 0x3f
inline fun Long.pointSince() = this ushr 6 and 0x1ffff
inline fun Long.isSpecial() = (this ushr 23 and 1) == 1L
inline fun Long.childCount() = this ushr 24 and 0x3ff
inline fun Long.childSince() = this ushr 34 and 0x7fff
inline fun Long.deltaGroupId() = this ushr 52   // [0,16) Recommended


/* Object
 * [33] ms/x/y
 * [4] status
 * [8] deltaMs: [-128, 128)
 * [Echo 5] radius: [0,8) 1/4
 * [Echo 5] initLoop: [0,1) 1/32
 * [Echo 7] deltaLoop: [-2,2) 1/32
 */
const val STATUS_CLR_MASK: Long = (0xf shl 33).inv()
const val DELTAMS_CLR_MASK: Long = (0xff shl 37).inv()
inline fun Long.withStatus(s: Long) = this and STATUS_CLR_MASK or (s shl 33)
inline fun Long.withDeltaMs(d: Long) = this and DELTAMS_CLR_MASK or (d+128 shl 37)

inline fun Long.status() = this ushr 33 and 0xf
inline fun Long.deltaMs() = (this ushr 37 and 0xff) - 128
inline fun Long.echoRadius() = (this ushr 45 and 0x1f) * 0.25
inline fun Long.echoFromRad() = (this ushr 50 and 0x1f) * TWO_PI_PER_32
inline fun Long.echoDeltaRad() = ( (this ushr 55) - 64 ) * TWO_PI_PER_32


/* DeltaGroup / Idx
 * [10] count
 * [22] since
 */
inline fun Int.count() = this and 0x7ff
inline fun Int.since() = this ushr 10   // [0,2^18) Recommended


@Serializable
data class Fumen (
	val before: Int,
	val objectCount: Int,

	val points: LongArray,
	val wishChilds: LongArray,
	val deltaGroups: Array<LongArray>,

	val wishes: LongArray,      val wishIdx: IntArray,   // "Equivalent" Wish(es) Are Allowed
	var echoes: LongArray,      val echoIdx: IntArray,
	var hints: LongArray,       val hintIdx: IntArray
)