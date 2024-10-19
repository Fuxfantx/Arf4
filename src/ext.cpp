#include <Arf4.h>
		  Arf4::Fumen	Arf;
		  int8_t		InputDelta;

static const luaL_reg Arf4Lib[] = {
	{"LoadArf", Ar::LoadArf},
	{"NewTable", Ar::NewTable},
	{"UpdateArf", Ar::UpdateArf},
#ifdef AR_BUILD_VIEWER
	{"ExportArf", Ar::ExportArf},
	{"OrganizeArf", Ar::OrganizeArf},
#else
	{"JudgeArf", Ar::JudgeArf},
	{"SimpleEase", Ar::SimpleEaseLua},
	{"PartialEase", Ar::PartialEaseLua},
	{"DoHapticFeedback", Ar::DoHapticFeedback},
	{"SetBound", Ar::SetBound},
	{"SetDaymode", Ar::SetDaymode},
	{"SetInputDelta", Ar::SetInputDelta},
	{"SetJudgeRange", Ar::SetJudgeRange},
	{"SetObjectSize", Ar::SetObjectSize},
	{"GetJudgeStat", Ar::GetJudgeStat},
#endif
	{"SetCam", Ar::SetCam},
	{nullptr, nullptr}
};

inline dmExtension::Result Arf4LuaInit(dmExtension::Params* p) {
	/* Defold Restriction:
	 * Must Get the Lua Stack Balanced in the Initiation Process. */
	luaL_register(p->m_L, "Arf4", Arf4Lib);		lua_pop(p->m_L, 1);
	return dmExtension::RESULT_OK;
}
inline dmExtension::Result Arf4APPOK(dmExtension::AppParams* params) {
	return dmExtension::RESULT_OK;
}
inline dmExtension::Result Arf4OK(dmExtension::Params* params) {
	return dmExtension::RESULT_OK;
}
DM_DECLARE_EXTENSION(AcArf4, "AcArf4", Arf4APPOK, Arf4APPOK, Arf4LuaInit, nullptr, nullptr, Arf4OK)