/*
 * tkTextLineMarker.c --
 *
 *	This file contains code that manages line-markers displayed in the
 *	marker margins, an addition to the Tk Text widget.
 *
 * Copyright (c) 2006-2007 Tim Baker
 *
 * RCS: @(#) $Id$
 */

#include "tkText.h"

struct TkTextLineMarker
{
    char *name;			/* malloc'd */
    Tk_OptionTable optionTable;
    int refCount;		/* The number of times this marker is
				 * assigned to any line in any margin. */

    Tk_Image image;		/* -image */
    CONST char *imageString;	/* -image */
    Pixmap bitmap;		/* -bitmap */
    Tcl_Obj *textObj;		/* -text */
};

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_BITMAP, "-bitmap", "bitmap", "Bitmap",
	NULL, -1, Tk_Offset(TkTextLineMarker, bitmap),
	TK_OPTION_NULL_OK, (ClientData) NULL, 0},
    {TK_OPTION_STRING, "-image", "image", "Image",
	NULL, -1, Tk_Offset(TkTextLineMarker, imageString),
	TK_OPTION_NULL_OK, (ClientData) NULL, 0},
    {TK_OPTION_STRING, "-text", "text", "Text",
	NULL, Tk_Offset(TkTextLineMarker, textObj), -1,
	TK_OPTION_NULL_OK, (ClientData) NULL, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0, 0, 0}
};

static void
MarkerFree(
    TkText *textPtr,		/* Information about text widget. */
    TkTextLineMarker *marker)	/* Information about marker. */
{
    if (marker == NULL)
	return;
    Tk_FreeConfigOptions((char *) marker, marker->optionTable,
	    textPtr->tkwin);
    if (marker->image != None)
	Tk_FreeImage(marker->image);
    ckfree(marker->name);
    ckfree((char *) marker);
}

static void
MarkerImageProc(
    ClientData clientData,	/* Pointer to marker record. */
    int x, int y,		/* Upper left pixel (within image) that must
				 * be redisplayed. */
    int width, int height,	/* Dimensions of area to redisplay (may be
				 * <= 0). */
    int imgWidth, int imgHeight)/* New dimensions of image. */

{
    TkText *textPtr = clientData;

    textPtr->lineMarkerMaxWidth = -1;
    if (textPtr->autoSizeMarkerMarginCount > 0)
	TkTextEventuallyRelayoutWindow(textPtr);
}

static int
MarkerWidth(
    TkText *textPtr,		/* Information about text widget. */
    TkTextLineMarker *marker)	/* Information about marker. */
{
    if (marker->image != NULL) {
	int iw, ih;
	Tk_SizeOfImage(marker->image, &iw, &ih);
	return iw;
    }
    if (marker->bitmap != None) {
	int bw, bh;
	Tk_SizeOfBitmap(textPtr->display, marker->bitmap, &bw, &bh);
	return bw;
    }
    if (marker->textObj != NULL) {
	int bytes;
	CONST char *s = Tcl_GetStringFromObj(marker->textObj, &bytes);
	return Tk_TextWidth(textPtr->tkfont, s, bytes);
    }
    return 0;
}

