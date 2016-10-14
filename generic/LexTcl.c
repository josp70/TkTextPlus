/*
 * LexTcl.c --
 *
 * Lexer for the Tcl scripting language.
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

#define SCE_TCL_DEFAULT 0
#define SCE_TCL_WHITESPACE 1
#define SCE_TCL_WORD1 2
#define SCE_TCL_WORD2 3
#define SCE_TCL_WORD3 4
#define SCE_TCL_WORD4 5
#define SCE_TCL_WORD5 6
#define SCE_TCL_WORD6 7
#define SCE_TCL_WORD7 8
#define SCE_TCL_WORD8 9
#define SCE_TCL_WORD9 10
#define SCE_TCL_COMMENT 11
#define SCE_TCL_COMMENTLINE 12
#define SCE_TCL_NUMBER 13
#define SCE_TCL_WORD_IN_QUOTE 14
#define SCE_TCL_IN_QUOTE 15
#define SCE_TCL_OPERATOR 16
#define SCE_TCL_IDENTIFIER 17
#define SCE_TCL_SUBSTITUTION 18
#define SCE_TCL_SUB_BRACE 19
#define SCE_TCL_MODIFIER 20
#define SCE_TCL_EXPAND 21
#define SCE_TCL_COMMENT_BOX 22
#define SCE_TCL_BLOCK_COMMENT 23

static CONST char *styleNames[] = {
    "default", "whitespace", "keyword1", "keyword2", "keyword3", "keyword4",
    "keyword5", "keyword6", "keyword7", "keyword8", "keyword9", "comment",
    "commentline", "number", "word_in_quote", "string", "operator",
    "identifier", "substitution", "sub_brace", "modifier", "expand",
    "comment_box", "comment_block", NULL
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

static bool IsAWordChar(const int ch) {
    return (ch >= 0x80) || (isalnum(ch) || ch == '_' || ch ==':' || ch=='.');
}

static bool IsAWordStart(const int ch) {
    return (ch >= 0x80) || (isalpha(ch) || ch == '_');
}

static bool IsADigitBase(unsigned int ch, unsigned int base) {
    if (base <= 10) {
	return (ch >= '0') && (ch < '0' + base);
    } else {
	return ((ch >= '0') && (ch <= '9')) ||
	       ((ch >= 'A') && (ch < 'A' + base - 10)) ||
	       ((ch >= 'a') && (ch < 'a' + base - 10));
    }
}

static bool IsANumberChar(int ch) {
    /* Not exactly following number definition (several dots are seen as OK, etc.)
     * but probably enough in most cases. */
    return (ch < 0x80) &&
           (IsADigitBase(ch, 0x10) || toupper(ch) == 'E' ||
            ch == '.' || ch == '-' || ch == '+');
}

#define isComment(s) (s==SCE_TCL_COMMENT || s==SCE_TCL_COMMENTLINE || s==SCE_TCL_COMMENT_BOX || s==SCE_TCL_BLOCK_COMMENT)

#define StyleIs(s) \
    (vars.style == (s))

enum tLineState {
    LS_DEFAULT = 0,
    LS_OPEN_COMMENT = 0x1,
    LS_OPEN_DOUBLE_QUOTE = 0x2,
    LS_COMMENT_BOX = 0x4,
    LS_VARNAME_IN_BRACES = 0x8,
    LS_MASK_STATE = 0xf,
    LS_ONLY_COMMENT = 0x20,	/* Nothing but whitespace + comment */
    LS_BLANK = 0x40,		/* Nothing but ' ' and '\t', or empty */
    LS_CONTINUATION = 0x80	/* Line ends with a '\' */
};

