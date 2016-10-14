/*
 * LexLua.c --
 *
 * Lexer for the Lua scripting language.
 *
 * Copyright (c) 2006-2007 Tim Baker.
 *
 * This file contains code derived from Scintilla (www.scintilla.org) which
 * is distributed with the following copyright:
 *
 *   "Copyright 1998-2003 by Neil Hodgson <neilh@scintilla.org>
 *
 *    All Rights Reserved 
 *
 *    Permission to use, copy, modify, and distribute this software and its 
 *    documentation for any purpose and without fee is hereby granted, 
 *    provided that the above copyright notice appear in all copies and that 
 *    both that copyright notice and this permission notice appear in 
 *    supporting documentation."
 *
 * RCS: @(#) $Id$
 */

#include "tkText.h"
#include "tkTextHighlight.h"

#define SCE_LUA_DEFAULT 0
#define SCE_LUA_WHITESPACE 1
#define SCE_LUA_WORD1 2
#define SCE_LUA_WORD2 3
#define SCE_LUA_WORD3 4
#define SCE_LUA_WORD4 5
#define SCE_LUA_WORD5 6
#define SCE_LUA_WORD6 7
#define SCE_LUA_WORD7 8
#define SCE_LUA_WORD8 9
#define SCE_LUA_WORD9 10
#define SCE_LUA_COMMENT 11
#define SCE_LUA_COMMENTLINE 12
#define SCE_LUA_COMMENTDOC 13
#define SCE_LUA_NUMBER 14
#define SCE_LUA_STRING 15
#define SCE_LUA_CHARACTER 16
#define SCE_LUA_LITERALSTRING 17
#define SCE_LUA_PREPROCESSOR 18
#define SCE_LUA_OPERATOR 19
#define SCE_LUA_IDENTIFIER 20
#define SCE_LUA_STRINGEOL 21
#define SCE_LUA_FUNCTION 22

static CONST char *styleNames[] = {
    "default", "whitespace", "keyword1", "keyword2", "keyword3", "keyword4",
    "keyword5", "keyword6", "keyword7", "keyword8", "keyword9", "comment",
    "commentline", "commentdoc", "number", "string", "character",
    "literalstring", "preprocessor", "operator", "identifier", "stringeol",
    "function", NULL
};

struct Lexer2
{
    struct Lexer lexer;		/* Required first field. */
    int foldCompact;
};

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BOOLEAN, "-foldcompact", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldCompact), 0, NULL, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0, 0, 0}
};

static bool IsAWordChar(const int ch)
{
    return (ch >= 0x80) || (isalnum(ch) || ch == '.' || ch == '_');
}

static bool IsAWordStart(const int ch)
{
    return (ch >= 0x80) || (isalnum(ch) || ch == '_');
}

