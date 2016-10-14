/*
 * LexMake.c --
 *
 * Lexer for Makefiles.
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

#define SCE_MAKE_DEFAULT 0
#define SCE_MAKE_WHITESPACE 1
#define SCE_MAKE_WORD1 2
#define SCE_MAKE_WORD2 3
#define SCE_MAKE_WORD3 4
#define SCE_MAKE_WORD4 5
#define SCE_MAKE_WORD5 6
#define SCE_MAKE_WORD6 7
#define SCE_MAKE_WORD7 8
#define SCE_MAKE_WORD8 9
#define SCE_MAKE_WORD9 10
#define SCE_MAKE_COMMENT 11
#define SCE_MAKE_PREPROCESSOR 12
#define SCE_MAKE_VARIABLE 13
#define SCE_MAKE_OPERATOR 14
#define SCE_MAKE_TARGET 15
#define SCE_MAKE_IDEOL 16
#define SCE_MAKE_SUBSTITUTION 17
#define SCE_MAKE_CHARACTER 18
#define SCE_MAKE_STRING 19
#define SCE_MAKE_STRINGEOL 20

static CONST char *styleNames[] = {
    "default", "whitespace", "keyword1", "keyword2", "keyword3", "keyword4",
    "keyword5", "keyword6", "keyword7", "keyword8", "keyword9", "comment",
    "preprocessor", "variable", "operator", "target", "ideol",
    "substitution", "character", "string", "stringeol", NULL
};

struct Lexer2
{
    struct Lexer lexer;		/* Required first field. */
    int foldComment;
};

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BOOLEAN, "-foldcomment", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldComment), 0, NULL, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0, 0, 0}
};

enum {
LS_CONTINUATION = 0x1,
LS_BLANK = 0x2,
LS_COMMENT = 0x4,
LS_TAB = 0x8,
LS_TARGET = 0x10,
LS_VARIABLE = 0x20
};

void ColourRange(int start, int end, int style)
{
    while (start <= end)
	vars.styleBuf[start++] = style;
}

