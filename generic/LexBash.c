/*
 * LexBash.c --
 *
 * Lexer for bash shell scripts.
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

#define SCE_SH_DEFAULT 0
#define SCE_SH_WHITESPACE 1
#define SCE_SH_WORD1 2
#define SCE_SH_WORD2 3
#define SCE_SH_WORD3 4
#define SCE_SH_WORD4 5
#define SCE_SH_WORD5 6
#define SCE_SH_WORD6 7
#define SCE_SH_WORD7 8
#define SCE_SH_WORD8 9
#define SCE_SH_WORD9 10
#define SCE_SH_ERROR 11
#define SCE_SH_COMMENTLINE 12
#define SCE_SH_NUMBER 13
#define SCE_SH_STRING 14
#define SCE_SH_CHARACTER 15
#define SCE_SH_OPERATOR 16
#define SCE_SH_IDENTIFIER 17
#define SCE_SH_SCALAR 18
#define SCE_SH_PARAM 19
#define SCE_SH_BACKTICKS 20
#define SCE_SH_HERE_DELIM 21
#define SCE_SH_HERE_Q 22

static CONST char *styleNames[] = {
    "default", "whitespace", "keyword1", "keyword2", "keyword3", "keyword4",
    "keyword5", "keyword6", "keyword7", "keyword8", "keyword9", "error",
    "commentline", "number", "string", "character", "operator", "identifier",
    "scalar", "param", "backticks", "here_delim", "here_q", NULL
};

struct Lexer2
{
    struct Lexer lexer;		/* Required first field. */
    int foldComment;
    int foldCompact;
};

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BOOLEAN, "-foldcomment", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldComment), 0, NULL, 0},
    {TK_OPTION_BOOLEAN, "-foldcompact", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(struct Lexer2, foldCompact), 0, NULL, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0, 0, 0}
};

#define BASH_BASE_ERROR		65
#define BASH_BASE_DECIMAL	66
#define BASH_BASE_HEX		67
#define BASH_BASE_OCTAL		68
#define BASH_BASE_OCTAL_ERROR	69

#define HERE_DELIM_MAX 256

static int translateBashDigit(char ch) {
    if (ch >= '0' && ch <= '9') {
	return ch - '0';
    } else if (ch >= 'a' && ch <= 'z') {
	return ch - 'a' + 10;
    } else if (ch >= 'A' && ch <= 'Z') {
	return ch - 'A' + 36;
    } else if (ch == '@') {
	return 62;
    } else if (ch == '_') {
	return 63;
    }
    return BASH_BASE_ERROR;
}

static bool isEOLChar(char ch) {
    return (ch == '\r') || (ch == '\n');
}

static bool isSingleCharOp(char ch) {
    char strCharSet[2];
    strCharSet[0] = ch;
    strCharSet[1] = '\0';
    return (NULL != strstr("rwxoRWXOezsfdlpSbctugkTBMACahGLNn", strCharSet));
}

static bool isBashOperator(char ch) {
    if (ch == '^' || ch == '&' || ch == '\\' || ch == '%' ||
            ch == '(' || ch == ')' || ch == '-' || ch == '+' ||
            ch == '=' || ch == '|' || ch == '{' || ch == '}' ||
            ch == '[' || ch == ']' || ch == ':' || ch == ';' ||
            ch == '>' || ch == ',' || ch == '/' || ch == '<' ||
            ch == '?' || ch == '!' || ch == '.' || ch == '~' ||
	ch == '@')
	return true;
    return false;
}

static int getBashNumberBase(void) {
    char s[11];
    int len = GetCurrent(s, sizeof(s)) - 1; /* only want to charIndex - 1 */
    int i, base = 0;

    for (i = 0; i < len; i++)
	base = base * 10 + (s[i] - '0');
    if (base > 64 || len > 2) {
	return BASH_BASE_ERROR;
    }
    return base;
}