static int
MarkerConfigure(
    TkText *textPtr,		/* Information about text widget. */
    TkTextLineMarker *marker,	/* Information about marker. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[])	/* Argument objects. */
{
    Tk_SavedOptions savedOptions;
    int error;
    Tcl_Obj *errorResult = NULL;
    int mask;
    Tk_Image image = NULL;

    for (error = 0; error <= 1; error++) {
	if (error == 0) {
	    if (Tk_SetOptions(textPtr->interp, (char *) marker,
		    marker->optionTable,
		    objc, objv, textPtr->tkwin, &savedOptions, &mask)
		    != TCL_OK) {
		mask = 0;
		continue;
	    }

	    if (marker->imageString != NULL) {
		image = Tk_GetImage(textPtr->interp, textPtr->tkwin,
			marker->imageString, MarkerImageProc,
			(ClientData) textPtr);
		if (image == NULL) {
		    continue;
		}
	    }

	    Tk_FreeSavedOptions(&savedOptions);
	    break;
	} else {
	    errorResult = Tcl_GetObjResult(textPtr->interp);
	    Tcl_IncrRefCount(errorResult);
	    Tk_RestoreSavedOptions(&savedOptions);

	    if (image != NULL)
		Tk_FreeImage(image);

	    Tcl_SetObjResult(textPtr->interp, errorResult);
	    Tcl_DecrRefCount(errorResult);
	    return TCL_ERROR;
	}
    }

    if (image != marker->image) {
	if (marker->image)
	    Tk_FreeImage(marker->image);
	marker->image = image;
    }

    textPtr->lineMarkerMaxWidth = -1;
    if (textPtr->autoSizeMarkerMarginCount > 0)
	TkTextEventuallyRelayoutWindow(textPtr);

    return TCL_OK;
}

int
TkTextGetLineMarker(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Obj *objPtr,		/* Marker name object. */
    TkTextLineMarker **markerPtr) /* Returned marker. */
{
    Tcl_HashEntry *hPtr;
    CONST char *name;

    name = Tcl_GetString(objPtr);
    hPtr = Tcl_FindHashEntry(&textPtr->lineMarkerHash, name);
    if (hPtr == NULL) {
	Tcl_AppendResult(textPtr->interp, "line marker \"",
	    name, "\" doesn't exist", NULL);
	return TCL_ERROR;
    }
    (*markerPtr) = Tcl_GetHashValue(hPtr);
    return TCL_OK;
}

static void
SyncLineMarkerFlags(
    TkSharedText *sharedPtr,
    TkTextLine *linePtr)
{
    TkText *peer;
    TkTextLinePeerData *data;
    int i;

    /* Check in each symbol margin in each peer for a marker. */
    for (i = 0; i < 4; i++) {
	if (linePtr->flags & (LINE_FLAG_MARKER1 << i)) {
	    /* NOTE: 'peer' and 'data' are not in sync here.
	     * i.e. data != linePtr->peerData[peer->pixelReference] */
	    for (peer = sharedPtr->peers, data = linePtr->peerData;
		    peer != NULL;
		    peer = peer->next, data++) {
		if (data->flags & (LINE_FLAG_MARKER1 << i))
		    break;
	    }
	    if (peer == NULL) {
		linePtr->flags &= ~(LINE_FLAG_MARKER1 << i);
dbwin("MARKERS: line %d has no more markers in margin %d", BTREE_LINESTO(sharedPtr->peers, linePtr)+1, i);
	    }
	}
    }
}

int
TkTextLineMarkerCmd(
    TkText *textPtr,		/* Information about text widget. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *CONST objv[])	/* Argument objects. Someone else has already
				 * parsed this command enough to know that
				 * objv[1] is "image". */
{
    static CONST char *optionStrings[] = {
	"cget", "configure", "create", "delete", "names", "set", NULL
    };
    enum {
	CMD_CGET, CMD_CONF, CMD_CREATE, CMD_DELETE, CMD_NAMES, CMD_SET
    };
    int index;
    TkTextLineMarker *marker;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }
    switch (index) {
	case CMD_CGET: {
	    Tcl_Obj *objPtr;

	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 3, objv, "name option");
		return TCL_ERROR;
	    }
	    if (TkTextGetLineMarker(textPtr, objv[3], &marker) != TCL_OK) {
		return TCL_ERROR;
	    }
	    objPtr = Tk_GetOptionValue(interp, (char *) marker,
		    marker->optionTable, objv[4], textPtr->tkwin);
	    if (objPtr == NULL) {
		return TCL_ERROR;
	    }
	    Tcl_SetObjResult(interp, objPtr);
	    break;
	}
	case CMD_CONF: {
	    if (objc < 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name ?option value ...?");
		return TCL_ERROR;
	    }
	    if (TkTextGetLineMarker(textPtr, objv[3], &marker) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (objc <= 5) {
		Tcl_Obj *objPtr = Tk_GetOptionInfo(interp,
			(char *) marker, marker->optionTable,
			(objc == 5) ? objv[4] : NULL, textPtr->tkwin);
		if (objPtr == NULL) {
		    return TCL_ERROR;
		}
		Tcl_SetObjResult(interp, objPtr);
		break;
	    }
	    if (MarkerConfigure(textPtr, marker, objc - 4, objv + 4)
		    != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (marker->refCount > 0) {
		/* FIXME: only draw the affected lines. */
		TkTextEventuallyRedrawMargin(textPtr);
	    }
	    break;
	}
	case CMD_CREATE: {
	    Tcl_HashEntry *hPtr;
	    int isNew;
	    CONST char *name;

	    if (objc < 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name ?option value ...?");
		return TCL_ERROR;
	    }
	    name = Tcl_GetString(objv[3]);
	    if (!name[0]) {
		Tcl_AppendResult(interp, "bad line marker name \"\"", NULL);
		return TCL_ERROR;
	    }
	    hPtr = Tcl_CreateHashEntry(&textPtr->lineMarkerHash,
		    name, &isNew);
	    if (!isNew) {
		Tcl_AppendResult(interp, "line marker \"", name,
			"\" already exists", NULL);
		return TCL_ERROR;
	    }

	    marker = (TkTextLineMarker *) ckalloc(sizeof(TkTextLineMarker));
	    memset(marker, '\0', sizeof(TkTextLineMarker));
	    marker->name = ckalloc(strlen(name) + 1);
	    strcpy(marker->name, name);
	    marker->optionTable = Tk_CreateOptionTable(interp, optionSpecs);

	    if (Tk_InitOptions(interp, (char *) marker,
		    marker->optionTable, textPtr->tkwin) != TCL_OK) {
		MarkerFree(textPtr, marker);
		Tcl_DeleteHashEntry(hPtr);
		return TCL_ERROR;
	    }
	    if (MarkerConfigure(textPtr, marker, objc - 4, objv + 4)
		    != TCL_OK) {
		MarkerFree(textPtr, marker);
		Tcl_DeleteHashEntry(hPtr);
		return TCL_ERROR;
	    }
	    Tcl_SetHashValue(hPtr, marker);
	    break;
	}
	case CMD_DELETE: {
	    Tcl_HashEntry *hPtr, *deadEntries[128];
	    Tcl_HashSearch search;
	    int i, d, deadCount;
	    TkTextLine *linePtr;
	    int ref = textPtr->pixelReference;
	    TkText *peer;

	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 3, objv, "name");
		return TCL_ERROR;
	    }
	    if (TkTextGetLineMarker(textPtr, objv[3], &marker) != TCL_OK) {
		return TCL_ERROR;
	    }

	    /* If no lines are assigned this marker, just delete it. */
	    if (marker->refCount <= 0)
		goto doneDELETE;

	    /* Search for lines using this marker in each margin.
	     * The whole business with deadEntries[] is because one cannot
	     * delete entries while searching a hash table. */
	    for (i = 0; i < 4; i++) {
		Tcl_HashTable *tablePtr = &textPtr->markerInMarginHash[i];
		int flag = (LINE_FLAG_MARKER1 << i);
		hPtr = Tcl_FirstHashEntry(tablePtr, &search);
		deadCount = 0;
		while (hPtr != NULL) {
		    TkTextLineMarker *marker2 = Tcl_GetHashValue(hPtr);
		    if (marker2 == marker) {
			deadEntries[deadCount++] = hPtr;
			if (deadCount == 128) {
			    for (d = 0; d < deadCount; d++) {
				linePtr = (TkTextLine *) Tcl_GetHashKey(
					tablePtr, deadEntries[d]);
				linePtr->peerData[ref].flags &= ~flag;
				ASSERT(marker->refCount > 0);
				marker->refCount--;
				linePtr->flags &= ~flag; /* fixed below */
				TkTextEventuallyRedrawMarginLine(textPtr,
					linePtr);
				Tcl_DeleteHashEntry(deadEntries[d]);
			    }
			    deadCount = 0;
			    hPtr = Tcl_FirstHashEntry(tablePtr, &search);
			    continue;
			}
		    }
		    hPtr = Tcl_NextHashEntry(&search);
		}
		for (d = 0; d < deadCount; d++) {
		    linePtr = (TkTextLine *) Tcl_GetHashKey(tablePtr,
			    deadEntries[d]);
		    linePtr->peerData[ref].flags &= ~flag;
		    ASSERT(marker->refCount > 0);
		    marker->refCount--;
		    linePtr->flags &= ~flag; /* fixed below */
		    TkTextEventuallyRedrawMarginLine(textPtr, linePtr);
		    Tcl_DeleteHashEntry(deadEntries[d]);
		}
	    }

	    /* Now go back through every line that still has a marker in
	     * any peer and set the B-tree LINE_FLAG_MARKERx flag. */
	    for (peer = textPtr->sharedTextPtr->peers;
		    peer != NULL;
		    peer = peer->next) {
		for (i = 0; i < 4; i++) {
		    Tcl_HashTable *tablePtr = &peer->markerInMarginHash[i];
		    hPtr = Tcl_FirstHashEntry(tablePtr, &search);
		    while (hPtr != NULL) {
			linePtr = (TkTextLine *) Tcl_GetHashKey(tablePtr, hPtr);
			linePtr->flags |= (LINE_FLAG_MARKER1 << i);
			hPtr = Tcl_NextHashEntry(&search);
		    }
		}
	    }

	    doneDELETE:
	    ASSERT(marker->refCount == 0);
	    hPtr = Tcl_FindHashEntry(&textPtr->lineMarkerHash, marker->name);
	    MarkerFree(textPtr, marker);
	    Tcl_DeleteHashEntry(hPtr);

	    textPtr->lineMarkerMaxWidth = -1;
	    if (textPtr->autoSizeMarkerMarginCount > 0)
		TkTextEventuallyRelayoutWindow(textPtr);
	    break;
	}
	case CMD_NAMES: {
	    Tcl_HashSearch search;
	    Tcl_HashEntry *hPtr;

	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 3, objv, NULL);
		return TCL_ERROR;
	    }
	    for (hPtr = Tcl_FirstHashEntry(&textPtr->lineMarkerHash, &search);
		    hPtr != NULL;
		    hPtr = Tcl_NextHashEntry(&search)) {
		Tcl_AppendElement(interp,
			Tcl_GetHashKey(&textPtr->lineMarkerHash, hPtr));
	    }
	    break;
	}
 	case CMD_SET: {
	    CONST TkTextIndex *indexPtr;
	    TkTextLine *linePtr;
	    Tcl_HashEntry *hPtr;
	    TkTextMargin *margin, *margins[4];
	    int i, isNew;
	    int ref = textPtr->pixelReference;

	    if (objc < 4 || objc > 6) {
		Tcl_WrongNumArgs(interp, 3, objv, "line ?margin? ?name?");
		return TCL_ERROR;
	    }
	    indexPtr = TkTextGetIndexFromObj(interp, textPtr, objv[3]);
	    if (indexPtr == NULL) {
		return TCL_ERROR;
	    }
	    linePtr = indexPtr->linePtr;

	    margins[0] = textPtr->margin1;
	    margins[1] = textPtr->margin2;
	    margins[2] = textPtr->margin3;
	    margins[3] = textPtr->margin4;

	    if (objc == 4) {
		for (i = 0; i < 4; i++) {
		    hPtr = Tcl_FindHashEntry(&textPtr->markerInMarginHash[i],
			    (char *) linePtr);
		    if (hPtr != NULL) {
			Tcl_AppendElement(interp, TkTextMarginName(textPtr,
			    margins[i]));
			marker = Tcl_GetHashValue(hPtr);
			Tcl_AppendElement(interp, marker->name);
		    }
		}
		break;
	    }
	    if (TkTextGetMargin(textPtr, objv[4], &margin) != TCL_OK) {
		return TCL_ERROR;
	    }
	    for (i = 0; i < 4; i++) {
		if (margins[i] == margin)
		    break;
	    }
	    if (i == 4) {
		Tcl_AppendResult(interp, "margin \"",
			TkTextMarginName(textPtr, margin),
			"\" is not a symbol margin", NULL);
		return TCL_ERROR;
	    }
	    if (objc == 5) {
		hPtr = Tcl_FindHashEntry(&textPtr->markerInMarginHash[i],
			(char *) linePtr);
		if (hPtr != NULL) {
		    marker = Tcl_GetHashValue(hPtr);
		    Tcl_SetResult(interp, marker->name, TCL_VOLATILE);
		}
		break;
	    }
	    if (Tcl_GetString(objv[5])[0] == '\0') {
		hPtr = Tcl_FindHashEntry(&textPtr->markerInMarginHash[i],
			(char *) linePtr);
		if (hPtr != NULL) {
		    marker = Tcl_GetHashValue(hPtr);
		    ASSERT(marker->refCount > 0);
		    marker->refCount--;
		    linePtr->peerData[ref].flags &= ~(LINE_FLAG_MARKER1 << i);
		    SyncLineMarkerFlags(textPtr->sharedTextPtr, linePtr);
		    TkTextEventuallyRedrawMarginLine(textPtr, linePtr);
		    Tcl_DeleteHashEntry(hPtr);
		}
		break;
	    }
	    if (TkTextGetLineMarker(textPtr, objv[5], &marker) != TCL_OK) {
		return TCL_ERROR;
	    }
	    hPtr = Tcl_CreateHashEntry(&textPtr->markerInMarginHash[i],
		    (char *) linePtr, &isNew);
	    if (!isNew) {
		TkTextLineMarker *marker2 = Tcl_GetHashValue(hPtr);
		if (marker == marker2)
		    break;
	    }
	    Tcl_SetHashValue(hPtr, marker);
	    linePtr->peerData[ref].flags |= (LINE_FLAG_MARKER1 << i);
	    marker->refCount++;
	    linePtr->flags |= (LINE_FLAG_MARKER1 << i);
	    TkTextEventuallyRedrawMarginLine(textPtr, linePtr);
	    break;
	}
    }
    return TCL_OK;
}

