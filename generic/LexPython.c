/*
 * LexPython.c --
 *
 * Lexer for the Python scripting language.
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

#define SCE_P_DEFAULT 0
#define SCE_P_WHITESPACE 1
#define SCE_P_WORD1 2
#define SCE_P_WORD2 3
#define SCE_P_WORD3 4
#define SCE_P_WORD4 5
#define SCE_P_WORD5 6
#define SCE_P_WORD6 7
#define SCE_P_WORD7 8
#define SCE_P_WORD8 9
#define SCE_P_WORD9 10
#define SCE_P_COMMENTLINE 11
#define SCE_P_NUMBER 12
#define SCE_P_STRING 13
#define SCE_P_CHARACTER 14
#define SCE_P_TRIPLE 15
#define SCE_P_TRIPLEDOUBLE 16
#define SCE_P_CLASSNAME 17
#define SCE_P_DEFNAME 18
#define SCE_P_OPERATOR 19
#define SCE_P_IDENTIFIER 20
#define SCE_P_COMMENTBLOCK 21
#define SCE_P_STRINGEOL 22
#define SCE_P_DECORATOR 23

static CONST char *styleNames[] = {
    "default", "whitespace", "keyword1", "keyword2", "keyword3", "keyword4",
    "keyword5", "keyword6", "keyword7", "keyword8", "keyword9", "commentline",
    "number", "string", "character", "triple", "tripledouble", "classname",
    "defname", "operator", "identifier", "commentblock", "stringeol",
    "decorator", NULL
};

struct Lexer2
{
    struct Lexer lexer;		/* Required first field. */
    int foldComment;
    int foldQuote;
    int whingeLevel;
};

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BOOLEAN, "-foldcomment", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldComment), 0, NULL, 0},
    {TK_OPTION_BOOLEAN, "-foldquote", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldQuote), 0, NULL, 0},
    {TK_OPTION_INT, "-whingelevel", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, whingeLevel), 0, NULL, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0, 0, 0}
};

enum kwType { kwOther, kwClass, kwDef, kwImport };

static bool IsPyComment(char *s, int pos, int len) {
    return len > 0 && s[pos] == '#';
}

static bool IsPyStringStart(int ch, int chNext, int chNext2) {
    if (ch == '\'' || ch == '"')
	return true;
    if (ch == 'u' || ch == 'U') {
	if (chNext == '"' || chNext == '\'')
	    return true;
	if ((chNext == 'r' || chNext == 'R') && (chNext2 == '"' || chNext2 == '\''))
	    return true;
    }
    if ((ch == 'r' || ch == 'R') && (chNext == '"' || chNext == '\''))
	return true;

    return false;
}

/* Return the state to use for the string starting at i; *nextIndex will be set to the first index following the quote(s) */
static int GetPyStringState(int i, unsigned int *nextIndex) {
    char ch = SafeGetCharAt(i);
    char chNext = SafeGetCharAt(i + 1);

    /* Advance beyond r, u, or ur prefix, but bail if there are any unexpected chars */
    if (ch == 'r' || ch == 'R') {
	i++;
	ch = SafeGetCharAt(i);
	chNext = SafeGetCharAt(i + 1);
    } else if (ch == 'u' || ch == 'U') {
	if (chNext == 'r' || chNext == 'R')
	    i += 2;
	else
	    i += 1;
	ch = SafeGetCharAt(i);
	chNext = SafeGetCharAt(i + 1);
    }

    if (ch != '"' && ch != '\'') {
	*nextIndex = i + 1;
	return SCE_P_DEFAULT;
    }

    if (ch == chNext && ch == SafeGetCharAt(i + 2)) {
	*nextIndex = i + 3;

	if (ch == '"')
	    return SCE_P_TRIPLEDOUBLE;
	else
	    return SCE_P_TRIPLE;
    } else {
	*nextIndex = i + 1;

	if (ch == '"')
	    return SCE_P_STRING;
	else
	    return SCE_P_CHARACTER;
    }
}

static bool IsAWordChar(int ch) {
    return (ch < 0x80) && (isalnum(ch) || ch == '.' || ch == '_');
}

static bool IsAWordStart(int ch) {
    return (ch < 0x80) && (isalnum(ch) || ch == '_');
}

