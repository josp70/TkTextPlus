/*
 * tkTextMargin.c --
 *
 *	This file contains code that manages the line number, fold and
 *	marker margins, an addition to the Tk Text widget.
 *
 * Copyright (c) 2006-2007 Tim Baker
 *
 * RCS: @(#) $Id$
 */

#include "tkText.h"

struct TkTextMargin
{
    CONST char *name;
    Tk_OptionTable optionTable;
    int x;
    TkTextMargin *nextPtr;
    GC activeFgGC;		/* font, color, linewidth=1 */
    GC fgGC;			/* font, color, linewidth=1 */
    int useWidth;		/* Auto width if widthObjPtr == NULL */
    int flags;			/* LINE_FLAG_MARKER1-4 */
    int markerIndex;		/* 0, 1, 2, 3 for marker margins */

    void (*displayProc)(TkText *, TkTextMargin *, TkTextLine *, Pixmap, int,
	    int, int);
    void (*layoutProc)(TkText *, TkTextMargin *);

    XColor *activeFgColorPtr;	/* -activeforeground, may be NULL */
    XColor *activeBgColorPtr;	/* -activebackground, may be NULL */
    TkTextLine *activeLinePtr;	/* -activeline */
    XColor *fgColorPtr;		/* -foreground */
    XColor *bgColorPtr;		/* -background */
    XColor *leftColorPtr;	/* -leftedge */
    XColor *rightColorPtr;	/* -rightedge */
    int justify;		/* -justify */
    int padX;			/* -padx */
    int side;			/* -side */
    int visible;		/* -visible */
    Tcl_Obj *widthObjPtr;	/* -width, may be NULL. A negative
				 * number is interpreted as the number
				 * of characters to display. */
    int width;			/* -width */
};

static char *sideStrings[] = {
    "left", "right", NULL
};

enum { SIDE_LEFT, SIDE_RIGHT };

#define CONF_LAYOUT	0x0001
#define CONF_DISPLAY	0x0002
#define CONF_ACTIVE	0x0004
#define CONF_FG		0x0010
#define CONF_BG		0x0020

static int		LineCO_Set(ClientData clientData,
			    Tcl_Interp *interp, Tk_Window tkwin,
			    Tcl_Obj **value, char *recordPtr,
			    int internalOffset, char *oldInternalPtr,
			    int flags);
static Tcl_Obj*		LineCO_Get(ClientData clientData, Tk_Window tkwin,
			    char *recordPtr, int internalOffset);
static void		LineCO_Restore(ClientData clientData,
			    Tk_Window tkwin, char *internalPtr,
			    char *oldInternalPtr);

static Tk_ObjCustomOption lineCO = {
    "line",			/* name */
    LineCO_Set,			/* setProc */
    LineCO_Get,			/* getProc */
    LineCO_Restore,		/* restoreProc */
    NULL,			/* freeProc */
    0
};

static Tk_OptionSpec marginOptionSpecs[] = {
    {TK_OPTION_COLOR, "-activebackground", "activeBackground", "Background",
	NULL, -1, Tk_Offset(TkTextMargin, activeBgColorPtr),
	TK_OPTION_NULL_OK, (ClientData) NULL, CONF_DISPLAY | CONF_BG},
    {TK_OPTION_COLOR, "-activeforeground", "activeForeground", "Foreground",
	NULL, -1, Tk_Offset(TkTextMargin, activeFgColorPtr),
	TK_OPTION_NULL_OK, (ClientData) NULL, CONF_DISPLAY | CONF_FG},
    {TK_OPTION_CUSTOM, "-activeline", NULL, NULL,
	 NULL, -1, Tk_Offset(TkTextMargin, activeLinePtr), TK_OPTION_NULL_OK,
	 (ClientData) &lineCO, CONF_ACTIVE},
    {TK_OPTION_COLOR, "-background", "background", "Background",
	"gray90", -1, Tk_Offset(TkTextMargin, bgColorPtr), 0,
	(ClientData) NULL, CONF_DISPLAY | CONF_BG},
    {TK_OPTION_COLOR, "-foreground", "foreground", "Foreground",
	"black", -1, Tk_Offset(TkTextMargin, fgColorPtr), 0,
	(ClientData) NULL, CONF_DISPLAY | CONF_FG},
    {TK_OPTION_JUSTIFY, "-justify", NULL, NULL,
	"right", -1, Tk_Offset(TkTextMargin, justify), 0, 0, CONF_DISPLAY},
    {TK_OPTION_COLOR, "-leftedge", "background", "Background",
	NULL, -1, Tk_Offset(TkTextMargin, leftColorPtr), TK_OPTION_NULL_OK,
	(ClientData) NULL, CONF_DISPLAY},
    {TK_OPTION_PIXELS, "-padx", NULL, NULL,
	"3", -1, Tk_Offset(TkTextMargin, padX), 0, (ClientData) NULL,
	CONF_LAYOUT},
    {TK_OPTION_COLOR, "-rightedge", "background", "Background",
	NULL, -1, Tk_Offset(TkTextMargin, rightColorPtr), TK_OPTION_NULL_OK,
	(ClientData) NULL, CONF_DISPLAY},
    {TK_OPTION_STRING_TABLE, "-side", NULL, NULL,
	"left", -1, Tk_Offset(TkTextMargin, side),
	0, (ClientData) sideStrings, CONF_LAYOUT},
    {TK_OPTION_BOOLEAN, "-visible", NULL, NULL,
	"0", -1, Tk_Offset(TkTextMargin, visible), 0, (ClientData) NULL,
	CONF_LAYOUT},
    {TK_OPTION_PIXELS, "-width", NULL, NULL,
	NULL, Tk_Offset(TkTextMargin, widthObjPtr),
	Tk_Offset(TkTextMargin, width), TK_OPTION_NULL_OK, (ClientData) NULL,
	CONF_LAYOUT},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0, 0, 0}
};