static bool isEndVar(char ch) {
    return !isalnum(ch) && ch != '$' && ch != '_';
}

static char opposite(char ch) {
    if (ch == '(')
	return ')';
    if (ch == '[')
	return ']';
    if (ch == '{')
	return '}';
    if (ch == '<')
	return '>';
    return ch;
}

struct QuoteCls {
    int  Rep;
    int  Count;
    char Up;
    char Down;
};

static void QuoteCls_New(struct QuoteCls *q, int r) {
    q->Rep   = r;
    q->Count = 0;
    q->Up    = '\0';
    q->Down  = '\0';
}
static void QuoteCls_Open(struct QuoteCls *q, char u) {
    q->Count++;
    q->Up    = u;
    q->Down  = opposite(q->Up);
}

static int ColouriseBashDoc(LexerArgs *args)
{
    /* Lexer for bash often has to backtrack to start of current style to determine
     * which characters are being used as quotes, how deeply nested is the
     * start position and what the termination string is for here documents */

    struct {
	int State;		/* 0: '<<' encountered */
	/* 1: collect the delimiter
	 * 2: here doc text (lines after the delimiter) */
	char Quote;		/* the char after '<<' */
	bool Quoted;		/* true if Quote in ('\'','"','`') */
	bool Indent;		/* indented delimiter (for <<-) */
	int DelimiterLength;	/* strlen(Delimiter) */
	char Delimiter[HERE_DELIM_MAX];	/* the Delimiter, 256: sizeof PL_tokenbuf */
    } HereDoc;

    struct QuoteCls Quote;

    int numBase = 0;
    char chNext2;

    memset(&HereDoc, '\0', sizeof(HereDoc));
    QuoteCls_New(&Quote, 1);

    BeginStyling(args, args->firstLine, args->lastLine);

    /* If in a long distance lexical state, seek to the beginning to find quote characters
     * Bash strings can be multi-line with embedded newlines, so backtrack.
     * Bash numbers have additional state during lexing, so backtrack too. */
    if (vars.style == SCE_SH_HERE_Q) {
	while (!AtStartOfDoc() && (vars.style != SCE_SH_HERE_DELIM)) {
	    Back();
	}
	ToLineStart();
    }
    if (vars.style == SCE_SH_STRING
     || vars.style == SCE_SH_BACKTICKS
     || vars.style == SCE_SH_CHARACTER
     || vars.style == SCE_SH_NUMBER
     || vars.style == SCE_SH_IDENTIFIER
     || vars.style == SCE_SH_COMMENTLINE
    ) {
	while (!AtStartOfDoc() && (vars.stylePrev == vars.style)) {
	    Back();
	}
	ChangeState(SCE_SH_DEFAULT);
    }

    for (; More(); Forward()) {
	/* if the current character is not consumed due to the completion of an
	 * earlier style, lexing can be restarted via a simple goto */
    restartLexer:
	chNext2 = SafeGetCharAt(vars.charIndex + 2);
#if 0 /* always false for UTF-8 */
	if (styler.IsLeadByte(ch)) {
	    chNext = styler.SafeGetCharAt(i + 2);
	    chPrev = ' ';
	    i += 1;
	    continue;
	}
#endif
	if ((vars.chPrev == '\r' && vars.chCur == '\n')) {	/* skip on DOS/Windows */
	    /* styler.ColourTo(i, state); */
	    continue;
	}

	if (vars.atLineEnd) {
	    if (HereDoc.State == 1) {
		/* Begin of here-doc (the line after the here-doc delimiter):
		 * Lexically, the here-doc starts from the next line after the >>, but the
		 * first line of here-doc seem to follow the style of the last EOL sequence */
		HereDoc.State = 2;
		if (HereDoc.Quoted) {
		    if (vars.style == SCE_SH_HERE_DELIM) {
			/* Missing quote at end of string! We are stricter than bash.
			 * Colour here-doc anyway while marking this bit as an error. */
			ChangeState(SCE_SH_ERROR);
		    }
		    /* HereDoc.Quote always == '\'' */
		    SetStyle(SCE_SH_HERE_Q);
		} else {
		    /* always switch */
		    SetStyle(SCE_SH_HERE_Q);
		}
		continue;
	    }
	}

	/* Determine if the current style should terminate. */
	if (vars.style == SCE_SH_WHITESPACE) {
	    if (!IsASpaceOrTab(vars.chCur)) {
		SetStyle(SCE_SH_DEFAULT);
	   }
	} else if (vars.style == SCE_SH_OPERATOR) {
	    SetStyle(SCE_SH_DEFAULT);
	} else if (vars.style == SCE_SH_NUMBER) {
	    int digit = translateBashDigit(vars.chCur);
	    if (numBase == BASH_BASE_DECIMAL) {
		if (vars.chCur == '#') {
		    numBase = getBashNumberBase();
		    if (numBase == BASH_BASE_ERROR)	/* take the rest as comment */
			goto numAtEnd;
		} else if (!isdigit(vars.chCur))
		    goto numAtEnd;
	    } else if (numBase == BASH_BASE_HEX) {
		if ((digit < 16) || (digit >= 36 && digit <= 41)) {
		    /* hex digit 0-9a-fA-F */
		} else
		    goto numAtEnd;
	    } else if (numBase == BASH_BASE_OCTAL ||
		   numBase == BASH_BASE_OCTAL_ERROR) {
		if (digit > 7) {
		    if (digit <= 9) {
			numBase = BASH_BASE_OCTAL_ERROR;
		    } else
			goto numAtEnd;
		}
	    } else if (numBase == BASH_BASE_ERROR) {
		if (digit > 9)
		    goto numAtEnd;
	    } else {	/* DD#DDDD number style handling */
		if (digit != BASH_BASE_ERROR) {
		    if (numBase <= 36) {
			/* case-insensitive if base<=36 */
			if (digit >= 36) digit -= 26;
		    }
		    if (digit >= numBase) {
			if (digit <= 9) {
			    numBase = BASH_BASE_ERROR;
			} else
			    goto numAtEnd;
		    }
		} else {
	    numAtEnd:
		    if (numBase == BASH_BASE_ERROR ||
		        numBase == BASH_BASE_OCTAL_ERROR) {
			ChangeState(SCE_SH_ERROR);
		    }
		    SetStyle(SCE_SH_DEFAULT);
		}
	    }
	} else if (vars.style == SCE_SH_IDENTIFIER) {
	    if (!iswordchar(vars.chCur) && vars.chCur != '+' && vars.chCur != '-') {
		char s[100];
		int len = GetCurrent(s, sizeof(s));
		int kw;

		if (MatchKeyword(s, len, &kw)) {
		    ChangeState(SCE_SH_WORD1 + kw);
		}
		SetStyle(SCE_SH_DEFAULT);
	    }
	} else if (vars.style == SCE_SH_COMMENTLINE) {
	    if (vars.chCur == '\\' && isEOLChar(vars.chNext)) {
		/* comment continuation */
		if (vars.chNext == '\r' && chNext2 == '\n') {
		    ForwardN(2);
		} else {
		    Forward();
		}
	    } else if (isEOLChar(vars.chCur)) {
		SetStyle(SCE_SH_DEFAULT);
	    }
	} else if (vars.style == SCE_SH_SCALAR) {	/* variable names */
	    if (isEndVar(vars.chCur)) {
		if (vars.charIndex == (vars.startIndex + 1)) {
		    /* Special variable: $(, $_ etc. */
		    ForwardSetStyle(SCE_SH_DEFAULT);
		} else {
		    SetStyle(SCE_SH_DEFAULT);
		}
	    }
	} else if (vars.style == SCE_SH_STRING
		|| vars.style == SCE_SH_CHARACTER
		|| vars.style == SCE_SH_BACKTICKS
		|| vars.style == SCE_SH_PARAM) {
	    if (!Quote.Down && !isspacechar(vars.chCur)) {
		QuoteCls_Open(&Quote, vars.chCur);
	    } else if (vars.chCur == '\\' && Quote.Up != '\\') {
		Forward();
	    } else if (vars.chCur == Quote.Down) {
		Quote.Count--;
		if (Quote.Count == 0) {
		    Quote.Rep--;
		    if (Quote.Rep <= 0) {
			ForwardSetStyle(SCE_SH_DEFAULT);
		    }
		    if (Quote.Up == Quote.Down) {
			Quote.Count++;
		    }
		}
	    } else if (vars.chCur == Quote.Up) {
		Quote.Count++;
	    }
	} else if (vars.style == SCE_SH_HERE_DELIM) {
	    /*
	     * From Bash info:
	     * ---------------
	     * Specifier format is: <<[-]WORD
	     * Optional '-' is for removal of leading tabs from here-doc.
	     * Whitespace acceptable after <<[-] operator
	     * */
	    if (HereDoc.State == 0) { /* '<<' encountered */
		HereDoc.State = 1;
		HereDoc.Quote = vars.chNext;
		HereDoc.Quoted = false;
		HereDoc.DelimiterLength = 0;
		HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
		if (vars.chNext == '\'' || vars.chNext == '\"') {	/* a quoted here-doc delimiter (' or ") */
		    Forward();
		    HereDoc.Quoted = true;
		} else if (!HereDoc.Indent && vars.chNext == '-') {	/* <<- indent case */
		    HereDoc.Indent = true;
		    HereDoc.State = 0;
		} else if (isalpha(vars.chNext) || vars.chNext == '_' || vars.chNext == '\\'
		    || vars.chNext == '-' || vars.chNext == '+' || vars.chNext == '!') {
		    /* an unquoted here-doc delimiter, no special handling */
		    /* TODO check what exactly bash considers part of the delim */
		} else if (vars.chNext == '<') {	/* HERE string <<< */
		    Forward();
		    SetStyle(SCE_SH_DEFAULT);
		    HereDoc.State = 0;
		} else if (isspacechar(vars.chNext)) {
		    /* eat whitespace */
		    HereDoc.State = 0;
		} else if (isdigit(vars.chNext) || vars.chNext == '=' || vars.chNext == '$') {
		    /* left shift << or <<= operator cases */
		    ChangeState(SCE_SH_OPERATOR);
		    SetStyle(SCE_SH_DEFAULT);
		    HereDoc.State = 0;
		} else {
		    /* symbols terminates; deprecated zero-length delimiter */
		}
	    } else if (HereDoc.State == 1) { /* collect the delimiter */
		if (HereDoc.Quoted) { /* a quoted here-doc delimiter */
		    if (vars.chCur == HereDoc.Quote) { /* closing quote => end of delimiter */
			SetStyle(SCE_SH_DEFAULT);
		    } else {
			if (vars.chCur == '\\' && vars.chNext == HereDoc.Quote) { /* escaped quote */
			    Forward();
			}
			HereDoc.Delimiter[HereDoc.DelimiterLength++] = vars.chCur;
			HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
		    }
		} else { /* an unquoted here-doc delimiter */
		    if (isalnum(vars.chCur) || vars.chCur == '_' || vars.chCur == '-' || vars.chCur == '+' || vars.chCur == '!') {
			HereDoc.Delimiter[HereDoc.DelimiterLength++] = vars.chCur;
			HereDoc.Delimiter[HereDoc.DelimiterLength] = '\0';
		    } else if (vars.chCur == '\\') {
			/* skip escape prefix */
		    } else {
			SetStyle(SCE_SH_DEFAULT);
			goto restartLexer;
		    }
		}
		if (HereDoc.DelimiterLength >= HERE_DELIM_MAX - 1) {
		    SetStyle(SCE_SH_ERROR);
		    goto restartLexer;
		}
	    }
	} else if (HereDoc.State == 2) {
	    /* state == SCE_SH_HERE_Q */
	    if (MatchStr(HereDoc.Delimiter)) {
		if (!HereDoc.Indent && isEOLChar(vars.chPrev) /* vars.atLineStart */) {
		endHereDoc:
		    /* standard HERE delimiter */
		    ForwardN(HereDoc.DelimiterLength);
		    if (isEOLChar(vars.chCur)) {
			SetStyle(SCE_SH_DEFAULT);
			HereDoc.State = 0;
			goto restartLexer;
		    }
		} else if (HereDoc.Indent) {
		    /* indented HERE delimiter */
		    while (!AtStartOfDoc()) {
			Back();
			if (isEOLChar(vars.chCur)) {
			    goto endHereDoc;
			} else if (!isspacechar(vars.chCur)) {
			    break;	/* got leading non-whitespace */
			}
		    }
		}
	    }
	}

	/* Determine if a new state should be entered. */
	if (vars.style == SCE_SH_DEFAULT) {
	    if (IsASpaceOrTab(vars.chCur)) {
		SetStyle(SCE_SH_WHITESPACE);
	    } else if (vars.chCur == '\\') {	/* escaped character */
		ChangeState(SCE_SH_IDENTIFIER); /* styler.ColourTo(i, SCE_SH_IDENTIFIER);*/
	    } else if (isdigit(vars.chCur)) {
		SetStyle(SCE_SH_NUMBER);
		numBase = BASH_BASE_DECIMAL;
		if (vars.chCur == '0') {	/* hex,octal */
		    if (vars.chNext == 'x' || vars.chNext == 'X') {
			numBase = BASH_BASE_HEX;
			Forward();
		    } else if (isdigit(vars.chNext)) {
			numBase = BASH_BASE_OCTAL;
		    }
		}
	    } else if (iswordstart(vars.chCur)) {
		SetStyle(SCE_SH_IDENTIFIER);
	    } else if (vars.chCur == '#') {
		SetStyle(SCE_SH_COMMENTLINE);
	    } else if (vars.chCur == '\"') {
		SetStyle(SCE_SH_STRING);
		QuoteCls_New(&Quote, 1);
		QuoteCls_Open(&Quote, vars.chCur);
	    } else if (vars.chCur == '\'') {
		SetStyle(SCE_SH_CHARACTER);
		QuoteCls_New(&Quote, 1);
		QuoteCls_Open(&Quote, vars.chCur);
	    } else if (vars.chCur == '`') {
		SetStyle(SCE_SH_BACKTICKS);
		QuoteCls_New(&Quote, 1);
		QuoteCls_Open(&Quote, vars.chCur);
	    } else if (vars.chCur == '$') {
		if (vars.chNext == '{') {
		    SetStyle(SCE_SH_PARAM);
		    goto startQuote;
		} else if (vars.chNext == '\'') {
		    SetStyle(SCE_SH_CHARACTER);
		    goto startQuote;
		} else if (vars.chNext == '"') {
		    SetStyle(SCE_SH_STRING);
		    goto startQuote;
		} else if (vars.chNext == '(' && chNext2 == '(') {
		    ChangeState(SCE_SH_OPERATOR);
		    SetStyle(SCE_SH_DEFAULT);
		    goto skipChar;
		} else if (vars.chNext == '(' || vars.chNext == '`') {
		    SetStyle(SCE_SH_BACKTICKS);
		startQuote:
		    QuoteCls_New(&Quote, 1);
		    QuoteCls_Open(&Quote, vars.chNext);
		    goto skipChar;
		} else {
		    SetStyle(SCE_SH_SCALAR);
		skipChar:
		    Forward();
		}
	    } else if (vars.chCur == '*') {
		if (vars.chNext == '*') {	/* exponentiation */
		    Forward();
		}
		SetStyle(SCE_SH_OPERATOR); /*styler.ColourTo(i, SCE_SH_OPERATOR);*/
/*				ForwardSetStyle(SCE_SH_DEFAULT); */
	    } else if (vars.chCur == '<' && vars.chNext == '<') {
		SetStyle(SCE_SH_HERE_DELIM);
		HereDoc.State = 0;
		HereDoc.Indent = false;
	    } else if (vars.chCur == '-'	/* file test operators */
	               && isSingleCharOp(vars.chNext)
	               && !isalnum((chNext2 = SafeGetCharAt(vars.charIndex+2)))) {
		ChangeState(SCE_SH_WORD1);
		ForwardN(2);
		SetStyle(SCE_SH_DEFAULT);
	    } else if (isBashOperator(vars.chCur)) {
		SetStyle(SCE_SH_OPERATOR);
	    } else {
		/* keep colouring defaults to make restart easier */
/*				Flush(); */
	    }
	}
	if (vars.style == SCE_SH_ERROR) {
	    break;
	}
    }
    Complete();
    return 0;
}