static int ColorizeTcl(LexerArgs *args)
{
    int lineState = LS_DEFAULT;
    int currentLine;
    bool cmdExpected = false;
    bool subParen = false;
    int leadingWS = 0;
    bool continuation = false;
    int braceDepthInComment = 0;

    /* Back up 1 line then begin styling. */
    currentLine = args->firstLine;
    if (args->firstLine > 0)
	currentLine--;
    BeginStyling(args, currentLine, args->lastLine);

    if (vars.linePrevPtr != NULL) {
	lineState = GetLineState(NULL, vars.linePrevPtr) & LS_MASK_STATE;
	continuation = (lineState & LS_CONTINUATION) != 0;
    }

    for ( ; More() ; Forward() ) {

	if (vars.atLineStart) {
	    /* We can expect the first word on this line to be a command
	     * if the previous line did not end in an unclosed string,
	     * continuation line, or variable name in braces. */
	    cmdExpected = false;

	    if (lineState == LS_OPEN_COMMENT) {
		SetStyle(SCE_TCL_COMMENTLINE);
	    } else if (lineState == LS_OPEN_DOUBLE_QUOTE) {
		SetStyle(SCE_TCL_IN_QUOTE);
	    } else if (lineState == LS_VARNAME_IN_BRACES) {
		SetStyle(SCE_TCL_SUB_BRACE);
	    } else if (lineState == LS_COMMENT_BOX &&
		    (MatchCh('#') || MatchChCh(' ', '#'))) {
		SetStyle(SCE_TCL_COMMENT_BOX);
	    } else {
		if (IsASpaceOrTab(vars.chCur)) {
		    SetStyle(SCE_TCL_WHITESPACE);
		} else {
		    SetStyle(SCE_TCL_DEFAULT);
		}
		cmdExpected = !continuation &&
		    (IsAWordStart(vars.chCur) ||
		    IsASpaceOrTab(vars.chCur));
	    }

	    /* Skip past any the leading whitespace. */
	    while (!vars.atLineEnd && IsASpaceOrTab(vars.chCur)) {
		Forward();
	    }
	    leadingWS = vars.charIndex;

	    braceDepthInComment = 0;
	}

	/* Determine if the current style should terminate. */
	if (StyleIs(SCE_TCL_WHITESPACE)) {
	    if (!IsASpaceOrTab(vars.chCur)) {
		SetStyle(SCE_TCL_DEFAULT);
	    }
	} else if (StyleIs(SCE_TCL_SUB_BRACE)) {
	    /* ${ overrides everything even \ except } */
	    if (MatchCh('}')) {
		StyleAhead(1, SCE_TCL_OPERATOR);
		SetStyle(SCE_TCL_DEFAULT);
	    } else {
		SetStyle(SCE_TCL_SUB_BRACE);
	    }
	    if (!vars.atLineEnd)
		continue;
	} else if (StyleIs(SCE_TCL_NUMBER)) {
	    if (!IsANumberChar(vars.chCur)) {
		SetStyle(SCE_TCL_DEFAULT);
	    }
	} else if (StyleIs(SCE_TCL_IN_QUOTE)) {
	    if (!IsEscaped(0)) {
		if (MatchCh('\"')) {
		    ForwardSetStyle(SCE_TCL_DEFAULT);
		} else if (MatchCh('[') || MatchCh(']') || MatchCh('$')) {
		    cmdExpected = MatchCh('[');
		    StyleAhead(1, SCE_TCL_OPERATOR);
		    SetStyle(SCE_TCL_IN_QUOTE);
		    continue;
		}
	    }
	} else if (StyleIs(SCE_TCL_OPERATOR)) {
	    SetStyle(SCE_TCL_DEFAULT);
#if 0
	} else if (StyleIs(SCE_TCL_DEFAULT) || StyleIs(SCE_TCL_OPERATOR)) {
	    if (cmdExpected) {
		cmdExpected = isspacechar(vars.chCur) ||
		    IsAWordStart(vars.chCur) ||
		    MatchCh('#');
	    }
#endif
	} else if (StyleIs(SCE_TCL_SUBSTITUTION)) {
	    switch (vars.chCur) {
		case '(':
		    subParen = true;
		    StyleAhead(1, SCE_TCL_OPERATOR);
		    SetStyle(SCE_TCL_SUBSTITUTION);
		    continue;
		case ')':
		    SetStyle(SCE_TCL_OPERATOR);
		    subParen = false;
		    continue;
		case '$':
		    continue;
		case ',':
		    if (subParen) {
			StyleAhead(1, SCE_TCL_OPERATOR);
			SetStyle(SCE_TCL_SUBSTITUTION);
		    } else {
			SetStyle(SCE_TCL_OPERATOR);
		    }
		    continue;
		default :
		    /* maybe spaces should be allowed ??? */
		    if (!IsAWordChar(vars.chCur)) { /* probably the code is wrong */
			SetStyle(SCE_TCL_DEFAULT);
			subParen = false;
		    }
		    break;
	    }
	} else if (isComment(vars.style)) {
	    /* To be done right we would need to keep a running brace-depth
	     * counter from the start of the file. For now we just try
	     * to match braces within a single line. */
	    if (MatchCh('{') && !IsEscaped(0))
		++braceDepthInComment;
	    else if (MatchCh('}') && !IsEscaped(0))
		--braceDepthInComment;
	    if (vars.chCur == '}' && braceDepthInComment < 0) {
		SetStyle(SCE_TCL_OPERATOR);
		ForwardSetStyle(SCE_TCL_DEFAULT);
	    }
	} else if (!IsAWordChar(vars.chCur)) {
	    if ((StyleIs(SCE_TCL_IDENTIFIER) && cmdExpected) || StyleIs(SCE_TCL_MODIFIER)) {
		char w[100];
		char *s = w;
		bool quote;
		int len = GetCurrent(w, sizeof(w));
		if (w[len - 1] == '\r')
		    w[len - 1] = 0;
		while (*s == ':') /* ignore leading : like in ::set a 10 */
		    ++s, --len;
		quote = StyleIs(SCE_TCL_IN_QUOTE); /* FIXME: impossible due to the if() */
		if (cmdExpected) {
		    int kw;
		    if (MatchKeyword(s, len, &kw)) {
			ChangeState(quote ? SCE_TCL_WORD_IN_QUOTE : (SCE_TCL_WORD1 + kw));
		    } 
		}
		cmdExpected = false;
		SetStyle(quote ? SCE_TCL_IN_QUOTE : SCE_TCL_DEFAULT);
	    } else if (StyleIs(SCE_TCL_MODIFIER) || StyleIs(SCE_TCL_IDENTIFIER)) {
		SetStyle(SCE_TCL_DEFAULT);
	    }
	}

	if (vars.atLineEnd) {
	    int flags = 0;

	    lineState = LS_DEFAULT;

	    continuation = (!vars.atLineStart && vars.chPrev == '\\' &&
		!IsEscaped(-1));
	    if (continuation)
		flags |= LS_CONTINUATION;

	    /* Update the line state, so it can be seen by next line */
	    if (StyleIs(SCE_TCL_IN_QUOTE)) {
		lineState = LS_OPEN_DOUBLE_QUOTE;
	    } else if (StyleIs(SCE_TCL_SUB_BRACE)) {
		lineState = LS_VARNAME_IN_BRACES;
	    } else {
		if (continuation) {
		    if (isComment(vars.style))
			lineState = LS_OPEN_COMMENT;
		} else if (StyleIs(SCE_TCL_COMMENT_BOX))
		    lineState = LS_COMMENT_BOX;
	    }

	    /* Remember whether this line is nothing but a comment. This
	     * information is used by the folding function. */
	    if (!StyleIs(SCE_TCL_COMMENT) && isComment(vars.style))
		flags |= LS_ONLY_COMMENT;

	    if (leadingWS == vars.charIndex)
		flags |= LS_BLANK;

	    SetLineState(NULL, vars.linePtr, flags | lineState);

	    if ((lineState != LS_COMMENT_BOX) &&
		    (lineState != LS_OPEN_DOUBLE_QUOTE) &&
		    (lineState != LS_VARNAME_IN_BRACES)) {
		SetStyle(SCE_TCL_DEFAULT); /* Don't give newline this style. */
	    }
	    continue;
	}

	/* Determine if a new style should be begin. */
	if (StyleIs(SCE_TCL_DEFAULT)) {
	    if (IsAWordStart(vars.chCur)) {
		SetStyle(SCE_TCL_IDENTIFIER);
		continue;
	    }
	    if (IsADigit(vars.chCur) && !IsAWordChar(vars.chPrev)) {
		SetStyle(SCE_TCL_NUMBER);
		if (MatchChCh('0', 'x') && IsADigitBase(GetRelative(2),
			0x10)) {
		    Forward();
		}
		continue;
	    }

	    switch (vars.chCur) {
		case ' ':
		case '\t':
		    SetStyle(SCE_TCL_WHITESPACE);
		    break;
		case '\\':
		    /* FIXME: \uHEX is unicode character */
		    SetStyle(SCE_TCL_OPERATOR);
		    break;
		case '\"':
		    SetStyle(SCE_TCL_IN_QUOTE);
		    break;
		case '{':
		    if (IsEscaped(0)) {
			SetStyle(SCE_TCL_IDENTIFIER);
			break;
		    }
		    /* Look for the {*} expansion thingy. */
		    if (MatchStr("{*}")) {
			char chNext3 = GetRelative(3);
			if (chNext3 == '{' ||
				chNext3 == '$' ||
				chNext3 == '[') {
			    StyleAhead(3, SCE_TCL_EXPAND);
			    SetStyle(SCE_TCL_DEFAULT);
			    break;
			}
		    }
		    SetStyle(SCE_TCL_OPERATOR);
		    cmdExpected = true;
		    break;
		case '}':
		    if (IsEscaped(0)) {
			SetStyle(SCE_TCL_IDENTIFIER);
			break;
		    }
		    SetStyle(SCE_TCL_OPERATOR);
		    break;
		case '[':
		    cmdExpected = true;
		case ']':
		case '(':
		case ')':
		    /* FIXME: what if these are \[ escaped? */
		    SetStyle(SCE_TCL_OPERATOR);
		    break;
		case ';':
		    SetStyle(SCE_TCL_OPERATOR);
		    cmdExpected = true;
		    break;
		case '$':
		    subParen = false;
		    if (vars.chNext != '{') {
			SetStyle(SCE_TCL_SUBSTITUTION);
		    } else {
			StyleAhead(2, SCE_TCL_OPERATOR);  /* ${ */
			SetStyle(SCE_TCL_SUB_BRACE);
		    }
		    break;
		case '#':
		    if ((leadingWS < vars.charIndex) && cmdExpected) {
			/* A comment following other text. */
			SetStyle(SCE_TCL_COMMENT);
			break;
		    }
		    if (leadingWS == vars.charIndex) {
			/* I don't like these special comment types. */
			if (vars.chNext == '~') {
			    SetStyle(SCE_TCL_BLOCK_COMMENT);
			} else if (vars.atLineStart &&
				(vars.chNext == '#' ||
				vars.chNext == '-')) {
			    SetStyle(SCE_TCL_COMMENT_BOX);
			} else {
			    SetStyle(SCE_TCL_COMMENTLINE);
			}
			break;
		    }
		    if ((IsASpaceOrTab(vars.chPrev) ||
			    (vars.chPrev == '\\') ||
			    isoperator(vars.chPrev)) &&
			    IsADigitBase(vars.chNext, 0x10)) {
			SetStyle(SCE_TCL_NUMBER);
			break;
		    }
		    /* foo#bar or something */
		    SetStyle(SCE_TCL_IDENTIFIER);
		    break;
		case '-':
		    /* "test text-20.59 {..." */
		    if (vars.stylePrev != SCE_TCL_DEFAULT) {
			SetStyle(SCE_TCL_IDENTIFIER);
			break;
		    }
		    if (IsADigit(vars.chNext)) {
			SetStyle(SCE_TCL_NUMBER);
		    } else {
			/* These modifiers are -switches ??? */
			SetStyle(SCE_TCL_MODIFIER);
		    }
		    break;
		default:
		    if (isoperator(vars.chCur)) {
			SetStyle(SCE_TCL_OPERATOR);
		    }
		    break;
	    }
	}
    }
    Complete();
    return 0;
}

