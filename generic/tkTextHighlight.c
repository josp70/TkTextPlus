/*
 * tkTextHighlight.c --
 *
 *	This file contains code that manages the syntax-highlighting
 *	addition to the Tk Text widget.
 *
 * Copyright (c) 2006-2007 Tim Baker
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define NO_STYLE -1
#define DEFAULT_STYLE 0 /* whitespace */

/* FIXME: This LexVars struct should be per-thread or kept on the stack but
 * I don't want to dereference a pointer everywhere. So I use a mutex to only
 * allow one thread at a time to access it. */
struct LexVars vars;
TCL_DECLARE_MUTEX(varsMutex)

int
FindStyleAtIndex(
    TkSharedText *sharedPtr,
    int lineIndex,
    int byteIndex)
{
    TkTextIndex index;
    TkTextSegment *segPtr;
    int offset;

    if (byteIndex == -1)
	byteIndex = 1000000;
    BTREE_BYTEINDEX(sharedPtr->peers, lineIndex, byteIndex, &index);
    segPtr = TkTextIndexToSeg(&index, &offset);
    if (segPtr->typePtr == &tkTextCharType) {
	return segPtr->body.chst.style[offset];
    }
    return NO_STYLE;
}

int
FindStyleAtSOL(
    TkSharedText *sharedPtr,
    int lineIndex)
{
    return FindStyleAtIndex(sharedPtr, lineIndex, 0);
}

int
FindStyleAtEOL(
    TkSharedText *sharedPtr,
    int lineIndex)
{
    return FindStyleAtIndex(sharedPtr, lineIndex, -1);
}

int
GetFirstNonWSChar(
    TkTextLine *linePtr)
{
    TkTextSegment *segPtr;
    const char *p;
    int n;

    for (segPtr = linePtr->segPtr;
	    segPtr != NULL;
	    segPtr = segPtr->nextPtr) {
	if ((segPtr->typePtr == &tkTextCharType) && (segPtr->size > 0)) {
	    for (p = segPtr->body.chars, n = 0;
		    n < segPtr->size;
		    p++, n++) {
		if (*p != ' ' && *p != '\t')
		    return *p;
	    }
	}
    }
    return '\0';
}

int
GetLineTextClipped(
    TkTextLine *linePtr,
    char *buf,
    int max)
{
    TkTextSegment *segPtr;
    int len = 0;

    for (segPtr = linePtr->segPtr;
	    segPtr != NULL;
	    segPtr = segPtr->nextPtr) {
	if ((segPtr->typePtr == &tkTextCharType) && (segPtr->size > 0)) {
	    if (len + segPtr->size > max) {
		int copy =  max - len;
		(void) memcpy(buf + len, segPtr->body.chars, copy);
		len += copy;
		break;
	    }
	    (void) memcpy(buf + len, segPtr->body.chars, segPtr->size);
	    len += segPtr->size;
	    if (len == max)
		break;
	}
    }

    return len;
}

int
GetLineText(
    TkTextLine *linePtr,
    Tcl_DString *dStringPtr)
{
    TkTextSegment *segPtr;

    Tcl_DStringSetLength(dStringPtr, 0);

    for (segPtr = linePtr->segPtr;
	    segPtr != NULL;
	    segPtr = segPtr->nextPtr) {
	if ((segPtr->typePtr == &tkTextCharType) && (segPtr->size > 0)) {
	    Tcl_DStringAppend(dStringPtr, segPtr->body.chars, segPtr->size);
	}
    }

    return Tcl_DStringLength(dStringPtr);
}

static int
GetLineStyle(
    TkTextLine *linePtr,
    Tcl_DString *dStringPtr)
{
    TkTextSegment *segPtr;

    Tcl_DStringSetLength(dStringPtr, 0);

    for (segPtr = linePtr->segPtr;
	    segPtr != NULL;
	    segPtr = segPtr->nextPtr) {
	if ((segPtr->typePtr == &tkTextCharType) && (segPtr->size > 0)) {
	    Tcl_DStringAppend(dStringPtr, segPtr->body.chst.style, segPtr->size);
	}
    }

    return Tcl_DStringLength(dStringPtr);
}

static void
SetLineStyle(
    Lexer *lexer,
    TkTextLine *linePtr,
    char *buf,
    int len)
{
    TkTextSegment *segPtr;
    int copied = 0;
#ifdef STEXT_DEBUG
    int i;
    for (i = 0; i < len; i++) ASSERT(buf[i] < lexer->numStyles);
#endif

    for (segPtr = linePtr->segPtr;
	    segPtr != NULL;
	    segPtr = segPtr->nextPtr) {
	if ((segPtr->typePtr == &tkTextCharType) && (segPtr->size > 0)) {
	    if (copied + segPtr->size > len) {
		int copy = len - copied;
		(void) memcpy(segPtr->body.chst.style, buf + copied, copy);
		break;
	    }
	    (void) memcpy(segPtr->body.chst.style, buf + copied, segPtr->size);
	    copied += segPtr->size;
	    if (copied >= len)
		break;
	}
    }
}

void
BeginStyling(
    LexerArgs *args,
    int firstLine,
    int lastLine)
{
    TkSharedText *sharedPtr = args->sharedPtr;
    Lexer *lexer = sharedPtr->lexer;
    TkText *textPtr = sharedPtr->peers; /* only needed for macros */

    vars.sharedPtr = sharedPtr;

    if (firstLine > 0) {
	vars.linePrevPtr = BTREE_FINDLINE(textPtr, firstLine - 1);
	ASSERT(vars.linePrevPtr != NULL);
	vars.linePtr = BTREE_NEXTLINE(textPtr, vars.linePrevPtr);
    } else {
	vars.linePrevPtr = NULL;
	vars.linePtr = BTREE_FINDLINE(textPtr, firstLine);
    }
    ASSERT(vars.linePtr != NULL);
    vars.lineNextPtr = BTREE_NEXTLINE(textPtr, vars.linePtr);

    vars.linesInDocument = args->linesInDocument;
    ASSERT(vars.linesInDocument > 0);
    if (lastLine >= vars.linesInDocument)
	lastLine = vars.linesInDocument - 1;
    vars.lineLastPtr = BTREE_FINDLINE(textPtr, lastLine);
    ASSERT(vars.lineLastPtr != NULL);

    vars.lineIndex = firstLine;
    vars.wordListPtrs = lexer->wordListPtrs;
    vars.charIndex = 0;
    vars.lineLength = GetLineText(vars.linePtr, &vars.charDString);
    vars.charBuf = Tcl_DStringValue(&vars.charDString);
    vars.style = (firstLine > 0) ? FindStyleAtEOL(sharedPtr, firstLine - 1) : DEFAULT_STYLE;
    vars.stylePrev = vars.style;
    vars.folding = args->folding;
    if (vars.folding) {
	GetLineStyle(vars.linePtr, &vars.styleDString);
	vars.styleBuf = Tcl_DStringValue(&vars.styleDString);
	vars.style = (vars.lineLength > 0) ? vars.styleBuf[0] : DEFAULT_STYLE;
	vars.styleNext = (vars.lineLength > 1) ? vars.styleBuf[1] : NO_STYLE;
    } else {
	Tcl_DStringSetLength(&vars.styleDString, vars.lineLength);
	vars.styleBuf = Tcl_DStringValue(&vars.styleDString);
#ifdef STEXT_DEBUG
	memset(vars.styleBuf, 0x7f, vars.lineLength);
#endif
    }
    vars.startIndex = 0;
    vars.chPrev = (firstLine > 0) ? '\n' : ' ';
    vars.chCur = (vars.lineLength > 0) ? vars.charBuf[0] : ' ';
    vars.chNext = (vars.lineLength > 1) ? vars.charBuf[1] : ' ';
    vars.atLineStart = true;
    vars.atLineEnd = (vars.chCur == '\r' && vars.chNext != '\n') ||
	(vars.chCur == '\n') || (vars.charIndex >= vars.lineLength);
    vars.changes.checking = false;
}

static void
ForwardOneChar(void)
{
    vars.chPrev = vars.chCur;
    vars.chCur = vars.chNext;
    vars.charIndex++;
    if (vars.charIndex + 1 < vars.lineLength)
	vars.chNext = vars.charBuf[vars.charIndex + 1];
    else
	vars.chNext = ' ';
    vars.stylePrev = vars.style; /* styling function may access stylePrev. */
    if (vars.folding) {
	vars.style = vars.styleNext;
	if (vars.charIndex + 1 < vars.lineLength)
	    vars.styleNext = vars.styleBuf[vars.charIndex + 1];
	else
	    vars.styleNext = NO_STYLE;
    }
}

