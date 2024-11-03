//  Arf4 Inout  //
#include <Arf4.h>
#include <bitsery/bitsery.h>
#include <dmsdk/dlib/hashtable.h>
#include <bitsery/traits/adapter_buffer.h>
#include <bitsery/traits/container_vector.h>
#include <bitsery/traits/compact_value.h>
#include <bitsery/traits/value_range.h>

struct PseudoContext {
	dmConfigFile::HConfig       m_ConfigFile;
	dmResource::HFactory        m_ResourceFactory;
};


/* Bitsery Settings */
#define Inout(TYPE, DETAILS)	template<typename S> void serialize(S& s, Arf4:: TYPE &its) {			   \
			  s.enableBitPacking( [&its](typename S::BPEnabledType& inout) { DETAILS ; } ); }
namespace bitsery {
	static constexpr auto ES = ext::ValueRange<float>  {0.0f, 1.0f, 1.0f/1024};				// [10] Curve
	static constexpr auto PY = ext::ValueRange<float>  {-8100.0f, 8100.0f, 1.0f/4};			// [16] Pos Y
	static constexpr auto PX = ext::ValueRange<float>  {-16200.0f, 16200.0f, 1.0f/4};			// [17] Pos X
	static constexpr auto DR = ext::ValueRange<float>  {-16.0f, 16.0f, 1.0f/131072};			// [21] Dt Ratio
	static constexpr auto DT = ext::ValueRange<double> {0.0, (double)0x111111, 1.0/131072};	// [41] Dt Base
	static constexpr auto CV = ext::CompactValueAsObject{};

	Inout( PosNode,
		inout.ext(its.ms, CV);		inout.ext(its.ease, CV);		inout.ext(its.x, PX);
		inout.ext(its.ci, ES);		inout.ext(its.ce, ES);			inout.ext(its.y, PY);
	)
	Inout( Hint,
		inout.ext(its.ms, CV);		inout.ext(its.status, CV);   // hint.status -> [0]Non-Special [1]Special
		inout.ext(its.x, PX);		inout.ext(its.y, PY);
	)
	Inout( Echo,
		inout.ext(its.fromMs, CV);	inout.ext(its.toMs, CV);		inout.ext(its.ci, ES);
		inout.ext(its.fromX, PX);	inout.ext(its.toX, PX);			inout.ext(its.ce, ES);
		inout.ext(its.fromY, PY);	inout.ext(its.toY, PY);			inout.ext(its.isReal, CV);
		inout.ext(its.ease, CV);	inout.ext(its.status, CV);   // echo.status -> [0]Non-Special [1]Special
	)

	Inout( AngleNode,
		   inout.ext(its.ease, CV);		inout.ext(its.degree, CV);	inout.ext(its.distance, CV);	)
	Inout( WishChild,
		   inout.ext(its.dt, DT);		inout.container(its.aNodes, 65535);							)
	Inout( DeltaNode,
		   inout.ext(its.ratio, DR);	inout.ext(its.initMs, CV);									)
	Inout( DeltaGroup,
		   inout.container(its.nodes, 65535);														)
	Inout( Idx,
		   inout.container2b(its.wIdx, 65535);
		   inout.container2b(its.hIdx, 65535);
		   inout.container2b(its.eIdx, 65535);
	)
	Inout( Wish,
		   inout.container(its.wishChilds, 65535);
		   inout.container(its.nodes, 65535);
		   inout.ext(its.deltaGroup, CV);
	)

	Inout( Fumen,
		   inout.container(its.idxGroups, 2047);
		   inout.container(its.deltaGroups, 65535);		inout.container(its.hint, 65535);
		   inout.container(its.wish, 131071);			inout.container(its.echo, 65535);
		   inout.ext(its.wgoRequired, CV);				inout.ext(its.hgoRequired, CV);
		   inout.ext(its.objectCount, CV);				inout.ext(its.egoRequired, CV);
		   inout.ext(its.before, CV);
	)

