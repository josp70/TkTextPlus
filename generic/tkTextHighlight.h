/*
 * tkTextHighlight.h --
 *
 *	Declarations shared among the files that implement text widgets.
 *
 * Copyright (c) 2006-2007 Tim Baker
 *
 * RCS: @(#) $Id$
 */

#include <ctype.h>
#include <string.h>

typedef struct WordList WordList;
struct WordList
{
    const char **words;
    int *lengths;
    int numWords;
    bool sorted;
    int starts[256];
};
extern void WordList_WordList(WordList *wl);
extern void WordList_Clear(WordList *wl);
extern void WordList_Free(WordList *wl);
extern void WordList_Set(WordList *wl, const char **words);
extern bool WordList_InList(WordList *wl, const char *s, int len);

typedef struct Lexer Lexer;
typedef struct LexerModule LexerModule;
typedef struct LexerArgs LexerArgs;

struct LexerArgs
{
    Lexer *lexer;
    struct TkSharedText *sharedPtr;
    int firstLine;
    int lastLine;
    int linesInDocument;
    bool folding;
};

typedef int (*LexerFunction)(LexerArgs *args);

struct LexerModule
{
    CONST char *name;		/* Identifier ("cpp", "tcl" etc). */
    int size;			/* sizeof(Lexer) plus extra options. */
    CONST char **styleNames;	/* NULL-terminated list of style names.
				 * These become tag names. */
    LexerFunction fnLexer;	/* Lexing function. */
    LexerFunction fnFolder;	/* Folding function. */
    Tk_OptionSpec *optionSpecs;	/* Lexer-specific option specs. */

    Tk_OptionTable optionTable;	/* Lexer-specific option table. */
    LexerModule *next;		/* Linked list of defined lexers. */
};

struct Lexer
{
    LexerModule *lm;
#define NUM_WORD_LISTS 9
    WordList wordLists[NUM_WORD_LISTS];
    WordList *wordListPtrs[NUM_WORD_LISTS];
    int numStyles;
    TkTextTag **tags;
    int enable;
    char *braceStyleStr;
    int braceStyle;
    int startLine;
    int endLine;
};

struct LexVars
{
    TkSharedText *sharedPtr;

    TkTextLine *linePtr;
    TkTextLine *linePrevPtr;
    TkTextLine *lineNextPtr;
    TkTextLine *lineLastPtr;

    WordList **wordListPtrs;

    char chPrev;
    char chCur;
    char chNext;
    Tcl_DString charDString;
    char *charBuf;

    int lineIndex;
    int linesInDocument;

    int lineLength;
    int charIndex;
    int startIndex;

    bool atLineStart;
    bool atLineEnd;

    bool folding;

    struct {
	bool checking;
	int style;
	int level;
	int state;
    } changes;

    int stylePrev;
    int style;
    int styleNext;
    Tcl_DString styleDString;
    char *styleBuf;
};
extern struct LexVars vars;

#define More() \
    ((vars.linePtr != NULL) /*&& (vars.charIndex < vars.lineLength)*/)

#define AtStartOfDoc() \
    (vars.linePrevPtr == NULL && vars.atLineStart)

#define ToLineStart() \
    while (vars.charIndex > 0) \
	Back(); \
    vars.style = vars.stylePrev;

extern void Forward(void);
extern void Back(void);
extern void ForwardN(int n);
extern void JumpToEOL(void);

#define MatchCh(c) \
    (vars.chCur == c)

#define MatchChCh(c1,c2) \
    ((vars.chCur == c1) && (vars.chNext == c2))

extern bool MatchStr(const char *s);
extern bool MatchStrAt(int i, const char *s);
extern void Flush(void);
extern void StyleAhead(int count, int style);
extern int GetCurrent(char *s, int len);
extern bool MatchKeyword(char *s, int len, int *index);
extern bool IsEscaped(int offset);

extern void SetLineState(TkText *textPtr, TkTextLine *linePtr, int state);
extern int GetLineState(TkText *textPtr, TkTextLine *linePtr);

#define GetRelative(n) \
    SafeGetCharAt(vars.charIndex + n)

#define ChangeState(s) \
    vars.style = (s)

#define SetStyle(s) \
    Flush(); \
    vars.style = (s)

#define ForwardSetStyle(s) \
    Forward(); \
   SetStyle(s);

#define Complete() \
    Flush();

#define GetCharAt(i) \
    vars.charBuf[i]

#define SafeGetCharAt(i) \
    (((i) >= 0 && (i) < vars.lineLength) ? vars.charBuf[i] : ' ')

#define ColourTo(i,s) \
    vars.charIndex = (i) + 1; \
    vars.style = (s); \
    Flush(); \
    vars.startIndex = (i) + 1

#define IsASpace(ch) \
    ((ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d)))

#define IsASpaceOrTab(ch) \
    ((ch == ' ') || (ch == '\t'))

#define IsADigit(ch) \
    ((ch >= '0') && (ch <= '9'))

#ifndef isascii /* Mingw doesn't define this with -ansi. */
#define isascii(c) ((c & ~0x7F) == 0)
#endif

#define isoperator(ch) \
    ((!isascii(ch) || !isalnum(ch)) && (strchr("%^&*()-+=|{}[]:;<>,/?!.~", ch) != NULL))

/**
 * Check if a character is a space.
 * This is ASCII specific but is safe with chars >= 0x80.
 */
#define isspacechar(ch) \
    ((ch == ' ') || ((ch >= 0x09) && (ch <= 0x0d)))

#define iswordchar(ch) \
    (isascii(ch) && (isalnum(ch) || ch == '.' || ch == '_'))

#define iswordstart(ch) \
    (isascii(ch) && (isalnum(ch) || ch == '_'))

#define InList(w,s,n) \
    WordList_InList(w, s, n)

extern void BeginStyling(LexerArgs *args, int firstLine, int lastLine);
extern int FindStyleAtSOL(TkSharedText *sharedPtr, int lineIndex);
extern int FindStyleAtEOL(TkSharedText *sharedPtr, int lineIndex);
extern int GetFirstNonWSChar(TkTextLine *linePtr);

enum { wsSpace = 1, wsTab = 2, wsSpaceTab = 4, wsInconsistent=8};
typedef bool (*PFNIsCommentLeader)(char *buf, int pos, int len);
extern int IndentAmount(TkTextLine *linePtr, TkTextLine *linePrevPtr,
    int *flags, PFNIsCommentLeader pfnIsCommentLeader);