static void
ForwardOneLine(void)
{
    vars.linePrevPtr = vars.linePtr;
    vars.linePtr = vars.lineNextPtr;
    vars.lineNextPtr = BTREE_NEXTLINE(vars.textPtr, vars.linePtr);
    vars.lineIndex++;
    vars.lineLength = GetLineText(vars.linePtr, &vars.charDString);
    vars.charBuf = Tcl_DStringValue(&vars.charDString);
    vars.stylePrev = vars.style; /* styling function may access stylePrev. */
    if (vars.folding) {
	GetLineStyle(vars.linePtr, &vars.styleDString);
	vars.styleBuf = Tcl_DStringValue(&vars.styleDString);
	vars.style = (vars.lineLength > 0) ? vars.styleBuf[0] : NO_STYLE;
	vars.styleNext = (vars.lineLength > 1) ? vars.styleBuf[1] : NO_STYLE;
    } else {
	Tcl_DStringSetLength(&vars.styleDString, vars.lineLength);
	vars.styleBuf = Tcl_DStringValue(&vars.styleDString);
#ifdef STEXT_DEBUG
	memset(vars.styleBuf, 0x7f, vars.lineLength);
#endif
    }
    vars.startIndex = 0;
    vars.charIndex = 0;
    vars.chPrev = '\n';
    vars.chCur = (vars.lineLength > 0) ? vars.charBuf[0] : ' ';
    /* FIXME: first char on next line */
    vars.chNext = (vars.lineLength > 1) ? vars.charBuf[1] : ' ';
}

void
Forward(void)
{
    if (vars.linePtr == NULL)
	return;

    /* Move forward 1 char in the current line. */
    if (!vars.atLineEnd /*vars.charIndex < vars.lineLength*/) {
	ForwardOneChar();

    /* Advance to the next line unless we just finished the final line. */
    } else if (vars.lineNextPtr != NULL) {
	if (!vars.folding) {
	    vars.charIndex++; /* past newline so it is styled */
	    Flush();
	    SetLineStyle(vars.sharedPtr->lexer, vars.linePtr, vars.styleBuf,
		    vars.lineLength);
	}
	if (vars.lineIndex == vars.linesInDocument - 1) {
	    vars.linePtr = NULL; /* stop */
	    return;
	}

	/* The line we just finished is past the last requested line. */
	if (vars.changes.checking) {
	    /* If the style/state/level is the same as before it was
	     * lexed/folded, then we are done. */
	    if (vars.changes.style == vars.style &&
		    vars.changes.state == vars.linePtr->state &&
		    vars.changes.level == vars.linePtr->level) {
		vars.linePtr = NULL; /* stop */
		return;
	    }

	/* The line we just finished is the last requested line. Keep
	 * checking lines until the style/state/level stops changing.
	 * This is done to repair damage caused by edits. */
	} else if (vars.linePtr == vars.lineLastPtr) {
	    vars.changes.checking = true;
	}

	ForwardOneLine();

	/* Remember the style/state/level before lexing/folding. */
	if (vars.changes.checking) {
	    if (vars.folding) {
		vars.changes.style = (vars.lineLength > 0) ?
		    vars.styleBuf[vars.lineLength - 1] : NO_STYLE;
	    } else {
		vars.changes.style = FindStyleAtEOL(vars.sharedPtr,
		    vars.lineIndex);
	    }
	    vars.changes.state = vars.linePtr->state;
	    vars.changes.level = vars.linePtr->level;
	}

    /* Finished. */
    } else {
	if (!vars.folding) {
	    vars.charIndex++; /* past newline so it is styled */
	    Flush();
	    SetLineStyle(vars.sharedPtr->lexer, vars.linePtr, vars.styleBuf,
		vars.lineLength);
	}
	vars.linePtr = NULL; /* stop */
	return;
    }
    vars.atLineStart = (vars.charIndex == 0);
    vars.atLineEnd = (vars.chCur == '\r' && vars.chNext != '\n') ||
	(vars.chCur == '\n') || (vars.charIndex >= vars.lineLength);
}

void
Back(void)
{
    if (vars.linePtr == NULL)
	return;

    /* Move back 1 char in the current line. */
    if (!vars.atLineStart) {
	vars.chNext = vars.chCur;
	vars.chCur = vars.chPrev;
	vars.charIndex--;
	vars.startIndex--;
	if (vars.charIndex > 0)
	    vars.chPrev = vars.charBuf[vars.charIndex - 1];
	else if (vars.linePrevPtr != NULL)
	    vars.chPrev = '\n';
	else
	    vars.chPrev = ' ';

	vars.styleNext = vars.style;
	vars.style = vars.stylePrev;
	if (vars.charIndex > 0)
	    vars.stylePrev = vars.styleBuf[vars.charIndex - 1];
	else if (vars.linePrevPtr != NULL)
	    vars.stylePrev = FindStyleAtEOL(vars.sharedPtr, vars.lineIndex - 1);
	else
	    vars.stylePrev = NO_STYLE;

    /* Move to the previous line if any. */
    } else if (vars.linePrevPtr != NULL) {
	vars.lineNextPtr = vars.linePtr;
	vars.linePtr = vars.linePrevPtr;
	vars.linePrevPtr = BTREE_PREVLINE(vars.textPtr, vars.linePtr);
	vars.lineIndex--;

	vars.lineLength = GetLineText(vars.linePtr, &vars.charDString);
	vars.charBuf = Tcl_DStringValue(&vars.charDString);
	vars.chNext = vars.chCur;
	vars.chCur = vars.chPrev;
	if (vars.lineLength > 1)
	    vars.chPrev = vars.charBuf[vars.lineLength - 2];
	else if (vars.linePrevPtr != NULL)
	    vars.chPrev = '\n';
	else
	    vars.chPrev = ' ';

	GetLineStyle(vars.linePtr, &vars.styleDString);
	vars.styleBuf = Tcl_DStringValue(&vars.styleDString);
	vars.styleNext = vars.style;
	vars.style = vars.stylePrev;
	if (vars.lineLength > 1)
	    vars.stylePrev = vars.styleBuf[vars.lineLength - 2];
	else if (vars.linePrevPtr != NULL)
	    vars.stylePrev = FindStyleAtEOL(vars.sharedPtr, vars.lineIndex - 1);
	else
	    vars.stylePrev = NO_STYLE;

	vars.charIndex = vars.lineLength - 1;
	vars.startIndex = vars.lineLength - 1;

    /* Finished. */
    } else {
	vars.linePtr = NULL; /* stop */
	return;
    }
    vars.atLineStart = (vars.charIndex == 0);
    vars.atLineEnd = (vars.chCur == '\r' && vars.chNext != '\n') ||
	(vars.chCur == '\n') || (vars.charIndex >= vars.lineLength);
}

void
ForwardN(
    int n)
{
    while (n--) Forward();
}

void
JumpToEOL(void)
{
    if (vars.linePtr == NULL || vars.atLineEnd)
	return;

    /* I'm pretty sure there is *always* a '\n' at the end. */
    ASSERT(vars.lineLength > 0);

    vars.charIndex = vars.lineLength - 1;
    vars.chPrev = (vars.charIndex > 0) ?
	vars.charBuf[vars.charIndex - 1] : (vars.linePrevPtr ? '\n' : '\0');
    vars.chCur = '\n';
    vars.chNext = '\0'; /* FIXME: first char on next line */

    if (!vars.folding) Flush();

    vars.stylePrev = (vars.charIndex > 0) ?
	vars.styleBuf[vars.charIndex - 1] : NO_STYLE; /* FIXME: style at end of previous line */
    if (vars.folding) {
	vars.style = vars.styleBuf[vars.charIndex];
	vars.styleNext = NO_STYLE; /* FIXME: style at start of next line */
    }
    vars.atLineStart = (vars.charIndex == 0);
    vars.atLineEnd = (vars.chCur == '\r' && vars.chNext != '\n') ||
	(vars.chCur == '\n') || (vars.charIndex >= vars.lineLength);
}