	struct Arf4Config {
		static constexpr EndiannessType		Endianness = DefaultConfig::Endianness;
		static constexpr bool				CheckAdapterErrors = false, CheckDataErrors = false;
	};
	using GetArf4Encoder = Serializer< OutputBufferAdapter< std::vector<uint8_t>, Arf4Config > >;
	using GetArf4Decoder = Deserializer< InputBufferAdapter<const uint8_t*, Arf4Config> >;
}


/* Inout APIs */
Arf4_API LoadArf(lua_State* L) {
	/* Usage:
	 * local before_or_false, objcnt, wgo_required, hgo_required, ego_required = Arf4.LoadArf(path, is_auto)
	 */
	lua_pushnumber(L, 2744634527);									// Path -> hash"__script_context"
	lua_gettable(L, LUA_GLOBALSINDEX);								// Path -> context
	const auto pContext = (PseudoContext*)lua_touserdata(L, 3);
	const auto isAuto = lua_toboolean(L, 2);
	const auto path = luaL_checkstring(L, 1);
	lua_pop(L, 3);

	// Acquire Buffer
	uint8_t* pBuf;														// free() this.
	uint32_t bufSize;

	const auto loadResult = dmResource::GetRaw( pContext->m_ResourceFactory, path, (void**)&pBuf, &bufSize );
	if( loadResult != dmResource::RESULT_OK ) {
		FILE* pFile = fopen(path, "rb");					// Open
		if( pFile == nullptr ) {
			lua_pushboolean(L, false);
			return 1;
		}

		fseek(pFile, 0, SEEK_END);							// Size
		bufSize = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		pBuf = (uint8_t*)malloc(bufSize);								// Copying
		if( fread( pBuf, 1, bufSize, pFile ) != bufSize ) {
			lua_pushboolean(L, false);
			pBuf = ( fclose(pFile), free(pBuf), nullptr );
			return 1;
		}
		fclose(pFile);
	}

	// Deserialize
	auto decodeState = bitsery::GetArf4Decoder(pBuf, bufSize);
	const bool readError = decodeState.adapter().error() != bitsery::ReaderError::NoError;
	const bool desError = !decodeState.adapter().isCompletedSuccessfully();
	if( readError || desError ) {
		lua_pushboolean(L, false);
		return free(pBuf), 1;
	}

	// Initialize
	decodeState.object( Arf={} );								// Lazy clear only when the buffer
	Arf.maxDt = ( InputDelta>63 ? 63 : InputDelta ) + 37;				//   is loaded successfully.
	Arf.minDt = Arf.maxDt - 74;

	for( auto& deltaGroup : Arf.deltaGroups ) {
		for( size_t i=1, l=deltaGroup.nodes.size(); i<l; ++i ) {
			const auto& lastNode = deltaGroup.nodes[i-1];
				  auto& thisNode = deltaGroup.nodes[i];
			thisNode.baseDt = lastNode.baseDt + (thisNode.initMs - lastNode.initMs) * lastNode.ratio;
		}
		deltaGroup.it = deltaGroup.nodes.cbegin();
	}

	for( auto& wish: Arf.wish ) {
		wish.pIt = wish.nodes.cbegin();
		for( size_t i=0, l=wish.nodes.size()-1; i<l; ++i )
			PrecalculatePosNode( wish.nodes[i] );

		wish.cIt = wish.wishChilds.begin();
		for( auto& c : wish.wishChilds )
			c.aIt = c.aNodes.cbegin();
	}

	if(isAuto) {
		for( auto& hint : Arf.hint )
			hint.status = hint.status ? SPECIAL_AUTO : AUTO,		hint.deltaMs = 0;
		for( auto& echo : Arf.echo )
			echo.status = echo.status ? SPECIAL_AUTO : AUTO,		echo.deltaMs = 0,
			PrecalculateEcho(echo);
	}
	else {
		for( auto& hint : Arf.hint )
			hint.status = hint.status ? SPECIAL : NJUDGED;
		for( auto& echo : Arf.echo )
			echo.status = echo.status ? SPECIAL : NJUDGED,
			PrecalculateEcho(echo);
	}

	return lua_pushnumber(L, Arf.before),			lua_pushnumber(L, Arf.objectCount),
		   lua_pushnumber(L, Arf.wgoRequired),		lua_pushnumber(L, Arf.hgoRequired),
		   lua_pushnumber(L, Arf.egoRequired),		free(pBuf), 5;
}