void
DrawBitmap(
    TkText *textPtr,		/* Information about text widget. */
    Pixmap bitmap,		/* Bitmap to draw. */
    Drawable drawable,		/* Where to draw. */
    XColor *fg, XColor *bg,	/* Foreground and background colors.
				 * May be NULL. */
    int src_x, int src_y,	/* Left and top of part of bitmap to copy. */
    int width, int height,	/* Width and height of part of bitmap to
				 * copy. */
    int dest_x, int dest_y	/* Left and top coordinates to copy part of
				 * the bitmap to. */
    )
{
    XGCValues gcValues;
    GC gc;
    unsigned long mask = 0;

    if (fg != NULL) {
	gcValues.foreground = fg->pixel;
	mask |= GCForeground;
    }
    if (bg != NULL) {
	gcValues.background = bg->pixel;
	mask |= GCBackground;
    } else {
	gcValues.clip_mask = bitmap;
	mask |= GCClipMask;
    }
    gcValues.graphics_exposures = False;
    mask |= GCGraphicsExposures;
    gc = Tk_GetGC(textPtr->tkwin, mask, &gcValues);
    XSetClipOrigin(textPtr->display, gc, dest_x, dest_y);
    XCopyPlane(textPtr->display, bitmap, drawable, gc,
	src_x, src_y, (unsigned int) width, (unsigned int) height,
	dest_x, dest_y, 1);
    XSetClipOrigin(textPtr->display, gc, 0, 0);
    Tk_FreeGC(textPtr->display, gc);
}