bool
MatchStr(
    const char *s)
{
    int n;
    if (vars.chCur != *s)
	return false;
    s++;
    if (vars.chNext != *s)
	return false;
    s++;
    for (n = 2; *s; n++) {
	if (vars.charIndex + n >= vars.lineLength)
	    return false;
	if (*s != vars.charBuf[vars.charIndex + n])
	    return false;
	s++;
    }
    return true;
}

bool
MatchStrAt(
    int i,
    const char *s)
{
    if (i < 0 || i >= vars.lineLength)
	return false;
    for (/**/; *s && i < vars.lineLength; i++, s++) {
	if (vars.charBuf[i] != *s)
	    return false;
    }
    return !*s;
}

void
Flush(void)
{
    while (vars.startIndex < vars.charIndex) {
	vars.styleBuf[vars.startIndex++] = vars.style;
    }
}

void
StyleAhead(
    int count,
    int style)
{
    int n = count;

    Flush();
    while (n-- > 0)
	vars.styleBuf[vars.startIndex++] = style;
    ForwardN(count - 1);
}

int
GetCurrent(
    char *s,
    int len)
{
    int i, n = 0;
    for (i = vars.startIndex; i < vars.charIndex && n < len; i++)
	s[n++] = vars.charBuf[i];
    s[n] = '\0';
    return n;
}

bool
MatchKeyword(
    char *s,
    int len,
    int *index)
{
    int i;

    for (i = 0; i < NUM_WORD_LISTS; i++) {
	if (WordList_InList(vars.wordListPtrs[i], s, len)) {
	    *index = i;
	    return true;
	}
    }
    return false;
}

bool IsEscaped(int offset)
{
    int i = vars.charIndex + offset - 1;
    if ((i >= 0) && (vars.charBuf[i] == '\\')) {
	return !IsEscaped(offset - 1);
    }
    return false;
}

/* ======================================== */

void
WordList_Init(
    WordList *wl)
{
    wl->words = NULL;
    wl->lengths = NULL;
    wl->numWords = 0;
    wl->sorted = false;
}

void
WordList_Clear(
    WordList *wl)
{
    if (wl->words) {
	ckfree((char *) wl->words);
	ckfree((char *) wl->lengths);
    }
    wl->words = NULL;
    wl->numWords = 0;
    wl->sorted = false;
}

void
WordList_Free(
    WordList *wl)
{
    WordList_Clear(wl);
}

void
WordList_Set(
    WordList *wl,
    const char **words)
{
    int i;
    for (i = 0; words[i]; i++)
	;
    wl->numWords = i;
    wl->lengths = (int *) ckalloc(i * sizeof(int));
    wl->words = words;
    wl->sorted = false;
}

static int
cmpString(
    const void *a1,
    const void *a2)
{
    return strcmp(*(char**)(a1), *(char**)(a2));
}

static void
SortWordList(
    const char **words,
    unsigned int len)
{
    qsort(words, len, sizeof(*words), cmpString);
}

bool
WordList_InList(
    WordList *wl,
    const char *s,
    int len)
{
    unsigned char firstChar;
    int j;
    const char **words = wl->words;
    if (words == NULL)
	return false;
    if (!wl->sorted) {
	unsigned int k;
	int l;
	SortWordList(words, wl->numWords);
	for (k = 0; k < (sizeof(wl->starts) / sizeof(wl->starts[0])); k++)
	    wl->starts[k] = -1;
	for (l = wl->numWords - 1; l >= 0; l--) {
	    unsigned char indexChar = words[l][0];
	    wl->starts[indexChar] = l;
	    wl->lengths[l] = strlen(words[l]);
	}
	wl->sorted = true;
    }
    firstChar = s[0];
    j = wl->starts[firstChar];
    if (j >= 0) {
	while (words[j][0] == firstChar) {
	    if ((s[1] == words[j][1]) && (len == wl->lengths[j])) {
		const char *a = words[j] + 1;
		const char *b = s + 1;
		while (*a && *a == *b) {
		    a++;
		    b++;
		}
		if (!*a /* && !*b */)
		    return true;
if (*a > *b) break;
	    }
	    j++;
if (!words[j]) break;
	}
    }
    j = wl->starts['^'];
    if (j >= 0) {
	while (words[j][0] == '^') {
	    const char *a = words[j] + 1;
	    const char *b = s;
	    while (*a && *a == *b) {
		a++;
		b++;
	    }
	    if (!*a)
		return true;
	    j++;
if (!words[j]) break;
	}
    }
    return false;
}

/* ======================================== */

typedef struct LexerInterpData
{
    LexerModule *lmHead;
} LexerInterpData;

static Tk_OptionSpec lexerOptionSpecs[] = {
    {TK_OPTION_STRING, "-bracestyle", (char *) NULL, (char *) NULL,
	(char *) NULL, -1, Tk_Offset(Lexer, braceStyleStr),
	TK_CONFIG_NULL_OK, NULL, 0},
    {TK_OPTION_BOOLEAN, "-enable", (char *) NULL, (char *) NULL,
	"1", -1, Tk_Offset(Lexer, enable), 0, NULL, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0, 0, 0}
};

static void
LexerModule_Init(
    Tcl_Interp *interp,
    LexerModule *lm)
{
    if (lm->optionSpecs != NULL) {
	Tk_OptionSpec *specPtr;

	/* Chain on the generic options. */
	for (specPtr = lm->optionSpecs;
		specPtr->type != TK_OPTION_END;
		specPtr++)
	    ;
	specPtr->clientData = lexerOptionSpecs;
	lm->optionTable = Tk_CreateOptionTable(interp, lm->optionSpecs);
    } else {
	lm->optionTable = Tk_CreateOptionTable(interp, lexerOptionSpecs);
    }
    lm->next = NULL;
}

static LexerModule *
LexerModule_Add(
    Tcl_Interp *interp,
    LexerModule *lmHead,
    LexerModule *lmTemplate)
{
    LexerModule *lm, *walk = lmHead;

    lm = (LexerModule *) ckalloc(sizeof(LexerModule));
    *lm = *lmTemplate;
    LexerModule_Init(interp, lm);

    if (walk == NULL) {
	return lm;
    }
    while (walk->next)
	walk = walk->next;
    walk->next = lm;
    return lmHead;
}

static void
FreeLexerInterpData(
    ClientData clientData,
    Tcl_Interp *interp)
{
    LexerInterpData *interpData = clientData;
    LexerModule *lmHead = interpData->lmHead, *next;

    while (lmHead != NULL) {
	next = lmHead->next;
	ckfree((char *) lmHead);
	lmHead = next;
    }
    ckfree((char *) interpData);
}

extern LexerModule lmBash;
extern LexerModule lmTOL;
extern LexerModule lmCPP;
extern LexerModule lmLua;
extern LexerModule lmMake;
extern LexerModule lmPython;
/* extern LexerModule lmRuby; */
extern LexerModule lmTcl;

static LexerModule *
LexerModule_InitAll(
    Tcl_Interp *interp)
{
    LexerInterpData *interpData = Tcl_GetAssocData(interp, "STextLexer", NULL);
    LexerModule *lmHead;

    if (interpData != NULL)
	return interpData->lmHead;

    interpData = (LexerInterpData *) ckalloc(sizeof(LexerInterpData));
    Tcl_SetAssocData(interp, "STextLexer", FreeLexerInterpData, interpData);

    lmHead = NULL;
    lmHead = LexerModule_Add(interp, lmHead, &lmBash);
    lmHead = LexerModule_Add(interp, lmHead, &lmCPP);
    lmHead = LexerModule_Add(interp, lmHead, &lmTOL);
    lmHead = LexerModule_Add(interp, lmHead, &lmLua);
    lmHead = LexerModule_Add(interp, lmHead, &lmMake);
    lmHead = LexerModule_Add(interp, lmHead, &lmPython);
/*    lmHead = LexerModule_Add(interp, lmHead, &lmRuby);*/
    lmHead = LexerModule_Add(interp, lmHead, &lmTcl);

    Tcl_DStringInit(&vars.charDString); /* FIXME: never freed */
    Tcl_DStringInit(&vars.styleDString); /* FIXME: never freed */

    return interpData->lmHead = lmHead;
}

#define LexerModule_Head(interp) LexerModule_InitAll(interp)

static LexerModule *
LexerModule_Find(
    Tcl_Interp *interp,
    CONST char *name)
{
    LexerModule *walk;

    walk = LexerModule_InitAll(interp);

    while (walk) {
	if (!strcmp(walk->name, name))
	    return walk;
	walk = walk->next;
    }
    return NULL;
}

