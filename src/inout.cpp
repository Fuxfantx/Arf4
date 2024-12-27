//  Arf4 Inout  //
#include <Arf4.h>
#include <bitsery/bitsery.h>
#include <bitsery/traits/adapter_buffer.h>
#include <bitsery/traits/container_vector.h>
#include <bitsery/traits/compact_value.h>
#include <bitsery/traits/value_range.h>
#include <dmsdk/dlib/crypt.h>

struct PseudoContext {
	dmConfigFile::HConfig       m_ConfigFile;
	dmResource::HFactory        m_ResourceFactory;
};


/* Bitsery Settings */
#define Inout(TYPE, DETAILS)	template<typename S> void serialize(S& s, Arf4:: TYPE &its) {			   \
			  s.enableBitPacking( [&its](typename S::BPEnabledType& inout) { DETAILS ; } ); }
namespace bitsery {
	static constexpr auto PY = ext::ValueRange{-8100.0f, 8100.0f, 1.0f/4};			// [16] Pos Y
	static constexpr auto PX = ext::ValueRange{-16200.0f, 16200.0f, 1.0f/4};		// [17] Pos X
	static constexpr auto MS = ext::ValueRange{-1048575.0f, 1048575.0f, 1.0f};		// [21] Time(ms)
	static constexpr auto DR = ext::ValueRange{-16.0f, 16.0f, 1.0f/131072};		// [21] Dt Ratio
	static constexpr auto DT = ext::ValueRange{0.0, (double)0xFFFFFF, 1.0/131072};	// [41] Dt Base
	static constexpr auto CV = ext::CompactValueAsObject{};

	Inout( Point,
		   inout.ext(its.x, PX);			inout.ext(its.y, PY);				inout.ext(its.t, MS);
		   inout.ext(its.whole, CV);
	)
	Inout( Echo,
		   inout.ext(its.toX, PX);			inout.ext(its.toY, PY);				inout.ext(its.toT, MS);
		   inout.ext(its.fromX, PX);		inout.ext(its.fromY, PY);			inout.ext(its.fromT, MS);
		   inout.ext(its.whole, CV);
	)
	Inout( Hint,
		   inout.ext(its.x, PX);			inout.ext(its.y, PY);				inout.ext(its.whole, CV);
	)

	Inout( WishChild,
		   inout.ext(its.dt, DT);			inout.ext(its.initRadius, DR);
		   inout.ext(its.toDegree, CV);		inout.ext(its.fromDegree, CV);								)
	Inout( DeltaNode,
		   inout.ext(its.init, MS);			inout.ext(its.value, DR);									)
	Inout( DeltaGroup,
		   inout.container(its.nodes, 65536);															)

	Inout( Idx,
		   inout.container4b(its.wIdx, 65536);
		   inout.container2b(its.hIdx, 65536);
		   inout.container2b(its.eIdx, 65536);
	)
	Inout( Wish,
		   inout.container(its.nodes, 65536);			inout.container(its.wishChilds, 65536);
		   inout.ext(its.whole, CV);
	)
	Inout( Fumen,
		   inout.container(its.idxGroups, 2048);		inout.container(its.deltaGroups, 65536);
		   inout.container(its.wish, 131072);			inout.container(its.hint, 32768);
		   inout.container(its.echo, 32768);			inout.ext(its.whole, CV);
	)

	struct Arf4Config {
		static constexpr EndiannessType		Endianness = DefaultConfig::Endianness;
		static constexpr bool				CheckAdapterErrors = false, CheckDataErrors = false;
	};
	using GetArf4Encoder = Serializer< OutputBufferAdapter< std::vector<uint8_t>, Arf4Config > >;
	using GetArf4Decoder = Deserializer< InputBufferAdapter<const uint8_t*, Arf4Config> >;
}