void
TkTextLineMarkerDraw(
    TkText *textPtr,
    TkTextLineMarker *marker,
    TkTextLine *linePtr,
    Pixmap pixmap,
    GC gc,
    int justify,
    int padX,
    int x,
    int width,
    int height,
    int baseline)
{
    int markerWidth, markerHeight;
    int bytesThatFit = 0;
    int left = 0;
    CONST char *s = NULL;

    if (marker->image != NULL) {
	Tk_SizeOfImage(marker->image, &markerWidth, &markerHeight);
	goto draw;
    }
    if (marker->bitmap != None) {
	Tk_SizeOfBitmap(textPtr->display, marker->bitmap, &markerWidth, &markerHeight);
	goto draw;
    }
    if (marker->textObj != NULL) {
	int bytes;
	s = Tcl_GetStringFromObj(marker->textObj, &bytes);
	bytesThatFit = Tk_MeasureChars(textPtr->tkfont, s, bytes, width - padX * 2,
		0, &markerWidth);
	if (bytesThatFit < bytes)
	    markerWidth = width - padX * 2;
	goto draw;
    }
    return;
    draw:
    switch (justify) {
	case TK_JUSTIFY_LEFT:
	    left = x + padX;
	    break;
	case TK_JUSTIFY_CENTER:
	    left = x + (width - markerWidth) / 2;
	    break;
	case TK_JUSTIFY_RIGHT:
	    left = x + width - padX - markerWidth;
	    break;
    }

    if (marker->image != NULL) {
	Tk_RedrawImage(marker->image, 0, 0, markerWidth, markerHeight, pixmap,
		left, 0 + (height - markerHeight) / 2);
	return;
    }
    if (marker->bitmap != None) {
	DrawBitmap(textPtr, marker->bitmap, pixmap, NULL, NULL,
		0, 0, markerWidth, markerHeight, left,
		0 + (height - markerHeight) / 2);
	return;
    }
    if (marker->textObj != NULL) {
	Tk_DrawChars(textPtr->display, pixmap, gc, textPtr->tkfont,
		s, bytesThatFit, left, baseline);
    }
}