static void
DeleteTag(
    TkText *textPtr,
    TkTextTag *tagPtr
    )
{
    Tcl_HashEntry *hPtr;

    if (tagPtr->affectsDisplay) {
	TkTextRedrawTag(textPtr->sharedTextPtr, NULL,
		NULL, NULL, tagPtr, 1);
    }
    hPtr = Tcl_FindHashEntry(&textPtr->sharedTextPtr->tagTable,
	    tagPtr->name);
    TkTextDeleteTag(textPtr, tagPtr);
    Tcl_DeleteHashEntry(hPtr);
}

/* Called from tkText.c as well */
void
Lexer_FreeInstance(
    TkText *textPtr,
    Lexer *lexer)
{
    int i;

    Tk_FreeConfigOptions((char *) lexer, lexer->lm->optionTable,
	    textPtr->tkwin);
    for (i = 0; i < lexer->numStyles; i++)
	DeleteTag(textPtr, lexer->tags[i]);
    ckfree((char *) lexer->tags);
    lexer->tags = NULL;
    for (i = 0; i < NUM_WORD_LISTS; i++)
	WordList_Free(lexer->wordListPtrs[i]);
    ckfree((char *) lexer);
}

static Lexer *
Lexer_NewInstance(
    TkText *textPtr,
    LexerModule *lm)
{
    Lexer *lexer;
    int i;

    lexer = (Lexer *) ckalloc(lm->size);
    memset(lexer, '\0', lm->size);
    lexer->lm = lm;
    for (i = 0; i < NUM_WORD_LISTS; i++) {
	lexer->wordListPtrs[i] = &lexer->wordLists[i];
	WordList_Init(lexer->wordListPtrs[i]);
    }
    lexer->enable = 1;
    lexer->braceStyle = NO_STYLE;
    lexer->braceStyleStr = NULL;
    lexer->startLine = lexer->endLine = -1;
    for (i = 0; lm->styleNames[i]; i++)
	;
    lexer->numStyles = i;
    lexer->tags = (TkTextTag **) ckalloc(lexer->numStyles *
	    sizeof(TkTextTag *));
    for (i = 0; i < lexer->numStyles; i++) {
	lexer->tags[i] = TkTextCreateTag(textPtr, lexer->lm->styleNames[i],
		NULL);
    }
    if (Tk_InitOptions(textPtr->interp, (char *) lexer, lm->optionTable,
	    textPtr->tkwin) != TCL_OK) {
	Lexer_FreeInstance(textPtr, lexer);
	return NULL;
    }
    return lexer;
}

/* ======================================== */