static void
OrderMargins(
    TkText *textPtr)		/* Information about text widget. */
{
    TkTextMargin *leftPtr = NULL, *rightPtr = NULL;
    int i;

    textPtr->leftMargins = textPtr->rightMargins = NULL;

    for (i = 0; i < 6; i++) {
	TkTextMargin *marginPtr = textPtr->marginOrder[i];
	if (marginPtr->side == SIDE_LEFT) {
	    if (leftPtr == NULL)
		textPtr->leftMargins = marginPtr;
	    else
		leftPtr->nextPtr = marginPtr;
	    marginPtr->nextPtr = NULL;
	    leftPtr = marginPtr;
	} else {
	    if (rightPtr == NULL)
		textPtr->rightMargins = marginPtr;
	    else
		rightPtr->nextPtr = marginPtr;
	    marginPtr->nextPtr = NULL;
	    rightPtr = marginPtr;
	}
    }
}

static int
MarginConfigure(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr,	/* Information about margin. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[],	/* Argument objects. */
    int creating)
{
    Tk_SavedOptions savedOptions;
    int error;
    Tcl_Obj *errorResult = NULL;
    int mask;
    TkTextLine *activeLinePtr = marginPtr->activeLinePtr;
    XGCValues gcValues;
    unsigned long gcMask;
    int autosize = !creating && (marginPtr->widthObjPtr == NULL);

    for (error = 0; error <= 1; error++) {
	if (error == 0) {
	    if (Tk_SetOptions(textPtr->interp, (char *) marginPtr,
		    marginPtr->optionTable,
		    objc, objv, textPtr->tkwin, &savedOptions, &mask)
		    != TCL_OK) {
		mask = 0;
		continue;
	    }

		/* xxx */

	    Tk_FreeSavedOptions(&savedOptions);
	    break;
	} else {
	    errorResult = Tcl_GetObjResult(textPtr->interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);

		/* xxx */

	    Tcl_SetObjResult(textPtr->interp, errorResult);
	    Tcl_DecrRefCount(errorResult);
	    return TCL_ERROR;
	}
    }

    if (creating)
	mask |= CONF_FG;
    else {
#if defined(WIN32) || defined(MAC_OSX_TK)
	if (marginPtr->fgGC->font != Tk_FontId(textPtr->tkfont))
#else
	XGetGCValues(textPtr->display, marginPtr->fgGC, GCFont, &gcValues);
	if (gcValues.font != Tk_FontId(textPtr->tkfont))
#endif
	    mask |= CONF_FG;
    }

    if (mask & CONF_FG) {
	if (marginPtr->activeFgGC != None) {
	    Tk_FreeGC(textPtr->display, marginPtr->activeFgGC);
	    marginPtr->activeFgGC = None;
	}
	if (marginPtr->fgGC != None)
	    Tk_FreeGC(textPtr->display, marginPtr->fgGC);

	if (marginPtr->activeFgColorPtr != NULL) {
	    gcMask = GCFont | GCForeground | GCLineWidth;
	    gcValues.font = Tk_FontId(textPtr->tkfont);
	    gcValues.foreground = marginPtr->activeFgColorPtr->pixel;
	    gcValues.line_width = 1;
	    marginPtr->activeFgGC = Tk_GetGC(textPtr->tkwin, gcMask, &gcValues);
	}

	gcMask = GCFont | GCForeground | GCLineWidth;
	gcValues.font = Tk_FontId(textPtr->tkfont);
	gcValues.foreground = marginPtr->fgColorPtr->pixel;
	gcValues.line_width = 1;
	marginPtr->fgGC = Tk_GetGC(textPtr->tkwin, gcMask, &gcValues);
    }

    if (marginPtr->flags & LINE_FLAG_MARKER1TO4) {
	if (autosize)
	    textPtr->autoSizeMarkerMarginCount--;
	if (marginPtr->widthObjPtr == NULL)
	    textPtr->autoSizeMarkerMarginCount++;
    }

    if (mask & CONF_LAYOUT) {
	OrderMargins(textPtr); /* in case -side changed */
	TkTextEventuallyRelayoutWindow(textPtr);
    } else if (mask & CONF_DISPLAY) {
	TkTextEventuallyRedrawMargin(textPtr);
    } else if (mask & CONF_ACTIVE) {
	if (activeLinePtr != marginPtr->activeLinePtr) {
	    if (activeLinePtr != NULL) {
		TkTextEventuallyRedrawMarginLine(textPtr, activeLinePtr);
	    }
	    if (marginPtr->activeLinePtr != NULL) {
		TkTextEventuallyRedrawMarginLine(textPtr, marginPtr->activeLinePtr);
	    }
	}
    }

    return TCL_OK;
}

static void
MarginFree(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr)	/* Information about margin. */
{
    Tk_FreeConfigOptions((char *) marginPtr, marginPtr->optionTable,
	    textPtr->tkwin);
    if (marginPtr->activeFgGC != None)
	Tk_FreeGC(textPtr->display, marginPtr->activeFgGC);
    if (marginPtr->fgGC != None)
	Tk_FreeGC(textPtr->display, marginPtr->fgGC);
    ckfree((char *) marginPtr);
}

