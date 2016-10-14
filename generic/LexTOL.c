/*
 * LexCPP.c --
 *
 * Lexer for C and C++.
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

#define SCE_C_DEFAULT 0
#define SCE_C_WHITESPACE 1
#define SCE_C_WORD1 2
#define SCE_C_WORD2 3
#define SCE_C_WORD3 4
#define SCE_C_WORD4 5
#define SCE_C_WORD5 6
#define SCE_C_WORD6 7
#define SCE_C_WORD7 8
#define SCE_C_WORD8 9
#define SCE_C_WORD9 10
#define SCE_C_COMMENT 11
#define SCE_C_COMMENTLINE 12
#define SCE_C_COMMENTDOC 13
#define SCE_C_NUMBER 14
#define SCE_C_STRING 15
#define SCE_C_CHARACTER 16
#define SCE_C_UUID 17
#define SCE_C_PREPROCESSOR 18
#define SCE_C_OPERATOR 19
#define SCE_C_IDENTIFIER 20
#define SCE_C_VERBATIM 21
#define SCE_C_REGEX 22
#define SCE_C_COMMENTLINEDOC 23
#define SCE_C_COMMENTDOCKEYWORD 24
#define SCE_C_COMMENTDOCKEYWORDERROR 25
#define SCE_C_FUNCTION 26
#define SCE_TOL_AtName 27

static CONST char *styleNames[] = {
    "default", "whitespace", "keyword1", "keyword2", "keyword3", "keyword4",
    "keyword5", "keyword6", "keyword7", "keyword8", "keyword9", "comment",
    "commentline", "commentdoc", "number", "string", "character", "uuid",
    "preprocessor", "operator", "identifier", "verbatim",
    "regex", "commentlinedoc", "commentdockeyword", "commentdockeyworderror",
    "function", "class_id", NULL
};

struct Lexer2
{
    struct Lexer lexer;		/* Required first field. */
    int stylingWithinPreprocessor;
    int foldComment;
    int foldPreprocessor;
    int foldCompact;
    int foldAtElse;
};

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BOOLEAN, "-foldatelse", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldAtElse), 0, NULL, 0},
    {TK_OPTION_BOOLEAN, "-foldcomment", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldComment), 0, NULL, 0},
    {TK_OPTION_BOOLEAN, "-foldcompact", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldCompact), 0, NULL, 0},
    {TK_OPTION_BOOLEAN, "-foldpreprocessor", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldPreprocessor), 0, NULL, 0},
    {TK_OPTION_BOOLEAN, "-stylingwithinpreprocessor", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, stylingWithinPreprocessor), 0, NULL, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0, 0, 0}
};

static bool IsOKBeforeRE(const int ch) {
    return (ch == '(') || (ch == '=') || (ch == ',');
}

static bool IsAWordChar(const int ch) {
    return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_');
}