int
IndentAmount(
    TkTextLine *linePtr,
    TkTextLine *linePrevPtr,
    int *flags,
    PFNIsCommentLeader pfnIsCommentLeader)
{
    char buf[256];
    int end = GetLineTextClipped(linePtr, buf, 512);
    int spaceFlags = 0;

    /* Determines the indentation level of the current line and also checks for consistent
     * indentation compared to the previous line.
     * Indentation is judged consistent when the indentation whitespace of each line lines
     * the same or the indentation of one line is a prefix of the other. */

    int pos = 0;
    char ch = end ? buf[pos] : '\0';
    int indent = 0;
    bool inPrevPrefix = linePrevPtr != NULL;
    int posPrev = inPrevPrefix ? 0 : 0;
    char bufPrev[256];
    int endPrev = inPrevPrefix ? GetLineTextClipped(linePrevPtr, bufPrev, 256) : 0;
    while ((ch == ' ' || ch == '\t') && (pos < end)) {
	if (inPrevPrefix) {
	    char chPrev = (posPrev < endPrev) ? bufPrev[posPrev++] : '\0';
	    if (chPrev == ' ' || chPrev == '\t') {
		if (chPrev != ch)
		    spaceFlags |= wsInconsistent;
	    } else {
		inPrevPrefix = false;
	    }
	}
	if (ch == ' ') {
	    spaceFlags |= wsSpace;
	    indent++;
	} else {	/* Tab */
	    spaceFlags |= wsTab;
	    if (spaceFlags & wsSpace)
		spaceFlags |= wsSpaceTab;
	    indent = (indent / 8 + 1) * 8;
	}
	if (pos + 1 == end)
	    break;
	ch = buf[++pos];
    }

    *flags = spaceFlags;
    indent += SC_FOLDLEVELBASE;
    /* if completely empty line or the start of a comment... */
    if ((ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') ||
	(pfnIsCommentLeader && (*pfnIsCommentLeader)(buf, pos, end-pos)) )
	return indent | SC_FOLDLEVELWHITEFLAG;
    else
	return indent;
}

#define DLINE(n) ((n) + 1) /* BTree line index -> display line index. */

static void
LexAndFold(
    TkSharedText *sharedPtr,
    int startLine,
    int numLines
    )
{
    Lexer *lexer = sharedPtr->lexer;
    LexerArgs lexerArgs;
    TkTextIndex index1, index2;
    TkTextLine *linePtr;
    int lastLine, lastStyledLine;

    ASSERT(startLine >= 0);
    ASSERT(numLines > 0);

    lexerArgs.lexer = lexer;
    lexerArgs.sharedPtr = sharedPtr;
    lexerArgs.linesInDocument = BTREE_NUMLINES(sharedPtr->peers);
    lastLine = startLine + numLines - 1;
    if (lastLine >= lexerArgs.linesInDocument)
	lastLine = lexerArgs.linesInDocument - 1;
    lexerArgs.firstLine = startLine;
    lexerArgs.lastLine = lastLine;

    Tcl_MutexLock(&varsMutex);

    lexerArgs.folding = false;
    (*lexer->lm->fnLexer)(&lexerArgs);
dbwin("LEX %s %d-%d", Tk_PathName(sharedPtr->peers->tkwin), DLINE(lexerArgs.firstLine), DLINE(lexerArgs.lastLine));
dbwin("LEX %s following until %d", Tk_PathName(sharedPtr->peers->tkwin), DLINE(vars.lineIndex));
    lastLine = vars.lineIndex;
    lastStyledLine = vars.lineIndex;

    /* Fold the requested lines plus any following lines that were lexed
     * because of EOL style changes. */
    if (lexer->lm->fnFolder != NULL) {
	lexerArgs.firstLine = startLine;
	lexerArgs.lastLine = lastLine;
	lexerArgs.folding = true;
	(*lexer->lm->fnFolder)(&lexerArgs);
dbwin("FOLD %s %d-%d", Tk_PathName(sharedPtr->peers->tkwin), DLINE(lexerArgs.firstLine), DLINE(lexerArgs.lastLine));
dbwin("FOLD %s following until %d", Tk_PathName(sharedPtr->peers->tkwin), DLINE(vars.lineIndex));
	lastLine = vars.lineIndex;
    }

    Tcl_MutexUnlock(&varsMutex);

    {
	/* Go through all the affected lines to fix problems caused by
	 * editing. If a line is marked as folded but is no longer foldable,
	 * then mark it as unfolded. If a line is marked as hidden but has
	 * no fold-parent, or has a fold-parent that isn't folded, then show
	 * it. */
	int lineIndex = startLine;
	int keepGoing = 0;
	for (linePtr = BTREE_FINDLINE(sharedPtr->peers, startLine);
		linePtr != NULL;
		linePtr = BTREE_NEXTLINE(sharedPtr->peers, linePtr), lineIndex++) {
	    TkText *peer;
	    for (peer = sharedPtr->peers;
		    peer != NULL;
		    peer = peer->next) {

		/* Case 1: folded unfoldable line. */
		if (GetLineFolded(peer, linePtr) &&
			!GetLineFoldable(sharedPtr, linePtr)) {
dbwin("REPAIR folded unfoldable line %d (%s)", DLINE(lineIndex), Tk_PathName(peer->tkwin));
		    SetLineFolded(peer, linePtr, false);
		}

		/* Case 2: Hidden line with no fold-parent. */
		if (!GetLineVisible(peer, linePtr)) {
		    TkTextLine *parent = GetFoldParent(sharedPtr, linePtr);
		    if (parent == NULL || parent == linePtr ||
			    !GetLineFolded(peer, parent)) {
			SetLineVisible(peer, linePtr, true);
dbwin("REPAIR hidden line %d (%s)", DLINE(lineIndex), Tk_PathName(peer->tkwin));
			keepGoing++;
		    }
		}
	    }
	    if (lineIndex == lastLine + keepGoing) break;
	}
    }

    BTREE_BYTEINDEX(sharedPtr->peers, startLine, 0, &index1);
    BTREE_BYTEINDEX(sharedPtr->peers, lastLine + 1, 0, &index2);
    BTREE_TEXTCHANGED(sharedPtr->peers, &index1, &index2);
    if (startLine == 0 && lastStyledLine + 1 >= lexerArgs.linesInDocument) {
dbwin("LexAndFold INVALIDATE WHOLE DOC");
	index1.linePtr = NULL; /* invalidate the whole document. */
    }

    /* Note this call only invalidates the metrics of lines whose styles
     * were set, not any additional lines whose fold-state may have
     * changed. */
    TkTextInvalidateLineMetrics(sharedPtr, NULL,
	    index1.linePtr, lastStyledLine - startLine, TK_TEXT_INVALIDATE_ONLY);
}

static void
RemoveStyle(
    TkText *textPtr)
{
    TkTextLine *linePtr;
    TkTextSegment *segPtr;

dbwin("RemoveStyle %s", Tk_PathName(textPtr->tkwin));
    for (linePtr = BTREE_FINDLINE(textPtr, 0);
	    linePtr != NULL;
	    linePtr = BTREE_NEXTLINE(textPtr, linePtr)){
	for (segPtr = linePtr->segPtr;
	    segPtr != NULL;
	    segPtr = segPtr->nextPtr) {
	    if (segPtr->typePtr == &tkTextCharType) {
		memset(segPtr->body.chst.style, NO_STYLE, segPtr->size);
	    }
	}
	SetLineState(textPtr, linePtr, 0);
	linePtr->level = 0;
    }

{
    TkTextIndex index1, index2;
    BTREE_BYTEINDEX(textPtr, 0, 0, &index1);
    BTREE_BYTEINDEX(textPtr, BTREE_NUMLINES(textPtr), 0, &index2);
    BTREE_TEXTCHANGED(textPtr, &index1, &index2);
    TkTextInvalidateLineMetrics(textPtr->sharedTextPtr, NULL,
	    NULL, 0, TK_TEXT_INVALIDATE_ONLY);
}
}

static void
RemoveStyleFromLine(
    TkText *textPtr,
    TkTextLine *linePtr)
{
    TkTextSegment *segPtr;

    if (linePtr == NULL) return;
dbwin("RemoveStyleFromLine line %d\n", BTREE_LINESTO(textPtr, linePtr) + 1);
    segPtr = linePtr->segPtr;
    while (segPtr != NULL) {
	if (segPtr->typePtr == &tkTextCharType) {
	    memset(segPtr->body.chst.style, NO_STYLE, segPtr->size);
	}
	segPtr = segPtr->nextPtr;
    }
    SetLineState(textPtr, linePtr, 0);
    linePtr->level = 0;
}

static void
EventuallyLexAndFold(
    TkSharedText *sharedPtr,
    int startLine,
    int numLines)
{
    Lexer *lexer = sharedPtr->lexer;
    TkText *peer;

    if (lexer->startLine == -1) {
	lexer->startLine = startLine;
	lexer->endLine = startLine + numLines;
    } else {
	if (startLine < lexer->startLine)
	    lexer->startLine = startLine;
	if (startLine + numLines > lexer->endLine)
	    lexer->endLine = startLine + numLines;
    }

    for (peer = sharedPtr->peers;
	    peer != NULL;
	    peer = peer->next) {
	TkTextEventuallyUpdateDInfo(peer);
    }
}

void
LexerInsertion(
    TkSharedText *sharedPtr,
    int startLine,
    int numLines)
{
    if (sharedPtr->lexer == NULL || !sharedPtr->lexer->enable)
	return;

    EventuallyLexAndFold(sharedPtr, startLine, numLines);
}

void
LexerDeletion(
    TkSharedText *sharedPtr,
    int startLine,
    int numLines)
{
    if (sharedPtr->lexer == NULL || !sharedPtr->lexer->enable)
	return;

    EventuallyLexAndFold(sharedPtr, startLine, 0);
}

void
LexerLexNeeded(
    TkSharedText *sharedPtr)
{
    Lexer *lexer = sharedPtr->lexer;

    if (lexer == NULL || !lexer->enable || lexer->startLine == -1)
	return;

    LexAndFold(sharedPtr, lexer->startLine,
	lexer->endLine - lexer->startLine + 1);

    lexer->startLine = lexer->endLine = -1;
}

bool
Lexer_OwnsTag(
    Lexer *lexer,
    TkTextTag *tagPtr)
{
    int i;

    for (i = 0; i < lexer->numStyles; i++) {
	if (lexer->tags[i] == tagPtr)
	    return true;
    }
    return false;
}

TkTextTag *
Lexer_TagAtIndex(
    Lexer *lexer,
    CONST TkTextIndex *indexPtr)
{
    int offsetInSeg;
    TkTextSegment *segPtr;

    if (lexer == NULL || !lexer->enable || lexer->tags == NULL)
	return NULL;

    segPtr = TkTextIndexToSeg(indexPtr, &offsetInSeg);
    if (segPtr->typePtr == &tkTextCharType) {
	int styleIndex = segPtr->body.chst.style[offsetInSeg];
	if (styleIndex != NO_STYLE) {
	    return lexer->tags[styleIndex];
	}
    }
    return NULL;
}

/* ======================================== */

bool TkTextIndexForwCharsExt(
    CONST TkText *textPtr,
    TkTextIndex *dstPtr,
    TkTextSegment **segPtrPtr,
    int *byteOffsetPtr,
    int charCount);

bool
TkTextIndexBackCharsExt(
    CONST TkText *textPtr, /* indices are not into shared data */
    TkTextIndex *dstPtr,
    TkTextSegment **segPtrPtr,
    int *byteOffsetPtr,
    int charCount)
{
    TkTextSegment *segPtr, *oldPtr;
    int lineIndex, segSize;
    CONST char *p;
    char *start, *end;

    if (charCount <= 0) {
	return TkTextIndexForwCharsExt(textPtr, dstPtr, segPtrPtr,
	    byteOffsetPtr, -charCount);
    }

    lineIndex = -1;

    segSize = dstPtr->byteIndex;
    for (segPtr = dstPtr->linePtr->segPtr; ; segPtr = segPtr->nextPtr) {
	if (segPtr == NULL) {
	    /*
	    * Two logical lines merged into one display line through
	    * eliding of a newline.
	    */

	    segPtr = BTREE_NEXTLINE(textPtr, dstPtr->linePtr)->segPtr;
	}
	if (segSize <= segPtr->size) {
	    break;
	}
	segSize -= segPtr->size;
    }
    while (1) {
	if (segPtr->typePtr == &tkTextCharType) {
	    start = segPtr->body.chars;
	    end = segPtr->body.chars + segSize;
	    for (p = end; ; p = Tcl_UtfPrev(p, start)) {
		if (charCount == 0) {
		    dstPtr->byteIndex -= (end - p);
		    *segPtrPtr = segPtr;
		    *byteOffsetPtr = p - start;
		    return true;
		}
		if (p == start) {
		    break;
		}
		charCount--;
	    }
	} else {
	    if (charCount <= segSize) {
		dstPtr->byteIndex -= charCount;
		*segPtrPtr = segPtr;
		*byteOffsetPtr = segSize; /* ??? */
		return true;
	    }
	    charCount -= segSize;
	}
	dstPtr->byteIndex -= segSize;

	oldPtr = segPtr;
	segPtr = dstPtr->linePtr->segPtr;
	if (segPtr != oldPtr) {
	    for (; segPtr->nextPtr != oldPtr; segPtr = segPtr->nextPtr) {
		/* Empty body. */
	    }
	    segSize = segPtr->size;
	    continue;
	}

	if (lineIndex < 0) {
	    lineIndex = BTREE_LINESTO(textPtr, dstPtr->linePtr);
	}
	if (lineIndex == 0) {
	    return false;
	}
	lineIndex--;
	dstPtr->linePtr = BTREE_FINDLINE(textPtr, lineIndex);

	oldPtr = dstPtr->linePtr->segPtr;
	for (segPtr = oldPtr; segPtr != NULL; segPtr = segPtr->nextPtr) {
	    dstPtr->byteIndex += segPtr->size;
	    oldPtr = segPtr;
	}
	segPtr = oldPtr;
	segSize = segPtr->size;
    }
}

bool
TkTextIndexForwCharsExt(
    CONST TkText *textPtr, /* indices are not into shared data */
    TkTextIndex *dstPtr,
    TkTextSegment **segPtrPtr,
    int *byteOffsetPtr,
    int charCount)
{
    TkTextLine *linePtr;
    TkTextSegment *segPtr = *segPtrPtr;
    int byteOffset = *byteOffsetPtr;
    char *start, *end, *p;
    Tcl_UniChar ch;

    if (charCount < 0) {
	return TkTextIndexBackCharsExt(textPtr, dstPtr, segPtrPtr,
	    byteOffsetPtr, -charCount);
    }

    while (1) {
	/* FIXME: check for elided text? */
	for (; segPtr != NULL; segPtr = segPtr->nextPtr) {
	    if (segPtr->typePtr == &tkTextCharType) {
		start = segPtr->body.chars + byteOffset;
		end = segPtr->body.chars + segPtr->size;
		for (p = start; p < end; p += Tcl_UtfToUniChar(p, &ch)) {
		    if (charCount == 0) {
			dstPtr->byteIndex += (p - start);
			*segPtrPtr = segPtr;
			*byteOffsetPtr = byteOffset + (p - start);
			return true;
		    }
		    charCount--;
		}
	    } else {
		if (charCount < segPtr->size - byteOffset) {
		    dstPtr->byteIndex += charCount;
		    *segPtrPtr = segPtr;
		    *byteOffsetPtr = byteOffset + charCount;
		    return true;
		}
		charCount -= segPtr->size - byteOffset;
	    }
	    dstPtr->byteIndex += segPtr->size - byteOffset;
	    byteOffset = 0;
	}

	linePtr = BTREE_NEXTLINE(textPtr, dstPtr->linePtr);
	if (linePtr == NULL) {
	    return false;
	}
	dstPtr->linePtr = linePtr;
	dstPtr->byteIndex = 0;
	segPtr = dstPtr->linePtr->segPtr;
    }
}

static char
BraceOpposite(
    char ch,
    int *direction)
{
    switch (ch) {
	case '(':
	    *direction = 1;
	    return ')';
	case ')':
	    *direction = -1;
	    return '(';
	case '[':
	    *direction = 1;
	    return ']';
	case ']':
	    *direction = -1;
	    return '[';
	case '{':
	    *direction = 1;
	    return '}';
	case '}':
	    *direction = -1;
	    return '{';
	case '<':
	    *direction = 1;
	    return '>';
	case '>':
	    *direction = -1;
	    return '<';
	default:
	    return '\0';
    }
}

/*
 * The following macros greatly simplify moving through a table...
 */
#define STRING_AT(table, offset, index) \
	(*((CONST char * CONST *)(((char *)(table)) + ((offset) * (index)))))
#define NEXT_ENTRY(table, offset) \
	(&(STRING_AT(table, offset, 1)))

int
Tcl_GetIndexStruct(interp, key, tablePtr, offset, msg, flags, 
	indexPtr)
    Tcl_Interp *interp; 	/* Used for error reporting if not NULL. */
    CONST char *key;		/* String to lookup. */
    CONST VOID *tablePtr;	/* The first string in the table. The second
				 * string will be at this address plus the
				 * offset, the third plus the offset again,
				 * etc. The last entry must be NULL
				 * and there must not be duplicate entries. */
    int offset;			/* The number of bytes between entries */
    CONST char *msg;		/* Identifying word to use in error messages. */
    int flags;			/* 0 or TCL_EXACT */
    int *indexPtr;		/* Place to store resulting integer index. */
{
    int i, index = -1, numAbbrev = 0, count;
    CONST char *p1, *p2;
    CONST char * CONST *entryPtr;

    for (entryPtr = tablePtr, i = 0; *entryPtr != NULL;
	    entryPtr = NEXT_ENTRY(entryPtr, offset), i++) {
	for (p1 = key, p2 = *entryPtr; *p1 == *p2; p1++, p2++) {
	    if (*p1 == '\0') {
		index = i;
		goto done;
	    }
	}
	if (*p1 == '\0') {
	    numAbbrev++;
	    index = i;
	}
    }

    if ((flags & TCL_EXACT) || (key[0] == '\0') || (numAbbrev != 1))
	goto error;

done:
    *indexPtr = index;
    return TCL_OK;

error:
    Tcl_AppendResult(interp, (numAbbrev > 1) &&
	    !(flags & TCL_EXACT) ? "ambiguous " : "bad ", msg, " \"",
	    key, "\": must be ", STRING_AT(tablePtr,offset,0), NULL);
    for (entryPtr = NEXT_ENTRY(tablePtr, offset), count = 0;
	    *entryPtr != NULL;
	    entryPtr = NEXT_ENTRY(entryPtr, offset), count++) {
	if (*NEXT_ENTRY(entryPtr, offset) == NULL) {
	    Tcl_AppendResult(interp, (count > 0) ? ", or " : " or ",
		*entryPtr, NULL);
	} else {
	    Tcl_AppendResult(interp, ", ", *entryPtr, NULL);
	}
    }
    return TCL_ERROR;
}

int
Tcl_GetIndex(interp, key, tablePtr, msg, flags, indexPtr)
    Tcl_Interp *interp; 	/* Used for error reporting if not NULL. */
    CONST char *key;		/* String to lookup. */
    CONST char **tablePtr;	/* Array of strings to compare against key;
				 * last entry must be NULL
				 * and there must not be duplicate entries. */
    CONST char *msg;		/* Identifying word to use in error messages. */
    int flags;			/* 0 or TCL_EXACT */
    int *indexPtr;		/* Place to store resulting integer index. */
{
    return Tcl_GetIndexStruct(interp, key, tablePtr, sizeof(char *),
	    msg, flags, indexPtr);
}

int
LexerConfigureCmd(
    TkText *textPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[])
{
    TkSharedText *sharedPtr = textPtr->sharedTextPtr;
    Lexer *lexer = sharedPtr->lexer;
    Tcl_Obj *resultObjPtr;
    int braceStyle = NO_STYLE;
    Tk_SavedOptions savedOptions;
    int error;
    Tcl_Obj *errorResult = NULL;
    int mask;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 3, objv, "?option? ?value?");
	return TCL_ERROR;
    }
    if (lexer == NULL) {
	Tcl_AppendResult(interp, "lexer is unspecified", NULL);
	return TCL_ERROR;
    }
    if (objc <= 4) {
	resultObjPtr = Tk_GetOptionInfo(interp, (char *) lexer,
		lexer->lm->optionTable,
		(objc == 3) ? (Tcl_Obj *) NULL : objv[3],
		textPtr->tkwin);
	if (resultObjPtr == NULL)
	    return TCL_ERROR;
	Tcl_SetObjResult(interp, resultObjPtr);
	return TCL_OK;
    }

    for (error = 0; error <= 1; error++) {
	if (error == 0) {
	    if (Tk_SetOptions(textPtr->interp, (char *) lexer,
		    lexer->lm->optionTable,
		    objc - 3, objv + 3, textPtr->tkwin, &savedOptions, &mask)
		    != TCL_OK) {
		mask = 0;
		continue;
	    }

	    if (lexer->braceStyleStr != NULL) {
		if (Tcl_GetIndex(interp, lexer->braceStyleStr,
			lexer->lm->styleNames, "style", 0,
			&braceStyle) != TCL_OK) {
		    continue;
		}
	    }

	    Tk_FreeSavedOptions(&savedOptions);
	    break;
	} else {
	    errorResult = Tcl_GetObjResult(interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);

		/* xxx */

	    Tcl_SetObjResult(interp, errorResult);
	    Tcl_DecrRefCount(errorResult);
	    return TCL_ERROR;
	}
    }

    lexer->braceStyle = braceStyle;
    /* FIXME: if the lexer is not enabled or becomes not enabled before
     * the next display update then things won't get redrawn. */
    EventuallyLexAndFold(sharedPtr, 0, BTREE_NUMLINES(textPtr));
    return TCL_OK;
}