#define SC_MARKNUM_FOLDEREND 25
#define SC_MARKNUM_FOLDEROPENMID 26
#define SC_MARKNUM_FOLDERMIDTAIL 27
#define SC_MARKNUM_FOLDERTAIL 28
#define SC_MARKNUM_FOLDERSUB 29
#define SC_MARKNUM_FOLDER 30
#define SC_MARKNUM_FOLDEROPEN 31

/* FIXME: redeclared from tkTextDisp.c */
#define DLINE_FIRST	0x100	/* This is the first DLine for a logical
				 * line. */
#define DLINE_LAST	0x200	/* This is the last DLine for a logical
				 * line. */

int
GetFoldMark(
    TkText *textPtr,
    TkTextLine *linePtr,
    int flags)
{
    bool needWhiteClosure = false; /* FIXME */
    bool firstSubLine = (flags & DLINE_FIRST) != 0; /* is this the first DLine for linePtr? */
    int level = linePtr->level;
    TkTextLine *nextPtr = BTREE_NEXTLINE(textPtr, linePtr);
    int levelNext = nextPtr ? nextPtr->level : SC_FOLDLEVELBASE;
    int levelNum = level & SC_FOLDLEVELNUMBERMASK;
    int levelNextNum = levelNext & SC_FOLDLEVELNUMBERMASK;
    int marks = 0;
#if 0
if (level & SC_FOLDLEVELWHITEFLAG)
{
    TkTextLine *prevPtr = BTREE_PREVLINE(textPtr, linePtr);
    if (prevPtr != NULL) {
	int levelPrev = prevPtr->level;
	int levelPrevNum = levelPrev & SC_FOLDLEVELNUMBERMASK;
	if (!(levelPrev & (SC_FOLDLEVELHEADERFLAG | SC_FOLDLEVELWHITEFLAG)) &&
		(levelPrevNum > levelNum))
	    needWhiteClosure = true;
    }
}
#endif
    if (level & SC_FOLDLEVELHEADERFLAG) {
	if (firstSubLine) {
	    if (!GetLineFolded(textPtr, linePtr)) {
		if (levelNum == SC_FOLDLEVELBASE)
		    marks |= 1 << SC_MARKNUM_FOLDEROPEN;
		else
		    marks |= 1 << SC_MARKNUM_FOLDEROPENMID;
	    } else {
		if (levelNum == SC_FOLDLEVELBASE)
		    marks |= 1 << SC_MARKNUM_FOLDER;
		else
		    marks |= 1 << SC_MARKNUM_FOLDEREND;
	    }
	} else {
if ((levelNum > SC_FOLDLEVELBASE) || !GetLineFolded(textPtr, linePtr))
	    marks |= 1 << SC_MARKNUM_FOLDERSUB;
	}
	needWhiteClosure = false;
    } else if (level & SC_FOLDLEVELWHITEFLAG) {
	if (needWhiteClosure) {
	    if (levelNext & SC_FOLDLEVELWHITEFLAG) {
		marks |= 1 << SC_MARKNUM_FOLDERSUB;
	    } else if (levelNum > SC_FOLDLEVELBASE) {
		marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
		needWhiteClosure = false;
	    } else {
		marks |= 1 << SC_MARKNUM_FOLDERTAIL;
		needWhiteClosure = false;
	    }
	} else if (levelNum > SC_FOLDLEVELBASE) {
	    if (levelNextNum < levelNum) {
		if (levelNextNum > SC_FOLDLEVELBASE) {
		    marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
		} else {
		    marks |= 1 << SC_MARKNUM_FOLDERTAIL;
		}
	    } else {
		marks |= 1 << SC_MARKNUM_FOLDERSUB;
	    }
	}
    } else if (levelNum > SC_FOLDLEVELBASE) {
	if (levelNextNum < levelNum) {
	    needWhiteClosure = false;
	    if (levelNext & SC_FOLDLEVELWHITEFLAG) {
#if 1
		if (levelNextNum > SC_FOLDLEVELBASE)
		    marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
		else
		    marks |= 1 << SC_MARKNUM_FOLDERTAIL;
#else
		marks |= 1 << SC_MARKNUM_FOLDERSUB;
#endif
		needWhiteClosure = true;
	    } else if (levelNextNum > SC_FOLDLEVELBASE) {
		marks |= 1 << SC_MARKNUM_FOLDERMIDTAIL;
	    } else {
		marks |= 1 << SC_MARKNUM_FOLDERTAIL;
	    }
	} else {
	    marks |= 1 << SC_MARKNUM_FOLDERSUB;
	}
    }

    return marks;
}