void
TkTextLineMarkerLineDeleted(
    TkSharedText *sharedPtr,
    TkTextLine *linePtr)
{
    TkText *peer;
    TkTextLineMarker *marker;
    TkTextLinePeerData *data = linePtr->peerData;
    Tcl_HashEntry *hPtr;
    int i, flags;

    for (peer = sharedPtr->peers;
	    peer != NULL;
	    peer = peer->next) {
	flags = data[peer->pixelReference].flags;
	if (flags & LINE_FLAG_MARKER1TO4) {
	    for (i = 0; i < 4; i++) {
		if (flags & (LINE_FLAG_MARKER1 << i)) {
		    hPtr = Tcl_FindHashEntry(&peer->markerInMarginHash[i],
			    (char *) linePtr);
		    ASSERT(hPtr != NULL);
		    marker = Tcl_GetHashValue(hPtr);
		    ASSERT(marker->refCount > 0);
		    marker->refCount--;
		    Tcl_DeleteHashEntry(hPtr);
		}
	    }
	}
    }
}

int 
TkTextLineMarkerMaxWidth(
    TkText *textPtr)		/* Information about text widget. */
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    int width, max = 0;

    if (textPtr->lineMarkerMaxWidth > 0) {
	return textPtr->lineMarkerMaxWidth;
    }
    hPtr = Tcl_FirstHashEntry(&textPtr->lineMarkerHash, &search);
    while (hPtr != NULL) {
	width = MarkerWidth(textPtr, Tcl_GetHashValue(hPtr));
	if (width > max)
	    max = width;
	hPtr = Tcl_NextHashEntry(&search);
    }
    return textPtr->lineMarkerMaxWidth = max;
}

void
TkTextInitLineMarkers(
    TkText *textPtr)		/* Information about text widget. */
{
    int i;
    
    Tcl_InitHashTable(&textPtr->lineMarkerHash, TCL_STRING_KEYS);
    for (i = 0; i < 4; i++) {
	Tcl_InitHashTable(&textPtr->markerInMarginHash[i], TCL_ONE_WORD_KEYS);
    }
}

void
TkTextFreeLineMarkers(
    TkText *textPtr)		/* Information about text widget. */
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    int i;

    for (hPtr = Tcl_FirstHashEntry(&textPtr->lineMarkerHash, &search);
	    hPtr != NULL;
	    hPtr = Tcl_NextHashEntry(&search)) {
	MarkerFree(textPtr, Tcl_GetHashValue(hPtr));
    }
    Tcl_DeleteHashTable(&textPtr->lineMarkerHash);
    for (i = 0; i < 4; i++) {
	Tcl_DeleteHashTable(&textPtr->markerInMarginHash[i]);
    }
}