/*
 * .text lexer bracematch
 * .text lexer cget option
 * .text lexer configure ?option value ...?
 * .text lexer invoke ?start? ?end?
 * .text lexer keywords 1|2|3|4|5|6|7|8|9 ?list?
 * .text lexer names
 * .text lexer set ?lexer?
 * .text lexer styleat index
 * .text lexer stylenames
 */
int
TkTextLexerCmd(
    TkText *textPtr,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *CONST objv[]
    )
{
    TkSharedText *sharedTextPtr = textPtr->sharedTextPtr;
    static CONST char *cmdNames[] = {
	"bracematch", "cget", "configure", "invoke", "keywords", "names",
	"set", "styleat", "stylenames", NULL };
    enum {
	CMD_BRACEMATCH, CMD_CGET, CMD_CONFIGURE, CMD_INVOKE, CMD_KEYWORDS,
	CMD_NAMES, CMD_SET, CMD_STYLEAT, CMD_STYLENAMES
    };
    int result = TCL_OK;
    int i;
    int index;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[2], cmdNames, "command", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    Tcl_Preserve((ClientData) textPtr);

    switch (index) {
	case CMD_BRACEMATCH: {
	    CONST TkTextIndex *indexPtr;
	    int offset;
	    TkTextSegment *segPtr;

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "index");
		result = TCL_ERROR;
		break;
	    }
	    indexPtr = TkTextGetIndexFromObj(interp, textPtr, objv[3]);
	    if (indexPtr == NULL) {
		result = TCL_ERROR;
		break;
	    }
	    if (sharedTextPtr->lexer == NULL) {
		goto nolexer;
	    }
	    segPtr = TkTextIndexToSeg(indexPtr, &offset);
	    if (segPtr->typePtr == &tkTextCharType) {
		char chBrace = segPtr->body.chars[offset];
		char styBrace = segPtr->body.chst.style[offset];
		int depth = 1;
		int direction;
		TkTextIndex index = *indexPtr;
		char chSeek = BraceOpposite(chBrace, &direction);

		if (chSeek == '\0')
		    break;
		styBrace = segPtr->body.chst.style[offset];
		while (TkTextIndexForwCharsExt(textPtr, &index, &segPtr, &offset, direction)) {
		    char chAtPos = segPtr->body.chars[offset];
		    char styAtPos = segPtr->body.chst.style[offset];
		    if (styAtPos == styBrace) {
			if (chAtPos == chBrace)
			    depth++;
			if (chAtPos == chSeek)
			    depth--;
			if (depth == 0) {
			    char buf[TK_POS_CHARS];
			    TkTextPrintIndex(textPtr, &index, buf);
			    Tcl_SetResult(interp, buf, TCL_VOLATILE);
			    break;
			}
		    }
		}
	    }
	    break;
	}
	case CMD_CGET: {
	    Tcl_Obj *resultObjPtr;

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "option");
		return TCL_ERROR;
	    }
	    if (sharedTextPtr->lexer == NULL) {
		goto nolexer;
	    }
	    resultObjPtr = Tk_GetOptionValue(interp,
		(char *) sharedTextPtr->lexer,
		sharedTextPtr->lexer->lm->optionTable, objv[3],
		textPtr->tkwin);
	    if (resultObjPtr == NULL)
		return TCL_ERROR;
	    Tcl_SetObjResult(interp, resultObjPtr);
	    break;
	}
	case CMD_CONFIGURE: {
	    result = LexerConfigureCmd(textPtr, interp, objc, objv);
	    break;
	}
	case CMD_INVOKE: {
	    CONST TkTextIndex *indexPtr;
	    int start = 0, end = BTREE_NUMLINES(textPtr);

	    if (objc > 5) {
		Tcl_WrongNumArgs(interp, 3, objv, "?start? ?end?");
		result = TCL_ERROR;
		break;
	    }
	    if (objc > 3) {
		indexPtr = TkTextGetIndexFromObj(interp, textPtr, objv[3]);
		if (indexPtr == NULL) {
		    result = TCL_ERROR;
		    break;
		}
		start = BTREE_LINESTO(textPtr, indexPtr->linePtr);
	    }
	    if (objc > 4) {
		indexPtr = TkTextGetIndexFromObj(interp, textPtr, objv[3]);
		if (indexPtr == NULL) {
		    result = TCL_ERROR;
		    break;
		}
		end = BTREE_LINESTO(textPtr, indexPtr->linePtr);
	    }
	    if (start > end) {
		Tcl_AppendResult(interp,
		    "start must be less than or equal to end", NULL);
		result = TCL_ERROR;
		break;
	    }
	    if (sharedTextPtr->lexer == NULL) {
		goto nolexer;
	    }
	    LexAndFold(sharedTextPtr, start, end - start + 1);
	    break;
	}
	case CMD_KEYWORDS: {
	    int numWords;
	    CONST char **words;
	    int index;

	    if (objc < 4 || objc > 5) {
		Tcl_WrongNumArgs(interp, 3, objv, "index ?list?");
		result = TCL_ERROR;
		break;
	    }
	    if (Tcl_GetIntFromObj(interp, objv[3], &index) != TCL_OK) {
		result = TCL_ERROR;
		break;
	    }
	    if (index < 1 || index > NUM_WORD_LISTS) {
		Tcl_AppendResult(interp, "bad index \"", Tcl_GetString(objv[3]),
		    "\": must be from 1 to 9", NULL);
		result = TCL_ERROR;
		break;
	    }
	    if (sharedTextPtr->lexer == NULL) {
		goto nolexer;
	    }
	    if (objc == 4) {
		/* Return list of keywords */
		Tcl_Obj *listObj;
		WordList *wl = sharedTextPtr->lexer->wordListPtrs[index - 1];

		listObj = Tcl_NewListObj(0, NULL);
		for (i = 0; i < wl->numWords; i++) {
		    Tcl_ListObjAppendElement(interp, listObj,
			Tcl_NewStringObj(wl->words[i], -1));
		}
		Tcl_SetObjResult(interp, listObj);
		break;
	    }
	    /* Perfect! A NULL-terminated list of words */
	    if (Tcl_SplitList(interp, Tcl_GetString(objv[4]), &numWords, &words) != TCL_OK) {
		result = TCL_ERROR;
		break;
	    }
	    WordList_Set(sharedTextPtr->lexer->wordListPtrs[index - 1], words);
	    EventuallyLexAndFold(sharedTextPtr, 0, BTREE_NUMLINES(textPtr));
	    break;
	}
	case CMD_NAMES: {
	    Tcl_Obj *listObj;
	    LexerModule *lm;

	    listObj = Tcl_NewListObj(0, NULL);
	    for (lm = LexerModule_Head(interp); lm != NULL; lm = lm->next) {
		Tcl_ListObjAppendElement(interp, listObj,
		    Tcl_NewStringObj(lm->name, -1));
	    }
	    Tcl_SetObjResult(interp, listObj);
	    break;
	}
	case CMD_SET: {
	    LexerModule *lm;
	    Lexer *lexer;
	    CONST char *s;

	    if (objc < 3 || objc > 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "?name?\"");
		result = TCL_ERROR;
		break;
	    }
	    if (objc == 3) {
		Tcl_SetResult(interp, sharedTextPtr->lexer ?
			(char *) sharedTextPtr->lexer->lm->name : "",
			TCL_VOLATILE);
		break;
	    }
	    s = Tcl_GetString(objv[3]);
	    if (!s[0]) {
		if (sharedTextPtr->lexer != NULL) {
		    Lexer_FreeInstance(textPtr, sharedTextPtr->lexer);
		    sharedTextPtr->lexer = NULL;
		    RemoveStyle(textPtr);
		}
		break;
	    }
	    lm = LexerModule_Find(textPtr->interp, s);
	    if (lm == NULL) {
		LexerModule *lmHead = LexerModule_Head(interp);
		int count;

		Tcl_AppendResult(interp, "unknown lexer \"", s,
		    "\": must be ", lmHead->name, NULL);
		for (lm = lmHead->next, count = 0;
			lm != NULL; lm = lm->next, count++) {
		    if (lm->next == NULL) {
			Tcl_AppendResult(interp, (count > 0) ?
				", or " : " or ", lm->name, NULL);
		    } else {
			Tcl_AppendResult(interp, ", ", lm->name, NULL);
		    }
		}
		result = TCL_ERROR;
		break;
	    }
	    if (sharedTextPtr->lexer != NULL) {
		Lexer_FreeInstance(textPtr, sharedTextPtr->lexer);
	    }
	    lexer = Lexer_NewInstance(textPtr, lm);
	    if (lexer == NULL) {
		result = TCL_ERROR;
	    }
	    sharedTextPtr->lexer = lexer;
	    EventuallyLexAndFold(sharedTextPtr, 0, BTREE_NUMLINES(textPtr));
	    break;
	}
	case CMD_STYLEAT: {
	    CONST TkTextIndex *indexPtr;
	    int offset;
	    TkTextSegment *segPtr;

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "index");
		result = TCL_ERROR;
		break;
	    }
	    indexPtr = TkTextGetIndexFromObj(interp, textPtr, objv[3]);
	    if (indexPtr == NULL) {
		result = TCL_ERROR;
		break;
	    }
	    if (sharedTextPtr->lexer == NULL) {
		goto nolexer;
	    }
	    /* FIXME: if LEX_NEEDED ... */
	    segPtr = TkTextIndexToSeg(indexPtr, &offset);
	    if (segPtr->typePtr == &tkTextCharType) {
		int style = segPtr->body.chst.style[offset];
		if (style != NO_STYLE) {
		    Tcl_SetStringObj(Tcl_GetObjResult(interp),
			    sharedTextPtr->lexer->tags[style]->name, -1);
		}
	    }
	    break;
	}
	case CMD_STYLENAMES: {
	    Tcl_Obj *listObj;

	    if (sharedTextPtr->lexer == NULL) {
		goto nolexer;
	    }
	    listObj = Tcl_NewListObj(0, NULL);
	    for (i = 0; i < sharedTextPtr->lexer->numStyles; i++) {
		Tcl_ListObjAppendElement(interp, listObj,
		    Tcl_NewStringObj(sharedTextPtr->lexer->lm->styleNames[i],
			    -1));
	    }
	    Tcl_SetObjResult(interp, listObj);
	    break;
	}
    }

    Tcl_Release((ClientData) textPtr);
    return result;