static void
DrawFoldButton(
    TkText *textPtr,
    TkTextLine *linePtr,
    Pixmap d,
    GC gc,
    int x,
    int y,
    int width,
    int height)
{
    Tk_Window tkwin = textPtr->tkwin;
    int w1, lineLeft, lineTop, buttonLeft, buttonTop, buttonThickness,
	buttonSize;
    GC buttonGC = gc;

    buttonSize = 9;
    buttonThickness = 1;

    w1 = buttonThickness / 2;

    /* Left edge of vertical line */
    lineLeft = x + (width - buttonThickness) / 2;

    /* Top edge of horizontal line */
    lineTop = y + (height - buttonThickness) / 2;

    buttonLeft = x + (width - buttonSize) / 2;
    buttonTop = y + (height - buttonSize) / 2;

    /* Erase button background */
    XFillRectangle(Tk_Display(tkwin), d,
	    Tk_3DBorderGC(tkwin, textPtr->border, TK_3D_FLAT_GC),
	    buttonLeft + buttonThickness,
	    buttonTop + buttonThickness,
	    buttonSize - buttonThickness,
	    buttonSize - buttonThickness);

    /* Draw button outline */
    XDrawRectangle(Tk_Display(tkwin), d, buttonGC,
	    buttonLeft + w1,
	    buttonTop + w1,
	    buttonSize - buttonThickness,
	    buttonSize - buttonThickness);

    /* Horizontal '-' */
    XFillRectangle(Tk_Display(tkwin), d, buttonGC,
	    buttonLeft + buttonThickness * 2,
	    lineTop,
	    buttonSize - buttonThickness * 4,
	    buttonThickness);

    if (GetLineFolded(textPtr, linePtr)) {
	/* Finish '+' */
	XFillRectangle(Tk_Display(tkwin), d, buttonGC,
		lineLeft,
		buttonTop + buttonThickness * 2,
		buttonThickness,
		buttonSize - buttonThickness * 4);
    }
}

static void
DisplayProcFold(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr,	/* Information about margin. */
    TkTextLine *linePtr,
    Pixmap pixmap,
    int height,
    int baseline,
    int flags)
{
    Display *display = Tk_Display(textPtr->tkwin);
    GC gc;
    int mark, lineLeft, lineTop, buttonLeft, buttonTop, buttonSize = 9,
	buttonThickness = 1;
    int marginLeft = marginPtr->x, marginWidth = marginPtr->useWidth;

    mark = GetFoldMark(textPtr, linePtr, flags);
    if (mark == 0)
	return;

    if (GetLineFoldHighlight(textPtr, linePtr) &&
	    (marginPtr->activeFgGC != None))
	gc = marginPtr->activeFgGC;
    else
	gc = marginPtr->fgGC;

    if (mark & (1<<SC_MARKNUM_FOLDER)) {
	DrawFoldButton(textPtr, linePtr, pixmap, gc, marginLeft, 0,
		marginWidth, height);
    }

    /* Left edge of vertical line */
    lineLeft = marginLeft + (marginWidth - buttonThickness) / 2;

    /* Top edge of horizontal line */
    lineTop = 0 + (height - buttonThickness) / 2;

    buttonLeft = marginLeft + (marginWidth - buttonSize) / 2;
    buttonTop = 0 + (height - buttonSize) / 2;

    /* SC_MARK_BOXMINUS + connected to next line */
    if (mark & (1<<SC_MARKNUM_FOLDEROPEN)) {

	/* Vertical '|' from middle to bottom. */
	XFillRectangle(display, pixmap, gc,
		lineLeft,
		lineTop,
		buttonThickness,
		height - lineTop);

	DrawFoldButton(textPtr, linePtr, pixmap, gc, marginLeft, 0,
		marginWidth, height);
    } 

    /* SC_MARK_BOXMINUSCONNECTED */
    if (mark & (1<<SC_MARKNUM_FOLDEROPENMID)) {

	/* Vertical '|' from top to bottom. */
	XFillRectangle(display, pixmap, gc,
		lineLeft,
		0,
		buttonThickness,
		height);

	DrawFoldButton(textPtr, linePtr, pixmap, gc, marginLeft, 0,
		marginWidth, height);
    } 

    /* SC_MARK_BOXPLUSCONNECTED */
    if (mark & (1<<SC_MARKNUM_FOLDEREND)) {

	/* Vertical '|' from top to bottom. */
	XFillRectangle(display, pixmap, gc,
		lineLeft,
		0,
		buttonThickness,
		height);

	DrawFoldButton(textPtr, linePtr, pixmap, gc, marginLeft, 0,
		marginWidth, height);
    } 

    /* SC_MARK_VLINE */
    if (mark & (1<<SC_MARKNUM_FOLDERSUB)) {

	/* Vertical '|' from top to bottom. */
	XFillRectangle(display, pixmap, gc,
		lineLeft,
		0,
		buttonThickness,
		height);
    } 

    /* SC_MARK_TCORNER */
    if (mark & (1<<SC_MARKNUM_FOLDERMIDTAIL)) {

	/* Vertical '|' from top to bottom. */
	XFillRectangle(display, pixmap, gc,
		lineLeft,
		0,
		buttonThickness,
		height);

	/* Horizontal '-' from middle to right. */
	XFillRectangle(display, pixmap, gc,
		lineLeft,
		lineTop,
		buttonLeft + buttonSize - lineLeft,
		buttonThickness);
    } 

    /* SC_MARK_LCORNER */
    if (mark & (1<<SC_MARKNUM_FOLDERTAIL)) {

	/* Vertical '|' from top to middle. */
	XFillRectangle(display, pixmap, gc,
		lineLeft,
		0,
		buttonThickness,
		lineTop - 0);

	/* Horizontal '-' from middle to right. */
	XFillRectangle(display, pixmap, gc,
		lineLeft,
		lineTop,
		buttonLeft + buttonSize - lineLeft,
		buttonThickness);
    }
}