static bool IsAWordStart(const int ch) {
    return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static bool IsADoxygenChar(const int ch) {
    return (islower(ch) || ch == '$' || ch == '@' ||
	    ch == '\\' || ch == '&' || ch == '<' ||
	    ch == '>' || ch == '#' || ch == '{' ||
	    ch == '}' || ch == '[' || ch == ']');
}

static int ColorizeTOL(LexerArgs *args)
{
    struct Lexer2 *lexer = (struct Lexer2 *) args->lexer;
    bool stylingWithinPreprocessor = lexer->stylingWithinPreprocessor;
    int chPrevNonWhite = ' ';
    int visibleChars = 0;
    bool lastWordWasUUID = false;

    BeginStyling(args, args->firstLine, args->lastLine);

    /* Do not leak onto next line */
    /*
    if (vars.style == SCE_C_STRINGEOL)
	vars.style = SCE_C_DEFAULT;
    */
    for ( ; More(); Forward() ) {

	if (vars.atLineStart) {

	    /* Do not leak onto next line */
            /*
	    if (vars.style == SCE_C_STRINGEOL)
		vars.style = SCE_C_DEFAULT;
            */
	    /* Reset states to begining of colourise so no surprises 
	     * if different sets of lines lexed. */
	    chPrevNonWhite = ' ';
	    visibleChars = 0;
	    lastWordWasUUID = false;
	}

	/* Handle line continuation generically. */
	if (vars.chCur == '\\') {
	    if (MatchStr("\\\n")) {
		ForwardN(1);
		continue;
	    }
	    if (MatchStr("\\\r\n")) {
		ForwardN(2);
		continue;
	    }
	}

	/* Determine if the current style should terminate. */
	if (vars.style == SCE_C_WHITESPACE) {
	    if (!IsASpaceOrTab(vars.chCur)) {
		SetStyle(SCE_C_DEFAULT);
	    }
	} else if (vars.style == SCE_C_OPERATOR) {
	    SetStyle(SCE_C_DEFAULT);
	} else if (vars.style == SCE_C_NUMBER) {
	    if (!IsAWordChar(vars.chCur)) {
		SetStyle(SCE_C_DEFAULT);
	    }
	} else if (vars.style == SCE_C_IDENTIFIER) {
	    if (!IsAWordChar(vars.chCur) || (vars.chCur == '.')) {
		char s[100];
		int len = GetCurrent(s, sizeof(s));
		int kw;

		if (MatchKeyword(s, len, &kw)) {
		    ChangeState(SCE_C_WORD1 + kw);
		    if (kw == 0)
			lastWordWasUUID = strcmp(s, "uuid") == 0;
#ifdef STEXT_DIFF
		} else if (MatchCh('(')) {
		    ChangeState(SCE_C_FUNCTION);
/*				} else {
 *					ChangeState(SCE_C_DEFAULT); */
#endif
		}
		SetStyle(SCE_C_DEFAULT);
	    }
	} else if (vars.style == SCE_C_PREPROCESSOR) {
	    if (stylingWithinPreprocessor) {
		if (IsASpace(vars.chCur)) {
		    SetStyle(SCE_C_DEFAULT);
		}
	    } else {
		if (vars.atLineEnd) {
		    SetStyle(SCE_C_DEFAULT);
		}
	    }
	} else if (vars.style == SCE_C_COMMENT) {
	    if (MatchChCh('*', '/')) {
		Forward();
		ForwardSetStyle(SCE_C_DEFAULT);
	    }
	} else if (vars.style == SCE_C_COMMENTDOC) {
	    if (MatchChCh('*', '/')) {
		Forward();
		ForwardSetStyle(SCE_C_DEFAULT);
	    } else if (vars.chCur == '@' || vars.chCur == '\\') {
		SetStyle(SCE_C_COMMENTDOCKEYWORD);
	    }
	} else if (vars.style == SCE_C_COMMENTLINE ||
		vars.style == SCE_C_COMMENTLINEDOC) {
	    if (vars.atLineEnd) {
		SetStyle(SCE_C_DEFAULT);
	    }
	} else if (vars.style == SCE_C_COMMENTDOCKEYWORD) {
	    if (MatchChCh('*', '/')) {
		ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);
		Forward();
		ForwardSetStyle(SCE_C_DEFAULT);
	    } else if (!IsADoxygenChar(vars.chCur)) {
		char s[100];
		int len = GetCurrent(s, sizeof(s));
		if (!isspace(vars.chCur) || !InList(vars.wordListPtrs[3-1], s+1, len)) {
		    ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);
		}
		SetStyle(SCE_C_COMMENTDOC);
	    }
	} else if (vars.style == SCE_C_STRING) {
	    if (vars.chCur == '\\') {
		if (vars.chNext == '\"' || vars.chNext == '\'' || vars.chNext == '\\') {
		    Forward();
		}
	    } else if (vars.chCur == '\"') {
		ForwardSetStyle(SCE_C_DEFAULT);
	    }/* else if (vars.atLineEnd) {
		ChangeState(SCE_C_STRINGEOL);
                }*/
	} else if (vars.style == SCE_C_CHARACTER) {
          /*if (vars.atLineEnd) {
		ChangeState(SCE_C_STRINGEOL);
                } else*/ if (vars.chCur == '\\') {
		if (vars.chNext == '\"' || vars.chNext == '\'' || vars.chNext == '\\') {
		    Forward();
		}
	    } else if (vars.chCur == '\'') {
		ForwardSetStyle(SCE_C_DEFAULT);
	    }
	} else if (vars.style == SCE_C_REGEX) {
	    if (vars.chCur == '\r' || vars.chCur == '\n' || vars.chCur == '/') {
		ForwardSetStyle(SCE_C_DEFAULT);
	    } else if (vars.chCur == '\\') {
		/* Gobble up the quoted character */
		if (vars.chNext == '\\' || vars.chNext == '/') {
		    Forward();
		}
	    }
	} else if (vars.style == SCE_C_VERBATIM) {
	    if (vars.chCur == '\"') {
		if (vars.chNext == '\"') {
		    Forward();
		} else {
		    ForwardSetStyle(SCE_C_DEFAULT);
		}
	    }
	} else if (vars.style == SCE_C_UUID) {
	    if (vars.chCur == '\r' || vars.chCur == '\n' || vars.chCur == ')') {
		SetStyle(SCE_C_DEFAULT);
	    }
	}

	if (vars.atLineEnd)
	    continue;

	/* Determine if a new style should begin. */
	if (vars.style == SCE_C_DEFAULT) {
	    if (IsASpaceOrTab(vars.chCur)) {
		SetStyle(SCE_C_WHITESPACE);
	    } else if (MatchChCh('@', '\"')) {
		SetStyle(SCE_C_VERBATIM);
		Forward();
	    } else if (IsADigit(vars.chCur) || (vars.chCur == '.' && IsADigit(vars.chNext))) {
		if (lastWordWasUUID) {
		    SetStyle(SCE_C_UUID);
		    lastWordWasUUID = false;
		} else {
		    SetStyle(SCE_C_NUMBER);
		}
	    } else if (IsAWordStart(vars.chCur) || (vars.chCur == '@')) {
		if (lastWordWasUUID) {
		    SetStyle(SCE_C_UUID);
		    lastWordWasUUID = false;
		} else {
		    SetStyle(SCE_C_IDENTIFIER);
		}
	    } else if (MatchChCh('/', '*')) {
		if (MatchStr("/**") || MatchStr("/*!")) {	/* Support of Qt/Doxygen doc. style */
		    SetStyle(SCE_C_COMMENTDOC);
		} else {
		    SetStyle(SCE_C_COMMENT);
		}
		Forward();	/* Eat the * so it isn't used for the end of the comment */
	    } else if (MatchChCh('/', '/')) {
		if (MatchStr("///") || MatchStr("//!"))	{/* Support of Qt/Doxygen doc. style */
		    SetStyle(SCE_C_COMMENTLINEDOC);
		} else {
		    SetStyle(SCE_C_COMMENTLINE);
		}
	    } else if (vars.chCur == '/' && IsOKBeforeRE(chPrevNonWhite)) {
		SetStyle(SCE_C_REGEX);
	    } else if (vars.chCur == '\"') {
		SetStyle(SCE_C_STRING);
	    } else if (vars.chCur == '\'') {
		SetStyle(SCE_C_CHARACTER);
	    } else if (vars.chCur == '#' && visibleChars == 0) {
		/* Preprocessor commands are alone on their line */
		SetStyle(SCE_C_PREPROCESSOR);
		/* Skip whitespace between # and preprocessor word */
		do {
		    Forward();
		} while (IsASpaceOrTab(vars.chCur) && More());
		if (vars.atLineEnd) {
		    SetStyle(SCE_C_DEFAULT);
		}
	    } else if (isoperator(vars.chCur)) {
		SetStyle(SCE_C_OPERATOR);
	    }
	}
	
	if (!IsASpace(vars.chCur)) {
	    chPrevNonWhite = vars.chCur;
	    visibleChars++;
	}
    }
    Complete();

    return 0;
}