static int ColourisePyDoc(LexerArgs *args)
{
    struct Lexer2 *lexer = (struct Lexer2 *) args->lexer;
    const int whingeLevel = lexer->whingeLevel;
    int initStyle;
    int spaceFlags = 0;
    bool hexadecimal = false;
    enum kwType kwLast = kwOther;

    /* Backtrack to previous line in case need to fix its tab whinging */
    if (args->firstLine > 0)
	BeginStyling(args, args->firstLine - 1, args->lastLine);
    else
	BeginStyling(args, args->firstLine, args->lastLine);
    initStyle = vars.style;

    initStyle = initStyle & 31;
    if (initStyle == SCE_P_STRINGEOL) {
	initStyle = SCE_P_DEFAULT;
    }

    if (!vars.atLineStart)
	IndentAmount(vars.linePtr, vars.linePrevPtr, &spaceFlags, IsPyComment);

#if 0
    /* Python uses a different mask because bad indentation is marked by oring with 32 */
    StyleContext sc(startPos, endPos - startPos, initStyle, styler, 0x7f);
#endif

    for (; More(); Forward()) {

	if (vars.atLineStart) {
	    if (vars.style == SCE_P_STRINGEOL) {
		SetStyle(SCE_P_DEFAULT);
	    }
	}
#if 0
	if (sc.atLineStart) {
	    const char chBad = static_cast<char>(64);
	    const char chGood = static_cast<char>(0);
	    char chFlags = chGood;

	    IndentAmount(vars.linePtr, vars.linePrevPtr, &spaceFlags, IsPyComment);
	    if (whingeLevel == 1) {
		chFlags = (spaceFlags & wsInconsistent) ? chBad : chGood;
	    } else if (whingeLevel == 2) {
		chFlags = (spaceFlags & wsSpaceTab) ? chBad : chGood;
	    } else if (whingeLevel == 3) {
		chFlags = (spaceFlags & wsSpace) ? chBad : chGood;
	    } else if (whingeLevel == 4) {
		chFlags = (spaceFlags & wsTab) ? chBad : chGood;
	    }
	    sc.SetStyle(sc.state);
	    styler.SetFlags(chFlags, static_cast<char>(sc.state));
	}
#endif

	/* Determine if the current style should terminate. */
	if (vars.style == SCE_P_WHITESPACE) {
	    if (!IsASpaceOrTab(vars.chCur)) {
		SetStyle(SCE_P_DEFAULT);
	    }
	} else if (vars.style == SCE_P_OPERATOR) {
	    kwLast = kwOther;
	    SetStyle(SCE_P_DEFAULT);
	} else if (vars.style == SCE_P_NUMBER) {
	    if (!IsAWordChar(vars.chCur) &&
	            !(!hexadecimal && ((vars.chCur == '+' || vars.chCur == '-') && (vars.chPrev == 'e' || vars.chPrev == 'E')))) {
		SetStyle(SCE_P_DEFAULT);
	    }
	} else if (vars.style == SCE_P_IDENTIFIER) {
	    if ((vars.chCur == '.') || (!IsAWordChar(vars.chCur))) {
		char s[100];
		int len = GetCurrent(s, sizeof(s));
		int kw, style = SCE_P_IDENTIFIER;
		if ((kwLast == kwImport) && (strcmp(s, "as") == 0)) {
		    style = SCE_P_WORD1;
		} else if (MatchKeyword(s, len, &kw)) {
		    style = SCE_P_WORD1 + kw;
		} else if (kwLast == kwClass) {
		    style = SCE_P_CLASSNAME;
		} else if (kwLast == kwDef) {
		    style = SCE_P_DEFNAME;
		}
		ChangeState(style);
		SetStyle(SCE_P_DEFAULT);
		if (style == SCE_P_WORD1) {
		    if (0 == strcmp(s, "class"))
			kwLast = kwClass;
		    else if (0 == strcmp(s, "def"))
			kwLast = kwDef;
		    else if (0 == strcmp(s, "import"))
			kwLast = kwImport;
		    else
			kwLast = kwOther;
		} else {
		    kwLast = kwOther;
		}
	    }
	} else if ((vars.style == SCE_P_COMMENTLINE) || (vars.style == SCE_P_COMMENTBLOCK)) {
	    if (vars.chCur == '\r' || vars.chCur == '\n') {
		SetStyle(SCE_P_DEFAULT);
	    }
	} else if (vars.style == SCE_P_DECORATOR) {
	    if (vars.chCur == '\r' || vars.chCur == '\n') {
		SetStyle(SCE_P_DEFAULT);
	    } else if (vars.chCur == '#') {
		SetStyle((vars.chNext == '#') ? SCE_P_COMMENTBLOCK  :  SCE_P_COMMENTLINE);
	    }
	} else if ((vars.style == SCE_P_STRING) || (vars.style == SCE_P_CHARACTER)) {
	    if (vars.chCur == '\\') {
		if ((vars.chNext == '\r') && (GetRelative(2) == '\n')) {
		    Forward();
		}
		Forward();
	    } else if ((vars.style == SCE_P_STRING) && (vars.chCur == '\"')) {
		ForwardSetStyle(SCE_P_DEFAULT);
	    } else if ((vars.style == SCE_P_CHARACTER) && (vars.chCur == '\'')) {
		ForwardSetStyle(SCE_P_DEFAULT);
	    }
	} else if (vars.style == SCE_P_TRIPLE) {
	    if (vars.chCur == '\\') {
		Forward();
	    } else if (MatchStr("\'\'\'")) {
		Forward();
		Forward();
		ForwardSetStyle(SCE_P_DEFAULT);
	    }
	} else if (vars.style == SCE_P_TRIPLEDOUBLE) {
	    if (vars.chCur == '\\') {
		Forward();
	    } else if (MatchStr("\"\"\"")) {
		Forward();
		Forward();
		ForwardSetStyle(SCE_P_DEFAULT);
	    }
	}

	if (vars.atLineEnd) {
	    if ((vars.style == SCE_P_DEFAULT) ||
	            (vars.style == SCE_P_TRIPLE) ||
	            (vars.style == SCE_P_TRIPLEDOUBLE)) {
		/* Perform colourisation of white space and triple quoted strings at end of each line to allow
		 * tab marking to work inside white space and triple quoted strings */
		SetStyle(vars.style);
	    }

	    if ((vars.style == SCE_P_STRING) || (vars.style == SCE_P_CHARACTER)) {
		ChangeState(SCE_P_STRINGEOL);
	    }
	    continue;
	}

	/* Determine if a new style should be begin. */
	if (vars.style == SCE_P_DEFAULT) {
	    if (IsASpaceOrTab(vars.chCur)) {
		SetStyle(SCE_P_WHITESPACE);
	    } else if (IsADigit(vars.chCur) || (vars.chCur == '.' && IsADigit(vars.chNext))) {
		if (vars.chCur == '0' && (vars.chNext == 'x' || vars.chNext == 'X')) {
		    hexadecimal = true;
		} else {
		    hexadecimal = false;
		}
		SetStyle(SCE_P_NUMBER);
	    } else if (isascii(vars.chCur) && isoperator(vars.chCur) || vars.chCur == '`') {
		SetStyle(SCE_P_OPERATOR);
	    } else if (vars.chCur == '#') {
		SetStyle(vars.chNext == '#' ? SCE_P_COMMENTBLOCK : SCE_P_COMMENTLINE);
	    } else if (vars.chCur == '@') {
		SetStyle(SCE_P_DECORATOR);
	    } else if (IsPyStringStart(vars.chCur, vars.chNext, GetRelative(2))) {
		unsigned int nextIndex = 0;
		SetStyle(GetPyStringState(vars.charIndex, &nextIndex));
		while (nextIndex > (vars.charIndex + 1) && More()) {
		    Forward();
		}
	    } else if (IsAWordStart(vars.chCur)) {
		SetStyle(SCE_P_IDENTIFIER);
	    }
	}
    }
    Complete();
    return 0;
}