static void
DisplayProcNumber(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr,	/* Information about margin. */
    TkTextLine *linePtr,
    Pixmap pixmap,
    int height,
    int baseline,
    int flags)
{
    Display *display = Tk_Display(textPtr->tkwin);
    GC gc;
    char buf[24];
    int len, textWidth, lineNumber = 0, textLeft = 0;
    int marginLeft = marginPtr->x, marginWidth = marginPtr->useWidth;

    if (!(flags & DLINE_FIRST))
	return;

    if (textPtr->lineNumbers == LINE_NUMBERS_DOCUMENT)
	lineNumber = BTREE_LINESTO(NULL, linePtr) + 1;
    else if (textPtr->lineNumbers == LINE_NUMBERS_RELATIVE)
	lineNumber = PEER_LINESTO(textPtr, linePtr) + 1;

    if ((linePtr == marginPtr->activeLinePtr) &&
	    (marginPtr->activeFgGC != None))
	gc = marginPtr->activeFgGC;
    else
	gc = marginPtr->fgGC;

    len = sprintf(buf, "%d", lineNumber);
    textWidth = Tk_TextWidth(textPtr->tkfont, buf, len);
    switch (marginPtr->justify) {
	case TK_JUSTIFY_LEFT:
	    textLeft = marginLeft + marginPtr->padX;
	    break;
	case TK_JUSTIFY_CENTER:
	    textLeft = marginLeft + (marginWidth - textWidth) / 2;
	    break;
	case TK_JUSTIFY_RIGHT:
	    textLeft = marginLeft + marginWidth - marginPtr->padX - textWidth;
	    break;
    }
    Tk_DrawChars(display, pixmap, gc, textPtr->tkfont,
	    buf, len, textLeft, baseline);
}

static void
DisplayProcSymbol(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr,	/* Information about margin. */
    TkTextLine *linePtr,
    Pixmap pixmap,
    int height,
    int baseline,
    int flags)
{
    int ref = textPtr->pixelReference;
    GC gc;

    if (!(flags & DLINE_FIRST))
	return;

    if (linePtr->peerData[ref].flags & marginPtr->flags) {
	Tcl_HashEntry *hPtr = Tcl_FindHashEntry(
		&textPtr->markerInMarginHash[marginPtr->markerIndex],
		(char *) linePtr);
	TkTextLineMarker *marker;

	ASSERT(hPtr != NULL);
	marker = Tcl_GetHashValue(hPtr);

	if ((linePtr == marginPtr->activeLinePtr) &&
		(marginPtr->activeFgGC != None))
	    gc = marginPtr->activeFgGC;
	else
	    gc = marginPtr->fgGC;

	TkTextLineMarkerDraw(textPtr, marker, linePtr, pixmap, gc,
	    marginPtr->justify, marginPtr->padX,
	    marginPtr->x, marginPtr->useWidth, height, baseline);
    }
}

static void
LayoutProcFold(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr)	/* Information about margin. */
{
    if (marginPtr->widthObjPtr == NULL) {
	int buttonSize = 9;
	marginPtr->useWidth = buttonSize + marginPtr->padX * 2;
    } else if (marginPtr->width < 0) {
	marginPtr->useWidth = textPtr->charWidth * (-marginPtr->width)
	    + marginPtr->padX * 2;
    } else {
	marginPtr->useWidth = marginPtr->width;
    }
}

static void
LayoutProcNumber(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr)	/* Information about margin. */
{
    if (marginPtr->widthObjPtr == NULL) {
	int lineCount = PEER_NUMLINES(textPtr);
	int charCount = 1;

	if (textPtr->lineNumbers == LINE_NUMBERS_DOCUMENT)
	    lineCount = BTREE_NUMLINES(textPtr);
	while (lineCount >= 10) {
	    lineCount /= 10;
	    ++charCount;
	}
	marginPtr->useWidth = textPtr->charWidth * charCount
	    + marginPtr->padX * 2;
    } else if (marginPtr->width < 0) {
	marginPtr->useWidth = textPtr->charWidth * (-marginPtr->width)
	    + marginPtr->padX * 2;
    } else {
	marginPtr->useWidth = marginPtr->width;
    }
}

static void
LayoutProcSymbol(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr)	/* Information about margin. */
{
    if (marginPtr->widthObjPtr == NULL) {
	marginPtr->useWidth = TkTextLineMarkerMaxWidth(textPtr) +
	    marginPtr->padX * 2;
    } else if (marginPtr->width < 0) {
	marginPtr->useWidth = textPtr->charWidth * (-marginPtr->width)
	    + marginPtr->padX * 2;
    } else {
	marginPtr->useWidth = marginPtr->width;
    }
}