static bool IsStreamCommentStyle(int style) {
    return style == SCE_C_COMMENT ||
	style == SCE_C_COMMENTDOC ||
	style == SCE_C_COMMENTDOCKEYWORD ||
	style == SCE_C_COMMENTDOCKEYWORDERROR;
}

static int FoldTOL(LexerArgs *args)
{
    struct Lexer2 *lexer = (struct Lexer2 *) args->lexer;
    bool foldComment = lexer->foldComment;
    bool foldPreprocessor = lexer->foldPreprocessor;
    bool foldCompact = lexer->foldCompact;
    bool foldAtElse = lexer->foldAtElse;

    int levelCurrent, levelMinCurrent, levelNext;
    int visibleChars = 0;

    BeginStyling(args, args->firstLine, args->lastLine);

    /* Store both the current line's fold level and the next line's in the
     * level store to make it easy to pick up with each increment
     * and to make it possible to fiddle the current level for "} else {". */
    levelCurrent = vars.linePrevPtr ?
	vars.linePrevPtr->level >> 16 : SC_FOLDLEVELBASE;
    levelMinCurrent = levelCurrent;
    levelNext = levelCurrent;

    for ( ; More(); Forward() ) {
	if (foldComment && IsStreamCommentStyle(vars.style)) {
	    if (!IsStreamCommentStyle(vars.stylePrev)) {
		levelNext++;
	    } else if (!IsStreamCommentStyle(vars.styleNext) && !vars.atLineEnd) {
		/* Comments don't end at end of line and the next character may be unstyled. */
		levelNext--;
	    }
	}
	if (foldComment && (vars.style == SCE_C_COMMENTLINE)) {
	    if (MatchChCh('/', '/')) {
		char chNext2 = SafeGetCharAt(vars.charIndex + 2);
		if (chNext2 == '{') {
		    levelNext++;
		} else if (chNext2 == '}') {
		    levelNext--;
		}
	    }
	}
	if (foldPreprocessor && (vars.style == SCE_C_PREPROCESSOR)) {
	    if (MatchCh('#')) {
		unsigned int j = vars.charIndex + 1;
		while ((j < vars.lineLength) && IsASpaceOrTab(SafeGetCharAt(j))) {
		    j++;
		}
		if (MatchStrAt(j, "region") || MatchStrAt(j, "if")) {
		    levelNext++;
		} else if (MatchStrAt(j, "end")) {
		    levelNext--;
		}
	    }
	}
	if (vars.style == SCE_C_OPERATOR) {
	    if (MatchCh('{')) {
		/* Measure the minimum before a '{' to allow
		 * folding on "} else {" */
		if (levelMinCurrent > levelNext) {
		    levelMinCurrent = levelNext;
		}
		levelNext++;
	    } else if (MatchCh('}')) {
		levelNext--;
	    }
	}
	if (vars.atLineEnd) {
	    int levelUse = foldAtElse ? levelMinCurrent : levelCurrent;
	    int lev = /* levelUse | */ (levelNext << 16);
	    if (visibleChars == 0 && foldCompact)
		lev |= SC_FOLDLEVELWHITEFLAG;
	    if (levelUse < levelNext)
		lev |= SC_FOLDLEVELHEADERFLAG;
	    SetLineFoldLevel(vars.sharedPtr, vars.linePtr, levelUse, lev);
	    levelCurrent = levelNext;
	    levelMinCurrent = levelCurrent;
	    visibleChars = 0;
	}
	if (!IsASpace(vars.chCur))
	    visibleChars++;
    }

    return 0;
}

LexerModule lmTOL = {
    "tol",
    sizeof(struct Lexer2),
    styleNames,
    ColorizeTOL,
    FoldTOL,
    optionSpecs
};