nolexer:
    Tcl_AppendResult(interp, "lexer is unspecified", NULL);
    Tcl_Release((ClientData) textPtr);
    return TCL_ERROR;
}

TkTextLine *
GetLastChild(
    TkSharedText *sharedPtr,
    TkTextLine *linePtr)
{
    int level = GetLineFoldDepth(sharedPtr, linePtr);
    TkTextLine *line2Ptr;

    for (line2Ptr = BTREE_NEXTLINE(NULL, linePtr);
	    line2Ptr != NULL;
	    line2Ptr = BTREE_NEXTLINE(NULL, line2Ptr)) {
	if (GetLineFoldDepth(sharedPtr, line2Ptr) <= level) {
	    break;
	}
	linePtr = line2Ptr;
    }
    return linePtr;
}

TkTextLine *
GetFoldParent(
    TkSharedText *sharedPtr,
    TkTextLine *linePtr)
{
    int level = GetLineFoldDepth(sharedPtr, linePtr);
    TkTextLine *line2Ptr;

    for (line2Ptr = BTREE_PREVLINE(NULL, linePtr);
	    line2Ptr != NULL;
	    line2Ptr = BTREE_PREVLINE(NULL, line2Ptr)) {
	if (GetLineFoldable(sharedPtr, line2Ptr) &&
		GetLineFoldDepth(sharedPtr, line2Ptr) < level) {
	    return line2Ptr;
	}
    }
    return NULL;
}