/* Inout APIs */
int Ar::LoadArf(lua_State* L) {
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
		if( pFile == nullptr )
			return lua_pushboolean(L, false), 1;

		fseek(pFile, 0, SEEK_END);							// Size
		bufSize = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);

		pBuf = (uint8_t*)malloc(bufSize);								// Copying
		if( fread( pBuf, 1, bufSize, pFile ) != bufSize ) {
			free(pBuf), fclose(pFile);
			return lua_pushboolean(L, false), 1;
		}
		fclose(pFile);
	}

	// Deserialize
	auto decodeState = bitsery::GetArf4Decoder(pBuf, bufSize);
	const bool readError =  decodeState.adapter().error() != bitsery::ReaderError::NoError,
			   desError  = !decodeState.adapter().isCompletedSuccessfully();
	if( readError || desError ) {
		lua_pushboolean(L, false);
		return free(pBuf), 1;
	}

	// Initialize
	Arf = {};
	decodeState.object(Arf);								// Lazy clear only when the buffer
	Arf.maxDt = ( InputDelta>63 ? 63 : InputDelta ) + 37;	//   is loaded successfully.
	Arf.minDt = Arf.maxDt - 74;

	for( auto& deltaGroup : Arf.deltaGroups ) {
		for( size_t i=1, l=deltaGroup.nodes.size(); i<l; ++i ) {
			const auto& lastNode = deltaGroup.nodes[i-1];
				  auto& thisNode = deltaGroup.nodes[i];
			thisNode.base = lastNode.base + (thisNode.init - lastNode.init) * lastNode.value;
		}
		deltaGroup.it = deltaGroup.nodes.cbegin();
	}
	for( auto& wish: Arf.wish )
		wish.pIt = wish.nodes.cbegin(),				wish.cIt = wish.wishChilds.cbegin();

	if(isAuto) {
		for( auto& hint : Arf.hint )
			hint.status = hint.status ? SPECIAL_AUTO : AUTO,	hint.deltaMs = 0;
		for( auto& echo : Arf.echo )
			echo.status = echo.status ? SPECIAL_AUTO : AUTO;
	}
	else {
		for( auto& hint : Arf.hint )
			hint.status = hint.status ? SPECIAL : NJUDGED,		hint.deltaMs = PENDING;
		for( auto& echo : Arf.echo )
			echo.status = echo.status ? SPECIAL : NJUDGED;
	}

	return lua_pushnumber(L, Arf.before),			lua_pushnumber(L, Arf.objectCount),
		   lua_pushnumber(L, Arf.wgoRequired),		lua_pushnumber(L, Arf.hgoRequired),
		   lua_pushnumber(L, Arf.egoRequired),		free(pBuf), 5;
}

#ifdef AR_BUILD_VIEWER
int Ar::ExportArf(lua_State* L) {
	/* Usage:
	 * local str_or_nil = Arf4.ExportArf()
	 */
	for( auto& hint : Arf.hint )	hint.status = hint.status >= SPECIAL,	hint.deltaMs = 0;
	for( auto& echo : Arf.echo )	echo.status = echo.status >= SPECIAL,	echo.deltaMs = 0;

	std::vector<uint8_t> buf;
	auto encodeState = bitsery::GetArf4Encoder(buf);
		 encodeState.object(Arf);

	const size_t bufSize = ( encodeState.adapter().flush(), encodeState.adapter().writtenBytesCount() );
				 bufSize ? lua_pushlstring( L, (char*)&buf[0], bufSize ) : lua_pushnil(L);
	return 1;
}
#else
int Ar::TransformStr(lua_State* L) {
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
	uint32_t outputSize = inputSize * 4 / 3 + 2;
	const auto inputStrMutable = (uint8_t*)const_cast<char*>(inputStr),
					 outputStr = (uint8_t*)malloc(outputSize);

	Encrypt(dmCrypt::ALGORITHM_XTEA, inputStrMutable, inputSize, proof16, 16);
	Encrypt(dmCrypt::ALGORITHM_XTEA, inputStrMutable, inputSize, proofMd5, 16);
	Encrypt(dmCrypt::ALGORITHM_XTEA, inputStrMutable, inputSize, proofSha1, 16);
	dmCrypt::Base64Encode(inputStrMutable, inputSize, outputStr, &outputSize);

	return lua_pushstring( L, (const char*)outputStr ), free(outputStr), 1;
}
#endif