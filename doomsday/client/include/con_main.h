/** @file con_main.h
 *
 * @authors Copyright © 2003-2013 Jaakko Keränen <jaakko.keranen@iki.fi>
 * @authors Copyright © 2005-2013 Daniel Swanson <danij@dengine.net>
 *
 * @par License
 * GPL: http://www.gnu.org/licenses/gpl.html
 *
 * <small>This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version. This program is distributed in the hope that it
 * will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details. You should have received a copy of the GNU
 * General Public License along with this program; if not, see:
 * http://www.gnu.org/licenses</small>
 */

/**
 * Console Subsystem.
 */

#ifndef LIBDENG_CONSOLE_MAIN_H
#define LIBDENG_CONSOLE_MAIN_H

#include <stdio.h>
#include "dd_share.h"
#include "dd_types.h"
#include "de_system.h"
#include "ui/dd_input.h"
#include "Game"

#include <de/shell/Lexicon> // known words

#define CMDLINE_SIZE 256
#define MAX_ARGS            256

#define OBSOLETE            CVF_NO_ARCHIVE|CVF_HIDE

// Macros for accessing the console variable values through the shared data ptr.
#define CV_INT(var)         (*(int*) var->ptr)
#define CV_BYTE(var)        (*(byte*) var->ptr)
#define CV_FLOAT(var)       (*(float*) var->ptr)
#define CV_CHARPTR(var)     (*(char**) var->ptr)
#define CV_URIPTR(var)      (*(Uri**) var->ptr)

struct cbuffer_s;

typedef struct {
    char cmdLine[2048];
    int argc;
    char* argv[MAX_ARGS];
} cmdargs_t;

typedef struct ccmd_s {
    /// Next command in the global list.
    struct ccmd_s* next;

    /// Next and previous overloaded versions of this command (if any).
    struct ccmd_s* nextOverload, *prevOverload;

    /// Execute function.
    int (*execFunc) (byte src, int argc, char** argv);

    /// Name of the command.
    const char* name;

    /// @ref consoleCommandFlags
    int flags;

    /// Minimum and maximum number of arguments. Used with commands
    /// that utilize an engine-validated argument list.
    int minArgs, maxArgs;

    /// List of argument types for this command.
    cvartype_t args[MAX_ARGS];
} ccmd_t;

typedef struct cvar_s {
    /// @ref consoleVariableFlags
    int flags;

    /// Type of this variable.
    cvartype_t type;

    /// Pointer to this variable's node in the directory.
    void* directoryNode;

    /// Pointer to the user data.
    void* ptr;

    /// Minimum and maximum values (for ints and floats).
    float min, max;

    /// On-change notification callback.
    void (*notifyChanged)(void);
} cvar_t;

ddstring_t const* CVar_TypeName(cvartype_t type);

/// @return  @ref consoleVariableFlags
int CVar_Flags(cvar_t const* var);

/// @return  Type of the variable.
cvartype_t CVar_Type(cvar_t const* var);

/// @return  Symbolic name/path-to the variable.
AutoStr* CVar_ComposePath(cvar_t const* var);

int CVar_Integer(cvar_t const* var);
float CVar_Float(cvar_t const* var);
byte CVar_Byte(cvar_t const* var);
char const* CVar_String(cvar_t const* var);
Uri const* CVar_Uri(cvar_t const* var);

/**
 * Changes the value of an integer variable.
 * @note Also used with @c CVT_BYTE.
 *
 * @param var      Variable.
 * @param value    New integer value for the variable.
 */
void CVar_SetInteger(cvar_t* var, int value);

/**
 * @copydoc CVar_SetInteger()
 * @param svflags  @ref setVariableFlags
 */
void CVar_SetInteger2(cvar_t* var, int value, int svflags);

void CVar_SetFloat2(cvar_t* var, float value, int svflags);
void CVar_SetFloat(cvar_t* var, float value);

void CVar_SetString2(cvar_t* var, char const* text, int svflags);
void CVar_SetString(cvar_t* var, char const* text);

void CVar_SetUri2(cvar_t* var, Uri const* uri, int svflags);
void CVar_SetUri(cvar_t* var, Uri const* uri);

typedef enum {
    WT_ANY = -1,
    KNOWNWORDTYPE_FIRST = 0,
    WT_CCMD = KNOWNWORDTYPE_FIRST,
    WT_CVAR,
    WT_CALIAS,
    WT_GAME,
    KNOWNWORDTYPE_COUNT
} knownwordtype_t;