int
TkTextGetMargin(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Obj *objPtr,		/* Margin name object. */
    TkTextMargin **marginPtrPtr)/* Returned margin. */
{
    static CONST char *marginNames[] = {
	"number", "fold", "marker1", "marker2", "marker3", "marker4", NULL
    };
    int index;
    TkTextMargin *marginPtrs[6];

    if (Tcl_GetIndexFromObj(textPtr->interp, objPtr, marginNames,
	    "margin name", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }

    marginPtrs[0] = textPtr->marginN;
    marginPtrs[1] = textPtr->marginF;
    marginPtrs[2] = textPtr->margin1;
    marginPtrs[3] = textPtr->margin2;
    marginPtrs[4] = textPtr->margin3;
    marginPtrs[5] = textPtr->margin4;

    *marginPtrPtr = marginPtrs[index];
    return TCL_OK;
}

int
TkTextMarginCmd(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[])	/* Argument objects. */
{
    static CONST char *optionStrings[] = {
	"cget", "configure", "names", "order", NULL
    };
    enum {
	MARG_CGET, MARG_CONFIGURE, MARG_NAMES, MARG_ORDER
    };
    int index;
    TkTextMargin *marginPtr;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], optionStrings,
	    "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    switch (index) {
	case MARG_CGET: {
	    Tcl_Obj *resultObjPtr;

	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 3, objv, "margin option");
		return TCL_ERROR;
	    }
	    if (TkTextGetMargin(textPtr, objv[3], &marginPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    resultObjPtr = Tk_GetOptionValue(interp, (char *) marginPtr,
		    marginPtr->optionTable, objv[4], textPtr->tkwin);
	    if (resultObjPtr == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, resultObjPtr);
	    break;
	}
	case MARG_CONFIGURE: {
	    Tcl_Obj *resultObjPtr;

	    if (objc < 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "margin ?option value ...?");
		return TCL_ERROR;
	    }
	    if (TkTextGetMargin(textPtr, objv[3], &marginPtr) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (objc <= 5) {
		resultObjPtr = Tk_GetOptionInfo(interp, (char *) marginPtr,
			marginPtr->optionTable, (objc == 5) ? objv[4] : NULL,
			textPtr->tkwin);
		if (resultObjPtr == NULL) {
		    return TCL_ERROR;
		}
		Tcl_SetObjResult(interp, resultObjPtr);
		break;
	    }
	    return MarginConfigure(textPtr, marginPtr, objc - 4, objv + 4, 0);
	}
	case MARG_NAMES: {
	    Tcl_SetResult(interp, "fold number marker1 marker2 marker3 marker4",
		TCL_STATIC);
	    break;
	}
	case MARG_ORDER: {
	    int i, j, order = 0, listObjc;
	    Tcl_Obj **listObjv;
	    TkTextMargin *marginPtrs[6];

	    if (objc > 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "?list?");
		return TCL_ERROR;
	    }
	    if (objc == 3) {
		for (i = 0; i < 6; i++) {
		    Tcl_AppendElement(interp, textPtr->marginOrder[i]->name);
		}
		break;
	    }
	    if (Tcl_ListObjGetElements(interp, objv[3], &listObjc, &listObjv)
		    != TCL_OK) {
		return TCL_ERROR;
	    }

	    marginPtrs[0] = textPtr->marginN;
	    marginPtrs[1] = textPtr->marginF;
	    marginPtrs[2] = textPtr->margin1;
	    marginPtrs[3] = textPtr->margin2;
	    marginPtrs[4] = textPtr->margin3;
	    marginPtrs[5] = textPtr->margin4;

	    for (i = 0; i < listObjc; i++) {
		if (TkTextGetMargin(textPtr, listObjv[i], &marginPtr)
			!= TCL_OK) {
		    return TCL_ERROR;
		}
		for (j = 0; j < 6; j++) {
		    if (marginPtrs[j] == marginPtr)
			break;
		}
		if (j < 6) {
		    textPtr->marginOrder[order++] = marginPtr;
		    marginPtrs[j] = NULL;
		}
	    }
	    for (j = 0; j < 6; j++) {
		marginPtr = marginPtrs[j];
		if (marginPtr != NULL) {
		    textPtr->marginOrder[order++] = marginPtr;
		}
	    }
	    /* Sort by left/right side. */
	    OrderMargins(textPtr);
	    /* FIXME: TkTextEventuallyRedrawMargin() is all that's needed but
	     * I need to set marginPtr->x again. */
	    TkTextEventuallyRelayoutWindow(textPtr);
	    break;
	}
    }
    return TCL_OK;
}

void
TkTextRelayoutMargins(
    TkText *textPtr)		/* Information about text widget. */
{
    TkTextMargin *marginPtr;
    int borders = textPtr->highlightWidth + textPtr->borderWidth;
    int left = borders;
    int right = Tk_Width(textPtr->tkwin) - borders;

    textPtr->lineMarkerMaxWidth = -1; /* in case the font changed */

    for (marginPtr = textPtr->leftMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	/* Get graphics contexts if textPtr->tkfont changed. */
	MarginConfigure(textPtr, marginPtr, 0, NULL, 0);
	(*marginPtr->layoutProc)(textPtr, marginPtr);
	if (marginPtr->visible) {
	    marginPtr->x = left;
	    left += marginPtr->useWidth;
	}
    }

    left = 0;
    for (marginPtr = textPtr->rightMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	/* Get graphics contexts if textPtr->tkfont changed. */
	MarginConfigure(textPtr, marginPtr, 0, NULL, 0);
	(*marginPtr->layoutProc)(textPtr, marginPtr);
	if (marginPtr->visible) {
	    marginPtr->x = left;
	    left += marginPtr->useWidth;
	}
    }
    for (marginPtr = textPtr->rightMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	if (marginPtr->visible) {
	    marginPtr->x += right - left;
	}
	
    }
}

void
TkTextGetMarginIndents(
    TkText *textPtr,		/* Information about text widget. */
    int *leftPtr,		/* Returned width of margins displayed on
				 * the left side of the window. */
    int *rightPtr)		/* Returned width of margins displayed on
				 * the right side of the window. */
{
    TkTextMargin *marginPtr;

    *leftPtr = *rightPtr = 0;

    for (marginPtr = textPtr->leftMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	if (marginPtr->visible) {
	    *leftPtr += marginPtr->useWidth;
	}
    }
    for (marginPtr = textPtr->rightMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	if (marginPtr->visible) {
	    *rightPtr += marginPtr->useWidth;
	}
    }
}

