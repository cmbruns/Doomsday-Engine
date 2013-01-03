#ifndef DOOMSDAY_API_CONSOLE_H
#define DOOMSDAY_API_CONSOLE_H

#include "api_base.h"
#include "dd_share.h"

/// @addtogroup console
///@{

DENG_API_TYPEDEF(Con)
{
    de_api_t api;

    void (*Open)(int yes);
    void (*AddCommand)(ccmdtemplate_t const* cmd);
    void (*AddVariable)(cvartemplate_t const* var);
    void (*AddCommandList)(ccmdtemplate_t const* cmdList);
    void (*AddVariableList)(cvartemplate_t const* varList);

    cvartype_t (*GetVariableType)(char const* name);

    byte (*GetByte)(char const* name);
    int (*GetInteger)(char const* name);
    float (*GetFloat)(char const* name);
    char const* (*GetString)(char const* name);
    Uri const* (*GetUri)(char const* name);

    void (*SetInteger2)(char const* name, int value, int svflags);
    void (*SetInteger)(char const* name, int value);

    void (*SetFloat2)(char const* name, float value, int svflags);
    void (*SetFloat)(char const* name, float value);

    void (*SetString2)(char const* name, char const* text, int svflags);
    void (*SetString)(char const* name, char const* text);

    void (*SetUri2)(char const* name, Uri const* uri, int svflags);
    void (*SetUri)(char const* name, Uri const* uri);

    void (*Message)(char const* message, ...);

    void (*Printf)(char const* format, ...);
    void (*FPrintf)(int flags, char const* format, ...);
    void (*PrintRuler)(void);
    void (*Error)(char const* error, ...);

    void (*SetPrintFilter)(con_textfilter_t filter);

    int (*Execute)(int silent, char const* command);
    int (*Executef)(int silent, char const* command, ...);
}
DENG_API_T(Con);

#ifndef DENG_NO_API_MACROS_CONSOLE
#define Con_Open                _api_Con.Open
#define Con_AddCommand          _api_Con.AddCommand
#define Con_AddVariable         _api_Con.AddVariable
#define Con_AddCommandList      _api_Con.AddCommandList
#define Con_AddVariableList     _api_Con.AddVariableList

#define Con_GetVariableType     _api_Con.GetVariableType

#define Con_GetByte             _api_Con.GetByte
#define Con_GetInteger          _api_Con.GetInteger
#define Con_GetFloat            _api_Con.GetFloat
#define Con_GetString           _api_Con.GetString
#define Con_GetUri              _api_Con.GetUri

#define Con_SetInteger2         _api_Con.SetInteger2
#define Con_SetInteger          _api_Con.SetInteger

#define Con_SetFloat2           _api_Con.SetFloat2
#define Con_SetFloat            _api_Con.SetFloat

#define Con_SetString2          _api_Con.SetString2
#define Con_SetString           _api_Con.SetString

#define Con_SetUri2             _api_Con.SetUri2
#define Con_SetUri              _api_Con.SetUri

#define Con_Message             _api_Con.Message

#define Con_Printf              _api_Con.Printf
#define Con_FPrintf             _api_Con.FPrintf
#define Con_PrintRuler          _api_Con.PrintRuler
#define Con_Error               _api_Con.Error

#define Con_SetPrintFilter      _api_Con.SetPrintFilter

#define DD_Execute              _api_Con.Execute
#define DD_Executef             _api_Con.Executef
#endif

#ifdef __DOOMSDAY__
DENG_USING_API(Con);
#endif

///@}

#endif // DOOMSDAY_API_CONSOLE_H