static int FoldTcl(LexerArgs *args)
{
    struct Lexer2 *lexer = (struct Lexer2 *) args->lexer;
    bool foldComment = lexer->foldComment;
    int foldCommentMinLevel = 100;
    int blankLineEndsComment = true;
    bool insideComment = false;
    int currentLine;
    int currentLevel = 0;
    int previousLevel;
    bool visibleChars;

    currentLine = args->firstLine;
    if (args->firstLine > 0)
	currentLine--;
    BeginStyling(args, currentLine, args->lastLine);

    if (vars.linePrevPtr != NULL) {
	currentLevel = vars.linePrevPtr->level >> 17;
	insideComment = (vars.linePrevPtr->level & (1 << 16)) != 0;
    }

    previousLevel = currentLevel;

    for ( ; More(); Forward() ) {

	visibleChars = !(vars.linePtr->state & LS_BLANK);
	if (!visibleChars) {
	    JumpToEOL();
	}

	if (vars.atLineEnd) {
	    int flag = 0;

	    if (foldComment) {
		if (vars.linePtr->state & LS_ONLY_COMMENT) {
		    if (!insideComment &&
			    (currentLevel <= foldCommentMinLevel) &&
			    (vars.lineNextPtr != NULL) &&
			    (vars.lineNextPtr->state & LS_ONLY_COMMENT)) {
			++currentLevel;
			insideComment = true;
		    }
		} else {
		    if (insideComment &&
			(visibleChars || blankLineEndsComment)) {
			--currentLevel;
			--previousLevel;
			insideComment = false;
		    }
		}
	    }
	    if (!visibleChars)
		flag = SC_FOLDLEVELWHITEFLAG;
	    if (currentLevel > previousLevel)
		flag = SC_FOLDLEVELHEADERFLAG;

	    SetLineFoldLevel(vars.sharedPtr, vars.linePtr,
		SC_FOLDLEVELBASE + previousLevel,
		flag |
		(currentLevel << 17) |
		(insideComment ? (1 << 16) : 0));

	    previousLevel = currentLevel;
	    continue;
	}

	if (vars.style == SCE_TCL_OPERATOR) {
	    switch (vars.chCur) {
		case '{':
		    ++currentLevel;
		    break;
		case '}':
		    --currentLevel;
		    break;
	    }
	}
    }

    return 0;
}

LexerModule lmTcl = {
    "tcl",
    sizeof(struct Lexer2),
    styleNames,
    ColorizeTcl,
    FoldTcl,
    optionSpecs
};