TkTextMargin *
TkTextMarginAtX(
    TkText *textPtr,		/* Information about text widget. */
    int x)
{
    TkTextMargin *marginPtr;

    for (marginPtr = textPtr->leftMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	if (marginPtr->visible && (x >= marginPtr->x) &&
		(x < marginPtr->x + marginPtr->useWidth)) {
	    return marginPtr;
	}
    }
    for (marginPtr = textPtr->rightMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	if (marginPtr->visible && (x >= marginPtr->x) &&
		(x < marginPtr->x + marginPtr->useWidth)) {
	    return marginPtr;
	}
    }
    return NULL;
}

CONST char *
TkTextMarginName(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr)	/* Information about margin. */
{
    return marginPtr->name;
}

int
TkTextMarginBounds(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr,	/* Information about margin. */
    int *leftPtr,
    int *widthPtr)
{
    if (marginPtr == NULL || !marginPtr->visible || marginPtr->useWidth <= 0)
	return 0;
    if (leftPtr) *leftPtr = marginPtr->x;
    if (widthPtr) *widthPtr = marginPtr->useWidth;
    return 1;
}

static void
MarginDraw(
    TkText *textPtr,		/* Information about text widget. */
    TkTextMargin *marginPtr,	/* Information about margin. */
    TkTextLine *linePtr,
    Pixmap pixmap,
    GC copyGC,
    int y,
    int height,
    int baseline,
    int flags)
{
    GC gc;
    int borders = textPtr->highlightWidth + textPtr->borderWidth;
    int minY = borders + textPtr->padY;
    int maxY = Tk_Height(textPtr->tkwin) - borders - textPtr->padY;
    int y_off;

    if ((linePtr == marginPtr->activeLinePtr) &&
	    (marginPtr->activeBgColorPtr != NULL))
	gc = Tk_GCForColor(marginPtr->activeBgColorPtr,
		Tk_WindowId(textPtr->tkwin));
    else
	gc = Tk_GCForColor(marginPtr->bgColorPtr,
		Tk_WindowId(textPtr->tkwin));
    XFillRectangle(textPtr->display, pixmap, gc,
	marginPtr->x, 0, (unsigned int) marginPtr->useWidth,
	(unsigned int) height);

    if (marginPtr->leftColorPtr != NULL) {
	gc = Tk_GCForColor(marginPtr->leftColorPtr,
		Tk_WindowId(textPtr->tkwin));
	XFillRectangle(textPtr->display, pixmap, gc,
	    marginPtr->x, 0, 1, (unsigned int) height);
    }
    if (marginPtr->rightColorPtr != NULL) {
	gc = Tk_GCForColor(marginPtr->rightColorPtr,
		Tk_WindowId(textPtr->tkwin));
	XFillRectangle(textPtr->display, pixmap, gc,
	    marginPtr->x + marginPtr->useWidth - 1, 0, 1,
	    (unsigned int) height);
    }

    (*marginPtr->displayProc)(textPtr, marginPtr, linePtr, pixmap, height,
	    baseline, flags);

    if ((y + height) > maxY) {
	height = maxY - y;
    }
    if (y < minY) {
	y_off = minY - y;
	height -= y_off;
    } else {
	y_off = 0;
    }

    XCopyArea(textPtr->display, pixmap, Tk_WindowId(textPtr->tkwin),
	    copyGC, marginPtr->x, y_off,
	    (unsigned int) marginPtr->useWidth, (unsigned int) height,
	    marginPtr->x, y + y_off);
}

void
TkTextDrawMargins(
    TkText *textPtr,		/* Information about text widget. */
    TkTextLine *linePtr,
    Pixmap pixmap,
    GC copyGC,
    int y,
    int height,
    int baseline,
    int flags)
{
    TkTextMargin *marginPtr;

    for (marginPtr = textPtr->leftMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	if (marginPtr->visible && marginPtr->useWidth > 0) {
	    MarginDraw(textPtr, marginPtr, linePtr, pixmap, copyGC, y,
		    height, baseline, flags);
	}
    }
    for (marginPtr = textPtr->rightMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	if (marginPtr->visible && marginPtr->useWidth > 0) {
	    MarginDraw(textPtr, marginPtr, linePtr, pixmap, copyGC, y,
		    height, baseline, flags);
	}
    }
}

void
TkTextMarginDeletion(
    TkText *textPtr)		/* Information about text widget. */
{
    TkTextMargin *marginPtr;

    for (marginPtr = textPtr->leftMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	marginPtr->activeLinePtr = NULL;
    }
    for (marginPtr = textPtr->rightMargins;
	    marginPtr != NULL;
	    marginPtr = marginPtr->nextPtr) {
	marginPtr->activeLinePtr = NULL;
    }
}

void
TkTextMarginLineCountChanged(
    TkText *textPtr,		/* Information about text widget. */
    int changeToLineCount)
{
    TkTextMargin *marginPtr = textPtr->marginN;

    if ((marginPtr != NULL) && marginPtr->visible &&
	    (marginPtr->widthObjPtr == NULL)) {
	int oldWidth = marginPtr->useWidth;
	LayoutProcNumber(textPtr, marginPtr);
	if (oldWidth != marginPtr->useWidth) {
dbwin("TkTextMarginLineCountChanged: %+d lines, relayout whole window", changeToLineCount);
	    TkTextEventuallyRelayoutWindow(textPtr);
	}
    }
}