#define LINEPTR(n) BTREE_FINDLINE(vars.sharedPtr->peers, n)

static bool IsCommentLine(int line) {
    char ch;
    if (line < 0) return false;
    if (line >= vars.linesInDocument) return false;
    ch = GetFirstNonWSChar(LINEPTR(line));
    return (ch == '#');
}

static int FoldBashDoc(LexerArgs *args) {
    struct Lexer2 *lexer = (struct Lexer2 *) args->lexer;
    bool foldComment = lexer->foldComment;
    bool foldCompact = lexer->foldCompact;
    int visibleChars = 0;
    int levelPrev;
    int levelCurrent;

    BeginStyling(args, args->firstLine, args->lastLine);

    levelPrev = vars.linePtr->level & SC_FOLDLEVELNUMBERMASK;
    levelCurrent = levelPrev;

    for ( ; More(); Forward()) {
	bool atEOL = vars.atLineEnd;
	/* Comment folding */
	if (foldComment && atEOL && IsCommentLine(vars.lineIndex)) {
	    if (!IsCommentLine(vars.lineIndex - 1)
		&& IsCommentLine(vars.lineIndex + 1))
		levelCurrent++;
	    else if (IsCommentLine(vars.lineIndex - 1)
		     && !IsCommentLine(vars.lineIndex+1))
		levelCurrent--;
	}
	if (vars.style == SCE_SH_OPERATOR) {
	    if (vars.chCur == '{') {
		levelCurrent++;
	    } else if (vars.chCur == '}') {
		levelCurrent--;
	    }
	}
	if (atEOL) {
	    int depth = levelPrev;
	    int flags = 0;
	    if (visibleChars == 0 && foldCompact)
		flags |= SC_FOLDLEVELWHITEFLAG;
	    if ((levelCurrent > levelPrev) && (visibleChars > 0))
		flags |= SC_FOLDLEVELHEADERFLAG;
	    SetLineFoldLevel(vars.sharedPtr, vars.linePtr, depth, flags);
	    levelPrev = levelCurrent;
	    visibleChars = 0;
	}
	if (!isspacechar(vars.chCur))
	    visibleChars++;
    }
    /* Fill in the real level of the next line, keeping the current flags as they will be filled in later */
    if (vars.linePtr != NULL) {
	int flagsNext = vars.linePtr->level & ~SC_FOLDLEVELNUMBERMASK;
	SetLineFoldLevel(vars.sharedPtr, vars.linePtr, levelPrev, flagsNext);
    }
    return 0;
}

LexerModule lmBash = {
    "bash",
    sizeof(struct Lexer2),
    styleNames,
    ColouriseBashDoc,
    FoldBashDoc,
    optionSpecs
};