#define VALID_KNOWNWORDTYPE(t)      ((t) >= KNOWNWORDTYPE_FIRST && (t) < KNOWNWORDTYPE_COUNT)

typedef struct knownword_s {
    knownwordtype_t type;
    void* data;
} knownword_t;

typedef struct calias_s {
    /// Name of this alias.
    char* name;

    /// Aliased command string.
    char* command;
} calias_t;

// Console commands can set this when they need to return a custom value
// e.g. for the game library.
extern int CmdReturnValue;
extern byte consoleDump;

void Con_Register(void);
void Con_DataRegister(void);

boolean Con_Init(void);
void Con_Shutdown(void);

void Con_InitDatabases(void);
void Con_ClearDatabases(void);
void Con_ShutdownDatabases(void);

void Con_Ticker(timespan_t time);

/// @return  @c true iff the event is 'eaten'.
//boolean Con_Responder(ddevent_t const *ev);

/**
 * Attempt to change the 'open' state of the console.
 * \note In dedicated mode the console cannot be closed.
 */
void Con_Open(int yes);

/// To be called after a resolution change to resize the console.
//void Con_Resize(void);

//boolean Con_IsActive(void);

//boolean Con_IsLocked(void);

//boolean Con_InputMode(void);

//char* Con_CommandLine(void);

//uint Con_CommandLineCursorPosition(void);

#if 0
struct cbuffer_s* Con_HistoryBuffer(void);

uint Con_HistoryOffset(void);

fontid_t Con_Font(void);

void Con_SetFont(fontid_t font);

con_textfilter_t Con_PrintFilter(void);

void Con_SetPrintFilter(con_textfilter_t filter);

void Con_FontScale(float* scaleX, float* scaleY);

void Con_SetFontScale(float scaleX, float scaleY);

float Con_FontLeading(void);

void Con_SetFontLeading(float value);

int Con_FontTracking(void);

void Con_SetFontTracking(int value);
#endif

void Con_AddCommand(const ccmdtemplate_t* cmd);
void Con_AddCommandList(const ccmdtemplate_t* cmdList);

/**
 * Search the console database for a named command. If one or more overloaded
 * variants exist then return the variant registered most recently.
 *
 * @param name              Name of the command to search for.
 * @return  Found command else @c 0
 */
ccmd_t* Con_FindCommand(const char* name);

/**
 * Search the console database for a command. If one or more overloaded variants
 * exist use the argument list to select the required variant.
 *
 * @param args
 * @return  Found command else @c 0
 */
ccmd_t* Con_FindCommandMatchArgs(cmdargs_t* args);

void Con_AddVariable(const cvartemplate_t* tpl);
void Con_AddVariableList(const cvartemplate_t* tplList);
cvar_t* Con_FindVariable(const char* path);

/// @return  Type of the variable associated with @a path if found else @c CVT_NULL
cvartype_t Con_GetVariableType(char const* path);

int Con_GetInteger(char const* path);
float Con_GetFloat(char const* path);
byte Con_GetByte(char const* path);
char const* Con_GetString(char const* path);
Uri const* Con_GetUri(char const* path);

void Con_SetInteger2(char const* path, int value, int svflags);
void Con_SetInteger(char const* path, int value);

void Con_SetFloat2(char const* path, float value, int svflags);
void Con_SetFloat(char const* path, float value);

void Con_SetString2(char const* path, char const* text, int svflags);
void Con_SetString(char const* path, char const* text);

void Con_SetUri2(char const* path, Uri const* uri, int svflags);
void Con_SetUri(char const* path, Uri const* uri);

calias_t* Con_AddAlias(const char* name, const char* command);

/**
 * @return  @c 0 if the specified alias can't be found.
 */
calias_t* Con_FindAlias(const char* name);

void Con_DeleteAlias(calias_t* cal);

/**
 * @return  @c true iff @a name matches a known command or alias name.
 */
boolean Con_IsValidCommand(const char* name);

/**
 * Attempt to execute a console command.
 *
 * @param src               The source of the command (@ref commandSource)
 * @param command           The command to be executed.
 * @param silent            Non-zero indicates not to log execution of the command.
 * @param netCmd            If @c true, command was sent over the net.
 *
 * @return  Non-zero if successful else @c 0.
 */
int Con_Execute(byte src, const char* command, int silent, boolean netCmd);
int Con_Executef(byte src, int silent, const char* command, ...) PRINTF_F(3,4);

/**
 * Print an error message and quit.
 */
void Con_Error(const char* error, ...);
void Con_AbnormalShutdown(const char* error);