#ifdef AR_BUILD_VIEWER
Arf4_API ExportArf(lua_State* L) {
	/* Usage:
	 * local str_or_false = Arf4.ExportArf()
	 */
	for( auto& hint : Arf.hint )	hint.status = hint.status >= SPECIAL;
	for( auto& echo : Arf.echo )	echo.status = echo.status >= SPECIAL;

	std::vector<uint8_t> buf;
	auto encodeState = bitsery::GetArf4Encoder(buf);
		 encodeState.object(Arf);
		 encodeState.adapter().flush();

	const size_t bufSize = encodeState.adapter().writtenBytesCount();
				 bufSize ? lua_pushlstring( L, (char*)&buf[0], bufSize ) : lua_pushboolean(L, false);
	return 1;
}
#else
#include <dmsdk/dlib/crypt.h>
Arf4_API TransformStr(lua_State* L) {
	/* Usage:
	 * local output_str = Arf4.TransformStr(input_str, proof_str, is_decode)
	 */
	size_t inputSize, proofSize;
	const char *inputStr = luaL_checklstring(L, 1, &inputSize),
			   *proofStr = luaL_checklstring(L, 2, &proofSize);

	uint8_t proof16[16], proofMd5[16], proofSha1[20];
	if( proofSize > 15 )
		for( size_t i=0; i<16; ++i )
			proof16[i] = proofStr[i];
	else {
		for( size_t i=0; i<proofSize; ++i )
			proof16[i] = proofStr[i];
		for( size_t i=proofSize; i<16; ++i )
			proof16[i] = i*3 + 73;
	}
	dmCrypt::HashMd5 ( (const uint8_t*)proofStr, (uint32_t)proofSize, proofMd5  );
	dmCrypt::HashSha1( (const uint8_t*)proofStr, (uint32_t)proofSize, proofSha1 );

	// Decode //
	if( lua_toboolean(L, 3) ) {
		uint32_t originalSize;
		uint8_t* outputStr = (uint8_t*)malloc( originalSize = inputSize );

		dmCrypt::Base64Decode( (const uint8_t*)inputStr, inputSize, outputStr, &originalSize );
		Decrypt(dmCrypt::ALGORITHM_XTEA, outputStr, originalSize, proofSha1, 16);
		Decrypt(dmCrypt::ALGORITHM_XTEA, outputStr, originalSize, proofMd5, 16);
		Decrypt(dmCrypt::ALGORITHM_XTEA, outputStr, originalSize, proof16, 16);

		return lua_pushlstring( L, (const char*)outputStr, originalSize ), free(outputStr), 1;
	}

	// Encode //
	uint32_t outputSize = inputSize * 2;
	const auto inputStrMutable = (uint8_t*)const_cast<char*>(inputStr),
					 outputStr = (uint8_t*)malloc(outputSize);

	Encrypt(dmCrypt::ALGORITHM_XTEA, inputStrMutable, inputSize, proof16, 16);
	Encrypt(dmCrypt::ALGORITHM_XTEA, inputStrMutable, inputSize, proofMd5, 16);
	Encrypt(dmCrypt::ALGORITHM_XTEA, inputStrMutable, inputSize, proofSha1, 16);
	dmCrypt::Base64Encode(inputStrMutable, inputSize, outputStr, &outputSize);

	return lua_pushstring( L, (const char*)outputStr ), free(outputStr), 1;
}
#endif