static int ColouriseMakeDoc(LexerArgs *args)
{
    bool targetOrVar = false;
    int leadingWS = 0;

    BeginStyling(args, args->firstLine, args->lastLine);

    for (; More(); Forward()) {

	if (vars.atLineStart) {
	    bool continuation = (vars.linePrevPtr != NULL) &&
		(vars.linePrevPtr->state & LS_CONTINUATION);

	    if (vars.style == SCE_MAKE_STRINGEOL)
		vars.style = SCE_MAKE_DEFAULT;

	    /* Skip leading whitespace */
	    if (IsASpaceOrTab(vars.chCur)) {
		if (vars.style == SCE_MAKE_DEFAULT) {
		    SetStyle(SCE_MAKE_WHITESPACE);
		}
		while (!vars.atLineEnd && IsASpaceOrTab(vars.chCur)) {
		    Forward();
		}
		if (vars.style == SCE_MAKE_WHITESPACE) {
		    SetStyle(SCE_MAKE_DEFAULT);
		}
	    }
	    leadingWS = vars.charIndex;

	    targetOrVar = vars.atLineStart;
	    if (continuation) {
		targetOrVar = false;
	    } else if (MatchCh('#')) {
		SetStyle(SCE_MAKE_COMMENT);
		JumpToEOL();
	    } else if (MatchCh('!')) {
		SetStyle(SCE_MAKE_PREPROCESSOR);
		JumpToEOL();
	    }
	}

	/* Determine if the current style should end. */
	switch (vars.style) {
	    case SCE_MAKE_WHITESPACE:
		if (!IsASpaceOrTab(vars.chCur)) {
		    SetStyle(SCE_MAKE_DEFAULT);
		}
		break;
	    case SCE_MAKE_OPERATOR:
		SetStyle(SCE_MAKE_DEFAULT);
		break;
	    case SCE_MAKE_COMMENT:
		if (vars.atLineEnd) {
		    if (vars.chPrev != '\\' || IsEscaped(-1)) {
			SetStyle(SCE_MAKE_DEFAULT);
		    }
		}
		break;
	    case SCE_MAKE_SUBSTITUTION:
		if (vars.atLineEnd) {
		    /* Error: unterminated variable dereference. */
		    ChangeState(SCE_MAKE_IDEOL);
		} else if (MatchCh(')') || MatchCh('}')) {
		    ForwardSetStyle(SCE_MAKE_DEFAULT);
		}
		break;
	    case SCE_MAKE_CHARACTER:
		if (vars.atLineEnd) {
		    if (vars.chPrev != '\\' || IsEscaped(-1)) {
			SetStyle(SCE_MAKE_STRINGEOL);
		    }
		} else if (MatchCh('\'') && !IsEscaped(0)) {
		    ForwardSetStyle(SCE_MAKE_DEFAULT);
		}
		break;
	    case SCE_MAKE_STRING:
		if (vars.atLineEnd) {
		    if (vars.chPrev != '\\' || IsEscaped(-1)) {
			SetStyle(SCE_MAKE_STRINGEOL);
		    }
		} else if (MatchCh('\"') && !IsEscaped(0)) {
		    ForwardSetStyle(SCE_MAKE_DEFAULT);
		}
		break;
	}

	if (vars.atLineEnd) {
	    int state = 0;

	    if (vars.stylePrev == SCE_MAKE_COMMENT)
		state |= LS_COMMENT;

	    if (leadingWS == vars.charIndex)
		state |= LS_BLANK;

	    if (vars.charBuf[0] == '\t')
		state |= LS_TAB;

	    if (vars.styleBuf[0] == SCE_MAKE_TARGET)
		state |= LS_TARGET;

	    if (vars.styleBuf[0] == SCE_MAKE_VARIABLE)
		state |= LS_VARIABLE;

	    if (vars.chPrev == '\\' && !IsEscaped(-1)) {
		state |= LS_CONTINUATION;
#if 0
		/* Comments and strings may extend to the next line. */
		if (vars.style != SCE_MAKE_COMMENT &&
			vars.style != SCE_MAKE_STRING) {
		    SetStyle(SCE_MAKE_DEFAULT);
		}
	    } else {
		SetStyle(SCE_MAKE_DEFAULT);
#endif
	    }
	    SetLineState(NULL, vars.linePtr, state);
	    continue;
	}

	/* NAME = VALUE, NAME := VALUE, NAME ?= VALUE */
	if (targetOrVar && (MatchCh('=') || MatchChCh(':', '=') ||
		MatchChCh('?', '='))) {
	    int i = vars.charIndex - 1;
	    while (i > 0 && IsASpaceOrTab(vars.charBuf[i]))
		i--;
	    SetStyle(SCE_MAKE_OPERATOR);
	    if (vars.chCur != '=') {
		Forward();
	    }
	    ColourRange(0, i, SCE_MAKE_VARIABLE);
	    targetOrVar = false;
	    continue;
	}

	/* TARGET : */
	if (targetOrVar && MatchCh(':')) {
	    int i = vars.charIndex - 1;
	    while (i > 0 && IsASpaceOrTab(vars.charBuf[i]))
		i--;
	    SetStyle(SCE_MAKE_OPERATOR);
	    ColourRange(0, i, SCE_MAKE_TARGET);
	    targetOrVar = false;
	    continue;
	}

	/* Determine if a new style should start. */
	if (vars.style == SCE_MAKE_DEFAULT) {
	    switch (vars.chCur) {
		case ' ':
		case '\t': {
		    SetStyle(SCE_MAKE_WHITESPACE);
		    break;
		case '$':
		    if (vars.chNext == '(' || vars.chNext == '{') {
			SetStyle(SCE_MAKE_SUBSTITUTION);
		    }
		    break;
		case '\'':
		    SetStyle(SCE_MAKE_CHARACTER);
		    break;
		case '\"':
		    SetStyle(SCE_MAKE_STRING);
		    break;
		case '\\':
		    if (!IsEscaped(0) && vars.chNext == '\"') {
			StyleAhead(2, SCE_MAKE_STRING);
			SetStyle(SCE_MAKE_DEFAULT);
		    }
		    break;
		}
	    }
	}
    }

    Complete();
    return 0;
}