/**
 * Iterate over words in the known-word dictionary, making a callback for each.
 * Iteration ends when all selected words have been visited or a callback returns
 * non-zero.
 *
 * @param pattern           If not @c NULL or an empty string, only process those
 *                          words which match this pattern.
 * @param type              If a valid word type, only process words of this type.
 * @param callback          Callback to make for each processed word.
 * @param parameters        Passed to the callback.
 *
 * @return  @c 0 iff iteration completed wholly.
 */
int Con_IterateKnownWords(const char* pattern, knownwordtype_t type,
    int (*callback)(const knownword_t* word, void* parameters), void* parameters);

enum KnownWordMatchMode {
    KnownWordExactMatch, // case insensitive
    KnownWordStartsWith  // case insensitive
};

int Con_IterateKnownWords(KnownWordMatchMode matchMode,
                          const char* pattern, knownwordtype_t type,
                          int (*callback)(const knownword_t* word, void* parameters),
                          void* parameters);

/**
 * Collect an array of knownWords which match the given word (at least
 * partially).
 *
 * \note: The array must be freed with free()
 *
 * @param word              The word to be matched.
 * @param type              If a valid word type, only collect words of this type.
 * @param count             If not @c NULL the matched word count is written back here.
 *
 * @return  A NULL-terminated array of pointers to all the known words which
 *          match the search criteria.
 */
const knownword_t** Con_CollectKnownWordsMatchingWord(const char* word,
    knownwordtype_t type, unsigned int* count);

AutoStr *Con_KnownWordToString(knownword_t const *word);

/**
 * Print a 'global' message (to stdout and the console) consisting of (at least)
 * one full line of text.
 *
 * @param message  Message with printf() formatting syntax for arguments.
 *                 The terminating line break character may be omitted, however
 *                 the message cannot be continued in a subsequent call.
 */
void Con_Message(char const *message, ...) PRINTF_F(1,2);

/**
 * Print a text fragment (manual/no line breaks) to the console.
 *
 * @param flags   @ref consolePrintFlags
 * @param format  Format for the output using printf() formatting syntax.
 */
void Con_FPrintf(int flags, char const *format, ...) PRINTF_F(2,3);
void Con_Printf(char const *format, ...) PRINTF_F(1,2);

/// Print a ruler into the console.
void Con_PrintRuler(void);

/**
 * @defgroup printPathFlags Print Path Flags
 * @ingroup flags
 */
/*{@*/
#define PPF_MULTILINE           0x1 // Use multiple lines.
#define PPF_TRANSFORM_PATH_MAKEPRETTY 0x2 // Make paths 'prettier'.
#define PPF_TRANSFORM_PATH_PRINTINDEX 0x4 // Print an index for each path.
/*}@*/

#define DEFAULT_PRINTPATHFLAGS (PPF_MULTILINE|PPF_TRANSFORM_PATH_MAKEPRETTY|PPF_TRANSFORM_PATH_PRINTINDEX)

/**
 * Prints the passed path list to the console.
 *
 * @todo treat paths as URIs (i.e., resolve symbols).
 *
 * @param pathList   A series of file/resource names/paths separated by @a delimiter.
 * @param delimiter  Path delimiter character.
 * @param separator  Text printed between list entries.
 * @param flags      @ref printPathFlags.
 */
void Con_PrintPathList4(const char* pathList, char delimiter, const char* separator, int flags);
void Con_PrintPathList3(const char* pathList, char delimiter, const char* separator); /* flags = DEFAULT_PRINTPATHFLAGS */
void Con_PrintPathList2(const char* pathList, char delimiter); /* separator = " " */
void Con_PrintPathList(const char* pathList); /* delimiter = ';' */

void Con_PrintCVar(cvar_t* cvar, const char* prefix);

de::String Con_VarAsStyledText(cvar_t *var, char const *prefix);
de::String Con_CmdAsStyledText(ccmd_t *cmd);
de::String Con_AliasAsStyledText(calias_t *alias);
de::String Con_GameAsStyledText(de::Game *game);

de::String Con_AnnotatedConsoleTerms(QStringList terms);

/**
 * Outputs the usage information for the given ccmd to the console if the
 * ccmd's usage is validated by Doomsday.
 *
 * @param ccmd  Ptr to the ccmd to print the usage info for.
 * @param printInfo  If @c true, print any additional info we have.
 */
void Con_PrintCCmdUsage(ccmd_t* ccmd, boolean printInfo);

/**
 * Collects all the known words of the console into a Lexicon.
 */
de::shell::Lexicon Con_Lexicon();

#endif /* LIBDENG_CONSOLE_MAIN_H */