static TkTextMargin *
MarginCreate(
    TkText *textPtr,		/* Information about text widget. */
    CONST char *name,
    int index)
{
    TkTextMargin *marginPtr;

    marginPtr = (TkTextMargin *) ckalloc(sizeof(TkTextMargin));
    memset(marginPtr, '\0', sizeof(TkTextMargin));
    marginPtr->name = name;
    marginPtr->optionTable = Tk_CreateOptionTable(textPtr->interp,
	    marginOptionSpecs);
    if (Tk_InitOptions(textPtr->interp, (char *) marginPtr,
	    marginPtr->optionTable, textPtr->tkwin) != TCL_OK) {
	MarginFree(textPtr, marginPtr);
	return NULL;
    }
    switch (index) {
	case 0:
	    marginPtr->displayProc = DisplayProcNumber;
	    marginPtr->layoutProc = LayoutProcNumber;
	    break;
	case 1:
	    marginPtr->displayProc = DisplayProcFold;
	    marginPtr->layoutProc = LayoutProcFold;
	    break;
	default:
	    marginPtr->displayProc = DisplayProcSymbol;
	    marginPtr->layoutProc = LayoutProcSymbol;
	    marginPtr->flags |= LINE_FLAG_MARKER1 << (index - 2);
	    marginPtr->markerIndex = (index - 2);
	    break;
    }
    (void) MarginConfigure(textPtr, marginPtr, 0, NULL, 1);
    (*marginPtr->layoutProc)(textPtr, marginPtr);
    return marginPtr;
}

void
TkTextInitMargins(
    TkText *textPtr)		/* Information about text widget. */
{
    int n = 0;

    textPtr->marginOrder[n++] = textPtr->marginN =
	MarginCreate(textPtr, "number", 0);
    textPtr->marginOrder[n++] = textPtr->marginF =
	MarginCreate(textPtr, "fold", 1);
    textPtr->marginOrder[n++] = textPtr->margin1 =
	MarginCreate(textPtr, "marker1", 2);
    textPtr->marginOrder[n++] = textPtr->margin2 =
	MarginCreate(textPtr, "marker2", 3);
    textPtr->marginOrder[n++] = textPtr->margin3 =
	MarginCreate(textPtr, "marker3", 4);
    textPtr->marginOrder[n++] = textPtr->margin4 =
	MarginCreate(textPtr, "marker4", 5);

    OrderMargins(textPtr);
}

void
TkTextFreeMargins(
    TkText *textPtr)		/* Information about text widget. */
{
    MarginFree(textPtr, textPtr->marginN);
    MarginFree(textPtr, textPtr->marginF);
    MarginFree(textPtr, textPtr->margin1);
    MarginFree(textPtr, textPtr->margin2);
    MarginFree(textPtr, textPtr->margin3);
    MarginFree(textPtr, textPtr->margin4);
}

static int
ObjectIsEmpty(objPtr)
    Tcl_Obj *objPtr;		/* Object to test. May be NULL. */
{
    int length;

    if (objPtr == NULL) {
	return 1;
    }
    if (objPtr->bytes != NULL) {
	return (objPtr->length == 0);
    }
    Tcl_GetStringFromObj(objPtr, &length);
    return (length == 0);
}

static int
LineCO_Set(clientData, interp, tkwin, value, recordPtr, internalOffset,
	oldInternalPtr, flags)
    ClientData clientData;
    Tcl_Interp *interp;		/* Current interp; may be used for errors. */
    Tk_Window tkwin;		/* Window for which option is being set. */
    Tcl_Obj **value;		/* Pointer to the pointer to the value object.
				 * We use a pointer to the pointer because we
				 * may need to return a value (NULL). */
    char *recordPtr;		/* Pointer to storage for the widget record. */
    int internalOffset;		/* Offset within *recordPtr at which the
				 * internal value is to be stored. */
    char *oldInternalPtr;	/* Pointer to storage for the old value. */
    int flags;			/* Flags for the option, set Tk_SetOptions. */
{
    TkTextLine *linePtr = NULL;
    char *internalPtr;
    TkText *textPtr = (TkText *) ((TkWindow *) tkwin)->instanceData;

    if (internalOffset >= 0) {
	internalPtr = recordPtr + internalOffset;
    } else {
	internalPtr = NULL;
    }

    if (flags & TK_OPTION_NULL_OK && ObjectIsEmpty(*value)) {
	*value = NULL;
    } else {
	int line;

	if (Tcl_GetIntFromObj(interp, *value, &line) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (line == BTREE_NUMLINES(textPtr) + 1) {
	    line--;
	}
	linePtr = TkBTreeFindLine(textPtr->sharedTextPtr->tree, NULL, line-1);
    }

    if (internalPtr != NULL) {
	*((TkTextLine **) oldInternalPtr) = *((TkTextLine **) internalPtr);
	*((TkTextLine **) internalPtr) = linePtr;
    }
    return TCL_OK;
}

static Tcl_Obj *
LineCO_Get(clientData, tkwin, recordPtr, internalOffset)
    ClientData clientData;
    Tk_Window tkwin;
    char *recordPtr;		/* Pointer to widget record. */
    int internalOffset;		/* Offset within *recordPtr containing the
				 * line value. */
{
    TkTextLine *linePtr = *(TkTextLine **)(recordPtr + internalOffset);

    if (linePtr == NULL) {
	return Tcl_NewObj();
    } else {
	return Tcl_NewIntObj(1+TkBTreeLinesTo(NULL, linePtr));
    }
}

static void
LineCO_Restore(clientData, tkwin, internalPtr, oldInternalPtr)
    ClientData clientData;
    Tk_Window tkwin;
    char *internalPtr;		/* Pointer to storage for value. */
    char *oldInternalPtr;	/* Pointer to old value. */
{
    *(TkTextLine **)internalPtr = *(TkTextLine **)oldInternalPtr;
}