static TkTextLine *
Expand(
    TkText *textPtr,
    TkTextLine *linePtr,
    bool doExpand)
{
    TkSharedText *sharedPtr = textPtr->sharedTextPtr;
    TkTextLine *last = GetLastChild(sharedPtr, linePtr);
    int lineMaxSubord = BTREE_LINESTO(textPtr, last);
    TkTextLine *next = BTREE_NEXTLINE(textPtr,linePtr);

    if (linePtr == last)
	return next; /* no children */

    linePtr = next;
    while (BTREE_LINESTO(textPtr, linePtr) <= lineMaxSubord) {
	if (doExpand) {
	    SetLineVisible(textPtr, linePtr, true);
	}
	if (GetLineFoldable(sharedPtr, linePtr)) {
	    if (doExpand && !GetLineFolded(textPtr, linePtr)) {
		linePtr = Expand(textPtr, linePtr, true);
	    } else {
		linePtr = Expand(textPtr, linePtr, false);
	    }
	} else {
	    linePtr = BTREE_NEXTLINE(textPtr, linePtr);
	}
	if (linePtr == NULL)
	    break;
    }
    return BTREE_NEXTLINE(textPtr, last);
}

static void
EnsureLineVisible(
    TkText *textPtr,
    TkTextLine *linePtr)
{
    TkSharedText *sharedPtr = textPtr->sharedTextPtr;

    if (!GetLineVisible(textPtr, linePtr)) {
	TkTextLine *parent = GetFoldParent(sharedPtr, linePtr);
	if (parent != NULL) {
	    EnsureLineVisible(textPtr, parent);
	    if (GetLineFolded(textPtr, parent)) {
		SetLineFolded(textPtr, parent, false);
		Expand(textPtr, parent, true);
	    }
	}
    }
}

void
ToggleContraction(
    TkText *textPtr,
    TkTextLine *linePtr)
{
    TkSharedText *sharedPtr = textPtr->sharedTextPtr;
    TkTextLine *line2Ptr;

    if (!GetLineFoldable(sharedPtr, linePtr)) {
	linePtr = GetFoldParent(sharedPtr, linePtr);
	if (linePtr == NULL)
	    return;
    }
    if (!GetLineFolded(textPtr, linePtr)) {
	SetLineFolded(textPtr, linePtr, true);
	line2Ptr = GetLastChild(sharedPtr, linePtr);
	while (line2Ptr != linePtr) {
	    SetLineVisible(textPtr, line2Ptr, false);
	    line2Ptr = BTREE_PREVLINE(textPtr, line2Ptr);
	}
    } else {
	EnsureLineVisible(textPtr, linePtr);
	SetLineFolded(textPtr, linePtr, false);
	Expand(textPtr, linePtr, true);
    }
}

#ifdef STEXT_DEBUG
int AssertSegfault = 1;
void AssertFailed(const char *s, const char *file, int line)
{
    if (AssertSegfault) *(char *)0 = 0;
    Tcl_Panic("%s, %s:%d", s, file, line);
}
#endif

struct dbwinterps {
    int count;
#define DBWIN_MAX_INTERPS 16
    Tcl_Interp *interps[DBWIN_MAX_INTERPS];
};

static Tcl_ThreadDataKey dbwinTDK;
static CONST char *DBWIN_VAR_NAME = "dbwin";

static void dbwin_forget_interp(ClientData clientData, Tcl_Interp *interp)
{
    struct dbwinterps *dbwinterps =
	    Tcl_GetThreadData(&dbwinTDK, sizeof(struct dbwinterps));
    int i;

    for (i = 0; i < dbwinterps->count; i++) {
	if (dbwinterps->interps[i] == interp) {
	    for (; i < dbwinterps->count - 1; i++) {
		dbwinterps->interps[i] = dbwinterps->interps[i + 1];
	    }
	    dbwinterps->count--;
	    break;
	}
    }
}

void dbwin_add_interp(Tcl_Interp *interp)
{
    struct dbwinterps *dbwinterps =
	    Tcl_GetThreadData(&dbwinTDK, sizeof(struct dbwinterps));

    if (dbwinterps->count < DBWIN_MAX_INTERPS) {
	dbwinterps->interps[dbwinterps->count++] = interp;

	Tcl_SetAssocData(interp,
	    DBWIN_VAR_NAME,
	    dbwin_forget_interp,
	    NULL);
    }
}

void dbwin(char *fmt, ...)
{
    struct dbwinterps *dbwinterps =
	    Tcl_GetThreadData(&dbwinTDK, sizeof(struct dbwinterps));
    char buf[512];
    va_list args;
    int i;

    if (dbwinterps->count <= 0)
	return;

    va_start(args, fmt);
    vsnprintf(buf, 512, fmt, args);
    va_end(args);

    buf[511] = '\0';

    for (i = 0; i < dbwinterps->count; i++) {
	/* All sorts of nasty stuff could happen here. */
	Tcl_SetVar2(dbwinterps->interps[i],
	    DBWIN_VAR_NAME,
	    NULL,
	    buf,
	    TCL_GLOBAL_ONLY);
    }
}

