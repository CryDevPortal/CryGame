#ifndef __DEBUG_HELPER_H__
#define __DEBUG_HELPER_H__


#define DEBUG_DRAW_INIT(py) CDebugHelper::Py() = py;
#define DEBUG_DRAW_NL(py) CDebugHelper::Py() += py;

#define DEBUG_DRAW_INT(num)                                                                \
	CDebugHelper::DrawTextLabel(10,  CDebugHelper::Py(), CDebugHelper::ColorWhite(), #num);      \
	CDebugHelper::DrawTextLabel(200, CDebugHelper::Py(), CDebugHelper::ColorWhite(), "%10i", (num)); \
	CDebugHelper::Py() += 15;

#define DEBUG_DRAW_FLOAT(flt)                                                                \
	CDebugHelper::DrawTextLabel(10,  CDebugHelper::Py(), CDebugHelper::ColorWhite(), #flt);      \
	CDebugHelper::DrawTextLabel(200, CDebugHelper::Py(), CDebugHelper::ColorWhite(), "%10.3f", (flt)); \
	CDebugHelper::Py() += 15;

#define DEBUG_DRAW_VEC3(vec)                                                                   \
	CDebugHelper::DrawTextLabel(10,  CDebugHelper::Py(), CDebugHelper::ColorWhite(), #vec);        \
	CDebugHelper::DrawTextLabel(200, CDebugHelper::Py(), CDebugHelper::ColorWhite(), "%10.3f", (vec).x); \
	CDebugHelper::DrawTextLabel(300, CDebugHelper::Py(), CDebugHelper::ColorWhite(), "%10.3f", (vec).y); \
	CDebugHelper::DrawTextLabel(400, CDebugHelper::Py(), CDebugHelper::ColorWhite(), "%10.3f", (vec).z); \
	CDebugHelper::Py() += 15;

#define DEBUG_DRAW_VEC4(vec)                                                                   \
	CDebugHelper::DrawTextLabel(10,  CDebugHelper::Py(), CDebugHelper::ColorWhite(), #vec);        \
	CDebugHelper::DrawTextLabel(200, CDebugHelper::Py(), CDebugHelper::ColorWhite(), "%10.3f", (vec).x); \
	CDebugHelper::DrawTextLabel(300, CDebugHelper::Py(), CDebugHelper::ColorWhite(), "%10.3f", (vec).y); \
	CDebugHelper::DrawTextLabel(400, CDebugHelper::Py(), CDebugHelper::ColorWhite(), "%10.3f", (vec).z); \
	CDebugHelper::DrawTextLabel(500, CDebugHelper::Py(), CDebugHelper::ColorWhite(), "%10.3f", (vec).w); \
	CDebugHelper::Py() += 15;

#define DEBUG_DRAW_ANG3(ang)   DEBUG_DRAW_VEC3(ang)

#define DEBUG_INIT_CVARS CDebugHelper::InitCVars();
#define DEBUG_REGISTER_TMP_CVAR(var, type) CDebugHelperAutoRegCVar<type> g__TmpCVar##var(#var, &var);
#define DEBUG_REGISTER_TMP_CVAR_INT(var) DEBUG_REGISTER_TMP_CVAR(var, int)
#define DEBUG_REGISTER_TMP_CVAR_FLOAT(var) DEBUG_REGISTER_TMP_CVAR(var, float)
#define DEBUG_REGISTER_TMP_CVAR_STRING(var) DEBUG_REGISTER_TMP_CVAR(var, const char*)

#define DEBUG_PROFILE_TIME_INIT(title) \
	const char* __sTimeLabelTitle = title; \
	CTimeValue __tLast, __tCurr, __tStart; \
	__tLast = __tCurr = __tStart = gEnv->pTimer->GetAsyncTime();

#define DEBUG_PROFILE_TIME_LABEL(label) \
	__tCurr = gEnv->pTimer->GetAsyncTime(); {\
	CTimeValue __tDiffTotal = __tCurr - __tStart; \
	CTimeValue __tDiffLast = __tCurr - __tLast; \
	CryLogAlways("[%s] %s - Since last: %f, Total: %f, Timestamp %f", __sTimeLabelTitle, label, __tDiffLast.GetSeconds(), __tDiffTotal.GetSeconds(), __tCurr.GetSeconds()); }\
	__tLast = __tCurr;


class CDebugHelper;
class CDebugHelperAutoRegCVarBase
{
public:
	CDebugHelperAutoRegCVarBase( const char *varName )
	{
		m_varName = varName;
		m_pNext = 0;
		if (!GetLast())
			GetFirst() = this;
		else
			GetLast()->m_pNext = this;
		GetLast() = this;
	}

	virtual ~CDebugHelperAutoRegCVarBase() {}
	virtual ICVar* CreateVar() = 0;

protected:
	string m_varName;

private:
	friend class CDebugHelper;
	CDebugHelperAutoRegCVarBase* m_pNext;

	static CDebugHelperAutoRegCVarBase*& GetFirst()
	{
		static CDebugHelperAutoRegCVarBase* pFirst = NULL;
		return pFirst;
	}

	static CDebugHelperAutoRegCVarBase*& GetLast()
	{
		static CDebugHelperAutoRegCVarBase* pLast = NULL;
		return pLast;
	}
};


template <class T>
class CDebugHelperAutoRegCVar : public CDebugHelperAutoRegCVarBase
{
public:
	CDebugHelperAutoRegCVar( const char *varName, T* variable )
		: CDebugHelperAutoRegCVarBase(varName)
		, m_Variable(variable)
	{
	}

	virtual ICVar* CreateVar()
	{
		if (gEnv->pConsole)
		{
			return gEnv->pConsole->Register(m_varName, (m_Variable), (*m_Variable));
		}
		return NULL;
	}

private:
	T* m_Variable;
};

class CDebugHelper
{
public:
	static void Draw2dLabel(float x,float y, float fontSize, const float* pColor, const char* pText)
	{
		SDrawTextInfo ti;
		ti.xscale = ti.yscale = fontSize;
		ti.flags = eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace;
		if (pColor)
		{
			ti.color[0] = pColor[0];
			ti.color[1] = pColor[1];
			ti.color[2] = pColor[2];
			ti.color[3] = pColor[3];
		}
		gEnv->pRenderer->DrawTextQueued(Vec3(x, y, 0.5f), ti, pText);
	}


	static void DrawTextLabel(float x, float y, const float* pColor, const char* pFormat, ...)
	{
		char buffer[512];
		const size_t cnt = sizeof(buffer);

		va_list args;
		va_start(args, pFormat);
		int written = vsnprintf(buffer, cnt, pFormat, args);
		if (written < 0 || written == cnt)
			buffer[cnt-1] = '\0';
		va_end(args);

		Draw2dLabel(x, y, 1.4f, pColor, buffer);
	}

	static const float* ColorWhite()
	{
		static const float color[4] =    {1.0f, 1.0f, 1.0f, 1.0f};
		return color;
	}

	static void InitCVars()
	{
		static bool isInit = false;
		if (isInit) return;
		isInit = true;
		CDebugHelperAutoRegCVarBase* node = CDebugHelperAutoRegCVarBase::GetFirst();
		while (node)
		{
			node->CreateVar();
			node = node->m_pNext;
		}
	}

	static float& Py()
	{
		static float py = 0;
		return py;
	}
};

#endif
