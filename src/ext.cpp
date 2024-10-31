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
	{"TransformStr", Ar::TransformStr},
	{"PushNullPtr", Ar::PushNullPtr},
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

#ifdef DM_PLATFORM_ANDROID
JavaVM*		pVm;				jclass			hapticClass;
jobject		pActivity;			jmethodID		hapticMethod;
static void hapticInit() {
	JNIEnv* pEnv;
	pVm = dmGraphics::GetNativeAndroidJavaVM();
	pVm-> AttachCurrentThread(&pEnv, NULL);
	hapticClass = (jclass)pEnv->NewGlobalRef(
		pEnv->CallObjectMethod(
			/* Object */ pEnv->CallObjectMethod(
				pActivity = dmGraphics::GetNativeAndroidActivity(),
				pEnv->GetMethodID( pEnv->FindClass("android/app/NativeActivity"), "getClassLoader",
												   "()Ljava/lang/ClassLoader;" )
			),
			/* Method */ pEnv->GetMethodID( pEnv->FindClass("java/lang/ClassLoader"), "loadClass",
										   "(Ljava/lang/String;)Ljava/lang/Class;" ),
			/* Arg */	 pEnv->NewStringUTF("arf4.utils.Haptic")
		)
	);
	hapticMethod = pEnv->GetStaticMethodID(hapticClass, "DoHapticFeedback", "(Landroid/app/Activity;)V");
	pEnv -> ExceptionClear();
	pVm  -> DetachCurrentThread();
}
#endif

static dmExtension::Result Arf4Init(dmExtension::Params* p) {
	#ifdef DM_PLATFFORM_ANDROID
		hapticInit();
	#endif
	return luaL_register(p->m_L, "Arf4", Arf4Lib), lua_pop(p->m_L, 1), dmExtension::RESULT_OK;
}
static dmExtension::Result Arf4Final(dmExtension::Params* p) {
	#ifdef DM_PLATFFORM_ANDROID
		JNIEnv* pEnv;
		pVm  -> AttachCurrentThread(&pEnv, NULL);
		pEnv -> DeleteGlobalRef(hapticClass);
		pVm  -> DetachCurrentThread();
	#endif
	return dmExtension::RESULT_OK;
}
static dmExtension::Result Arf4APPOK(dmExtension::AppParams* p) { return dmExtension::RESULT_OK; }
DM_DECLARE_EXTENSION(AcArf4, "AcArf4", Arf4APPOK, Arf4APPOK, Arf4Init, nullptr, nullptr, Arf4Final)