#define LINEPTR(n) BTREE_FINDLINE(args->sharedPtr->peers, n)

static bool IsCommentLine(LexerArgs *args, int line) {
    return (GetFirstNonWSChar(LINEPTR(line)) == '#');
}

static bool IsQuoteLine(LexerArgs *args, int line) {
    int style = FindStyleAtSOL(args->sharedPtr, line) & 31;
    return ((style == SCE_P_TRIPLE) || (style == SCE_P_TRIPLEDOUBLE));
}

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define KEEP_GOING_WHEN_LINE_CHANGES

static int FoldPyDoc(LexerArgs *args)
{
    struct Lexer2 *lexer = (struct Lexer2 *) args->lexer;
    const bool foldComment = lexer->foldComment;
    const bool foldQuotes = lexer->foldQuote;
    const int maxLines = args->lastLine;             /* Requested last line */
    const int docLines = args->linesInDocument - 1;  /* Available last line */
#ifdef KEEP_GOING_WHEN_LINE_CHANGES
    int levelBeforeChange;
    bool aLineChanged = false;
#endif
    int spaceFlags = 0;
    int lineCurrent = args->firstLine;
    int indentCurrent = IndentAmount(LINEPTR(lineCurrent), LINEPTR(lineCurrent-1), &spaceFlags, NULL);
    int indentCurrentLevel;
    int prev_state, prevQuote, prevComment;

    /* Backtrack to previous non-blank line so we can determine indent level
     * for any white space lines (needed esp. within triple quoted strings)
     * and so we can fix any preceding fold level (which is why we go back
     * at least one line in all cases) */
    while (lineCurrent > 0) {
	lineCurrent--;
	indentCurrent = IndentAmount(LINEPTR(lineCurrent), LINEPTR(lineCurrent-1), &spaceFlags, NULL);
	if (!(indentCurrent & SC_FOLDLEVELWHITEFLAG) &&
	        (!IsCommentLine(args, lineCurrent)) &&
	        (!IsQuoteLine(args, lineCurrent)))
	    break;
    }
    indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;

    /* Set up initial loop state */
    prev_state = SCE_P_DEFAULT & 31;
    if (lineCurrent >= 1)
	prev_state = FindStyleAtEOL(args->sharedPtr, lineCurrent - 1) & 31;
    prevQuote = foldQuotes && ((prev_state == SCE_P_TRIPLE) || (prev_state == SCE_P_TRIPLEDOUBLE));
    prevComment = 0;
    if (lineCurrent >= 1)
	prevComment = foldComment && IsCommentLine(args, lineCurrent - 1);

    /* Process all characters to end of requested range or end of any triple quote
     * or comment that hangs over the end of the range.  Cap processing in all cases
     * to end of document (in case of unclosed quote or comment at end). */
    while ((lineCurrent <= docLines) && ((lineCurrent <= maxLines)
	    || prevQuote || prevComment
#ifdef KEEP_GOING_WHEN_LINE_CHANGES
	    || aLineChanged
#endif
	    )) {

	/* Gather info */
	int lev = indentCurrent;
	int lineNext = lineCurrent + 1;
	int indentNext = indentCurrent;
	int quote = false, quote_start, quote_continue;
	int comment, comment_start, comment_continue;
	int levelAfterComments, levelBeforeComments;
	int skipLine, skipLevel;
	if (lineNext <= docLines) {
	    int style;
	    /* Information about next line is only available if not at end of document */
	    indentNext = IndentAmount(LINEPTR(lineNext), LINEPTR(lineNext - 1), &spaceFlags, NULL);
	    style = FindStyleAtSOL(args->sharedPtr, lineNext) & 31;
	    quote = foldQuotes && ((style == SCE_P_TRIPLE) || (style == SCE_P_TRIPLEDOUBLE));
	}
	quote_start = (quote && !prevQuote);
	quote_continue = (quote && prevQuote);
	comment = foldComment && IsCommentLine(args, lineCurrent);
	comment_start = (comment && !prevComment && (lineNext <= docLines) &&
		IsCommentLine(args, lineNext) && (lev > SC_FOLDLEVELBASE));
	comment_continue = (comment && prevComment);
	if ((!quote || !prevQuote) && !comment)
	    indentCurrentLevel = indentCurrent & SC_FOLDLEVELNUMBERMASK;
	if (quote)
	    indentNext = indentCurrentLevel;
	if (indentNext & SC_FOLDLEVELWHITEFLAG)
	    indentNext = SC_FOLDLEVELWHITEFLAG | indentCurrentLevel;

	if (quote_start) {
	    /* Place fold point at start of triple quoted string */
	    lev |= SC_FOLDLEVELHEADERFLAG;
	} else if (quote_continue || prevQuote) {
	    /* Add level to rest of lines in the string */
	    lev = lev + 1;
	} else if (comment_start) {
	    /* Place fold point at start of a block of comments */
	    lev |= SC_FOLDLEVELHEADERFLAG;
	} else if (comment_continue) {
	    /* Add level to rest of lines in the block */
	    lev = lev + 1;
	}

	/* Skip past any blank lines for next indent level info; we skip also
	 * comments (all comments, not just those starting in column 0)
	 * which effectively folds them into surrounding code rather
	 * than screwing up folding. */

	while (!quote &&
	        (lineNext < docLines) &&
	        ((indentNext & SC_FOLDLEVELWHITEFLAG) ||
	         (lineNext <= docLines && IsCommentLine(args, lineNext)))) {

	    lineNext++;
	    indentNext = IndentAmount(LINEPTR(lineNext), LINEPTR(lineNext - 1), &spaceFlags, NULL);
	}

	levelAfterComments = indentNext & SC_FOLDLEVELNUMBERMASK;
	levelBeforeComments = MAX(indentCurrentLevel,levelAfterComments);

	/* Now set all the indent levels on the lines we skipped
	 * Do this from end to start.  Once we encounter one line
	 * which is indented more than the line after the end of
	 * the comment-block, use the level of the block before */

	skipLine = lineNext;
	skipLevel = levelAfterComments;

#ifdef KEEP_GOING_WHEN_LINE_CHANGES
    	aLineChanged = false;
#endif

	while (--skipLine > lineCurrent) {
	    int skipLineIndent = IndentAmount(LINEPTR(skipLine), LINEPTR(skipLine - 1), &spaceFlags, NULL);
	    int whiteFlag;

	    if ((skipLineIndent & SC_FOLDLEVELNUMBERMASK) > levelAfterComments)
		skipLevel = levelBeforeComments;

	    whiteFlag = skipLineIndent & SC_FOLDLEVELWHITEFLAG;

#ifdef KEEP_GOING_WHEN_LINE_CHANGES
	    if (skipLine > args->lastLine)
		levelBeforeChange = LINEPTR(skipLine)->level;
#endif
	    SetLineFoldLevel(args->sharedPtr, LINEPTR(skipLine), skipLevel, whiteFlag);
#ifdef KEEP_GOING_WHEN_LINE_CHANGES
	    if (skipLine > args->lastLine) {
		if (levelBeforeChange != LINEPTR(skipLine)->level)
		    aLineChanged = true;
	    }
#endif
	}

	/* Set fold header on non-quote/non-comment line */
	if (!quote && !comment && !(indentCurrent & SC_FOLDLEVELWHITEFLAG) ) {
	    if ((indentCurrent & SC_FOLDLEVELNUMBERMASK) < (indentNext & SC_FOLDLEVELNUMBERMASK))
		lev |= SC_FOLDLEVELHEADERFLAG;
	}

	/* Keep track of triple quote and block comment state of previous line */
	prevQuote = quote;
	prevComment = comment_start || comment_continue;

#ifdef KEEP_GOING_WHEN_LINE_CHANGES
	if (lineCurrent > args->lastLine)
	    levelBeforeChange = LINEPTR(lineCurrent)->level;
#endif
	/* Set fold level for this line and move to next line */
	SetLineFoldLevel(args->sharedPtr, LINEPTR(lineCurrent), lev & SC_FOLDLEVELNUMBERMASK, lev);
#ifdef KEEP_GOING_WHEN_LINE_CHANGES
	if (lineCurrent > args->lastLine) {
	    if (levelBeforeChange != LINEPTR(lineCurrent)->level)
		aLineChanged = true;
	}
	/* Report back the last line we actually examined. */
	vars.lineIndex = lineNext - 1;
#endif
	indentCurrent = indentNext;
	lineCurrent = lineNext;
    }

    /* NOTE: Cannot set level of last line here because indentCurrent doesn't have
     * header flag set; the loop above is crafted to take care of this case! */
    /*styler.SetLevel(lineCurrent, indentCurrent); */
    return 0;
}

LexerModule lmPython = {
    "python",
    sizeof(struct Lexer2),
    styleNames,
    ColourisePyDoc,
    FoldPyDoc,
    optionSpecs
};