static bool IsANumberChar(int ch) {
    /* Not exactly following number definition (several dots are seen as OK, etc.)
     * but probably enough in most cases. */
    return (ch < 0x80) &&
            (isdigit(ch) || toupper(ch) == 'E' ||
            ch == '.' || ch == '-' || ch == '+' ||
            (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F'));
}

static bool isLuaOperator(const int ch)
{
    if (ch >= 0x80 || isalnum(ch))
	return false;
    /* '.' left out as it is used to make up numbers */
    if (ch == '*' || ch == '/' || ch == '-' || ch == '+' ||
	ch == '(' || ch == ')' || ch == '=' ||
	ch == '{' || ch == '}' || ch == '~' ||
	ch == '[' || ch == ']' || ch == ';' ||
	ch == '<' || ch == '>' || ch == ',' ||
	ch == '.' || ch == '^' || ch == '%' || ch == ':' ||
	ch == '#') {
	return true;
    }
    return false;
}

/* Test for [=[ ... ]=] delimiters, returns 0 if it's only a [ or ],
 * return 1 for [[ or ]], returns >=2 for [=[ or ]=] and so on.
 * The maximum number of '=' characters allowed is 254. */
static int LongDelimCheck(void) {
    int sep = 1;
    while (GetRelative(sep) == '=' && sep < 0xFF)
	sep++;
    if (GetRelative(sep) == vars.chCur)
	return sep;
    return 0;
}

static int ColouriseLuaDoc(LexerArgs *args)
{
    /* Initialize long string [[ ... ]] or block comment --[[ ... ]] nesting level,
     * if we are inside such a string. Block comment was introduced in Lua 5.0,
     * blocks with separators [=[ ... ]=] in Lua 5.1. */
    int nestLevel = 0;
    int sepCount = 0;

    BeginStyling(args, args->firstLine, args->lastLine);

    /* Must initialize the literal string nesting level, if we are inside such a string. */
    if (vars.style == SCE_LUA_LITERALSTRING || vars.style == SCE_LUA_COMMENT)
    {
	int lineState = GetLineState(NULL, vars.linePrevPtr);
	nestLevel = lineState >> 8;
	sepCount = lineState & 0xFF;
    }

    /* Do not leak onto next line */
    if (vars.style == SCE_LUA_STRINGEOL ||
	    vars.style == SCE_LUA_COMMENTLINE ||
	    vars.style == SCE_LUA_PREPROCESSOR) {
	vars.style = SCE_LUA_DEFAULT;
    }

    if (vars.charIndex == 0 && vars.chCur == '#') {
	/* shbang line: # is a comment only if first char of the script */
	SetStyle(SCE_LUA_COMMENTLINE);
    }

    for (; More(); Forward())
    {
	if (vars.atLineStart) {
	    if (vars.style == SCE_LUA_STRINGEOL) {
		SetStyle(SCE_LUA_DEFAULT);
	    }
	}

	/* Determine if the current style should terminate. */
	if (vars.style == SCE_LUA_WHITESPACE) {
	    if (!IsASpaceOrTab(vars.chCur)) {
		SetStyle(SCE_LUA_DEFAULT);
	    }
	} else if (vars.style == SCE_LUA_OPERATOR) {
	    SetStyle(SCE_LUA_DEFAULT);
	} else if (vars.style == SCE_LUA_NUMBER) {
	    /* We stop the number definition on non-numerical non-dot non-eE non-sign non-hexdigit char */
	    if (!IsANumberChar(vars.chCur)) {
		SetStyle(SCE_LUA_DEFAULT);
	    }
	} else if (vars.style == SCE_LUA_IDENTIFIER) {
	    if (!IsAWordChar(vars.chCur) || (vars.chCur == '.')) {
		char s[100];
		int len = GetCurrent(s, sizeof(s));
		int kw;

		if (MatchKeyword(s, len, &kw)) {
		    ChangeState(SCE_LUA_WORD1 + kw);
		}
#if 0
		else if (MatchCh('('))
		{
		    ChangeState(SCE_LUA_FUNCTION);
		}
#endif
		SetStyle(SCE_LUA_DEFAULT);
	    }
	} else if (vars.style == SCE_LUA_COMMENTLINE ||
		vars.style == SCE_LUA_PREPROCESSOR) {
	    if (vars.atLineEnd) {
		SetStyle(SCE_LUA_DEFAULT);
	    }
	} else if (vars.style == SCE_LUA_STRING) {
	    if (vars.chCur == '\\') {
		if (vars.chNext == '\"' || vars.chNext == '\'' ||
		    vars.chNext == '\\') {
		    Forward();
		}
	    } else if (vars.chCur == '\"') {
		ForwardSetStyle(SCE_LUA_DEFAULT);
	    } else if (vars.atLineEnd) {
		if (vars.chPrev != '\\' || IsEscaped(-1)) {
		    ChangeState(SCE_LUA_STRINGEOL);
		}
	    }
	} else if (vars.style == SCE_LUA_CHARACTER) {
	    if (vars.chCur == '\\') {
		if (vars.chNext == '\"' || vars.chNext == '\'' ||
		    vars.chNext == '\\') {
		    Forward();
		}
	    } else if (vars.chCur == '\'') {
		ForwardSetStyle(SCE_LUA_DEFAULT);
	    } else if (vars.atLineEnd) {
		if (vars.chPrev != '\\' || IsEscaped(-1)) {
		    ChangeState(SCE_LUA_STRINGEOL);
		}
	    }
	} else if (vars.style == SCE_LUA_LITERALSTRING ||
		vars.style == SCE_LUA_COMMENT) {
	    if (MatchCh('[')) {
		int sep = LongDelimCheck();
		if (sep == 1 && sepCount == 1) {     /* [[-only allowed to nest */
		    nestLevel++;
		    Forward();
		}
	    } else if (MatchCh(']')) {
		int sep = LongDelimCheck();
		if (sep == 1 && sepCount == 1) {     /* un-nest with ]]-only */
		    nestLevel--;
		    Forward();
		    if (nestLevel == 0) {
			ForwardSetStyle(SCE_LUA_DEFAULT);
		    }
		} else if (sep > 1 && sep == sepCount) {	/* ]=]-style delim */
		    ForwardN(sep);
		    ForwardSetStyle(SCE_LUA_DEFAULT);
		}
	    }
	}

	if (vars.atLineEnd) {
	    /* Update the line state, so it can be seen by next line */
	    switch (vars.style) {
		case SCE_LUA_LITERALSTRING:
		case SCE_LUA_COMMENT:
		    /* Inside a literal string or block comment, we set the line state */
		    SetLineState(NULL, vars.linePtr, (nestLevel << 8) | sepCount);
		    break;
		default:
		    /* Reset the line state */
		    SetLineState(NULL, vars.linePtr, 0);
		    break;
	    }
	    continue;
	}

	/* Determine if a new style should begin. */
	if (vars.style == SCE_LUA_DEFAULT) {
	    if (IsASpaceOrTab(vars.chCur)) {
		SetStyle(SCE_LUA_WHITESPACE);
	    } else if (IsADigit(vars.chCur) || (vars.chCur == '.' && IsADigit(vars.chNext))) {
		SetStyle(SCE_LUA_NUMBER);
		if (MatchCh('0') && toupper(vars.chNext) == 'X') {
		    ForwardN(1);
		}
	    } else if (IsAWordStart(vars.chCur)) {
		SetStyle(SCE_LUA_IDENTIFIER);
	    } else if (MatchCh('\"')) {
		SetStyle(SCE_LUA_STRING);
	    } else if (MatchCh('\'')) {
		SetStyle(SCE_LUA_CHARACTER);
	    } else if (MatchCh('['))
	    {
		sepCount = LongDelimCheck();
		if (sepCount == 0) {
		    SetStyle(SCE_LUA_OPERATOR);
		} else {
		    nestLevel = 1;
		    SetStyle(SCE_LUA_LITERALSTRING);
		    ForwardN(sepCount);
		}
	    } else if (MatchChCh('-', '-')) {
		SetStyle(SCE_LUA_COMMENTLINE);
		if (MatchStr("--[")) {
		    ForwardN(2);
		    sepCount = LongDelimCheck();
		    if (sepCount > 0) {
			nestLevel = 1;
			ChangeState(SCE_LUA_COMMENT);
			ForwardN(sepCount);
		    }
		} else {
		    Forward();
		}
	    } else if (vars.atLineStart && MatchCh('$')) {
		SetStyle(SCE_LUA_PREPROCESSOR);		/* Obsolete since Lua 4.0, but still in old code */
	    } else if (isLuaOperator(vars.chCur)) {
		SetStyle(SCE_LUA_OPERATOR);
	    }
	}
    }
    Complete();
    return 0;
}

static int FoldLuaDoc(LexerArgs *args)
{
    struct Lexer2 *lexer = (struct Lexer2 *) args->lexer;
    bool foldCompact = lexer->foldCompact;
    int visibleChars = 0;
    int levelPrev, levelCurrent;
    char s[10];

    BeginStyling(args, args->firstLine, args->lastLine);

    levelPrev = vars.linePtr->level & SC_FOLDLEVELNUMBERMASK;
    levelCurrent = levelPrev;

    for ( ; More(); Forward() ) {

	if (vars.style == SCE_LUA_WORD1) {
	    if (MatchCh('i') || MatchCh('d') || MatchCh('f') || MatchCh('e') || MatchCh('r') || MatchCh('u')) {
		unsigned int j;
		for (j = 0; j < 8; j++) {
		    if (!iswordchar(GetRelative(j))) {
			break;
		    }
		    s[j] = GetRelative(j);
		    s[j + 1] = '\0';
		}

		if ((strcmp(s, "if") == 0) || (strcmp(s, "do") == 0) || (strcmp(s, "function") == 0) || (strcmp(s, "repeat") == 0)) {
		    levelCurrent++;
		}
		if ((strcmp(s, "end") == 0) || (strcmp(s, "elseif") == 0) || (strcmp(s, "until") == 0)) {
		    levelCurrent--;
		}
	    }
	} else if (vars.style == SCE_LUA_OPERATOR) {
	    if (MatchCh('{') || MatchCh('(')) {
		levelCurrent++;
	    } else if (MatchCh('}') || MatchCh(')')) {
		levelCurrent--;
	    }
	} else if (vars.style == SCE_LUA_LITERALSTRING || vars.style == SCE_LUA_COMMENT) {
	    if (MatchCh('[')) {
		levelCurrent++;
	    } else if (MatchCh(']')) {
		levelCurrent--;
	    }
	}

	if (vars.atLineEnd) {
	    int lev = 0 /* levelPrev */;
	    if (visibleChars == 0 && foldCompact) {
		lev |= SC_FOLDLEVELWHITEFLAG;
	    }
	    if ((levelCurrent > levelPrev) && (visibleChars > 0)) {
		lev |= SC_FOLDLEVELHEADERFLAG;
	    }
	    SetLineFoldLevel(vars.sharedPtr, vars.linePtr, levelPrev, lev);
	    levelPrev = levelCurrent;
	    visibleChars = 0;
	}
	if (!isspacechar(vars.chCur)) {
	    visibleChars++;
	}
    }
    /* Fill in the real level of the next line, keeping the current flags as they will be filled in later */
    if (vars.lineNextPtr != NULL) {
	int flagsNext = vars.lineNextPtr->level & ~SC_FOLDLEVELNUMBERMASK;
	SetLineFoldLevel(vars.sharedPtr, vars.lineNextPtr, levelPrev, flagsNext);
    }

    return 0;
}

LexerModule lmLua = {
    "lua",
    sizeof(struct Lexer2),
    styleNames,
    ColouriseLuaDoc,
    FoldLuaDoc,
    optionSpecs
};