static int FoldMakeDoc(LexerArgs *args)
{
    struct Lexer2 *lexer = (struct Lexer2 *) args->lexer;
    bool foldComment = lexer->foldComment;
    bool foldRule = true;
    bool foldVariable = true;
    bool insideComment = false;
    bool insideRule = false;
    bool insideVariable = false;
    int currentLevel = 0;
    int previousLevel;

    /* Back up 1 line because the previous line may become foldable or
     * no-longer foldable due to editing the firstLine. */
    if (args->firstLine > 0)
	BeginStyling(args, args->firstLine - 1, args->lastLine);
    else
	BeginStyling(args, args->firstLine, args->lastLine);

    if (vars.linePrevPtr != NULL) {
	currentLevel = vars.linePrevPtr->level >> 18;
	insideRule = (vars.linePrevPtr->level & (1 << 16)) != 0;
	insideVariable = (vars.linePrevPtr->level & (1 << 17)) != 0;
	insideComment = (vars.linePrevPtr->state & LS_COMMENT) != 0;
    }

    previousLevel = currentLevel;

    for (; More(); Forward()) {

	if (vars.atLineStart) {
	    if (vars.linePtr->state & LS_COMMENT) {
		JumpToEOL();
	    } else if (vars.linePtr->state & LS_TARGET) {
		JumpToEOL();
	    }
	}

	if (vars.atLineEnd) {
	    int flag = 0;
	    if (foldComment) {
		if (vars.linePtr->state & LS_COMMENT) {
		    if (!insideComment &&
			    (vars.lineNextPtr != NULL) &&
			    (vars.lineNextPtr->state & LS_COMMENT)) {
			++currentLevel;
dbwin("line %d comment START", vars.lineIndex+1);
			insideComment = true;
		    }
else dbwin("line %d comment CONTINUE LS_COMMENT %s", vars.lineIndex+1, (vars.linePtr->state & LS_COMMENT) ? "yes" : "no");
		} else {
		    if (insideComment) {
			--currentLevel;
			--previousLevel;
			insideComment = false;
dbwin("line %d comment END LS_COMMENT %s", vars.lineIndex+1, (vars.linePtr->state & LS_COMMENT) ? "yes" : "no");
		    }
		}
	    }
	    /* Put a variable declaration plus all continuation lines into
	     * a fold. */
	    if (foldVariable) {
		if (insideVariable) {
		    if ((vars.linePrevPtr != NULL) &&
			    !(vars.linePrevPtr->state & LS_CONTINUATION)) {
			--currentLevel;
			--previousLevel;
			insideVariable = false;
		    }
		}
		if ((vars.linePtr->state & LS_VARIABLE) &&
			(vars.linePtr->state & LS_CONTINUATION)) {
		    ++currentLevel;
		    insideVariable = true;
		}
	    }
	    if (foldRule) {
		if (insideRule) {

		    if (!(vars.linePtr->state & (LS_COMMENT | LS_TAB | LS_BLANK))) {
			TkTextLine *linePtr;
			int depth;

			--currentLevel;
			--previousLevel;
			insideRule = false;

			/* The current line terminates a rule. Now walk back
			 * through all comments and blank lines, removing
			 * them from the rule. */
			for (linePtr = vars.linePrevPtr;
				linePtr != NULL;
				linePtr = BTREE_PREVLINE(vars.sharedPtr->peers, linePtr)) {
			    /* If we get all the way back to the start of a
			     * rule, then mark it as unfoldable since it is
			     * only a single line. */
			    if (linePtr->state & LS_TARGET) {
				/* Clear the depth<<18 as well. */
				int flags = linePtr->level & ~SC_FOLDLEVELNUMBERMASK;
				SetLineFoldLevel(vars.sharedPtr, linePtr,
					0, flags & ~SC_FOLDLEVELHEADERFLAG);
				break;
			    }
			    if (!(linePtr->state & (LS_COMMENT | LS_BLANK)))
				break;
			    depth = (linePtr->level >> 18) - 1;
			    SetLineFoldLevel(vars.sharedPtr, linePtr,
				    (linePtr->level & SC_FOLDLEVELNUMBERMASK) - 1,
				    (depth << 18) |
				    (linePtr->level & 0xFFFF));
			}
		    }
		}
		if (vars.linePtr->state & LS_TARGET) {
		    ++currentLevel;
		    insideRule = true;
		}
	    }
	    if (currentLevel > previousLevel)
		flag |= SC_FOLDLEVELHEADERFLAG;
	    SetLineFoldLevel(vars.sharedPtr, vars.linePtr,
		    SC_FOLDLEVELBASE + previousLevel,
		    flag |
		    (currentLevel << 18) |
		    (insideRule ? (1 << 16) : 0) |
		    (insideVariable ? (1 << 17) : 0));

	    previousLevel = currentLevel;
	    continue;
	}
    }
    return 0;
}

LexerModule lmMake = {
    "makefile",
    sizeof(struct Lexer2),
    styleNames,
    ColouriseMakeDoc,
    FoldMakeDoc,
    optionSpecs
};

