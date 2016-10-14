# textplus.tcl --
#
# This file defines the default bindings for Tk textplus widgets and provides
# procedures that help in implementing the bindings.
#
# RCS: @(#) $Id$
#
# Copyright (c) 1992-1994 The Regents of the University of California.
# Copyright (c) 1994-1997 Sun Microsystems, Inc.
# Copyright (c) 1998 by Scriptics Corporation.
# Copyright (c) 2007 by Tim Baker.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#-------------------------------------------------------------------------
# Elements of ::tk::Priv that are used in this file:
#
# afterId -		If non-null, it means that auto-scanning is underway
#			and it gives the "after" id for the next auto-scan
#			command to be executed.
# char -		Character position on the line;  kept in order
#			to allow moving up or down past short lines while
#			still remembering the desired position.
# mouseMoved -		Non-zero means the mouse has moved a significant
#			amount since the button went down (so, for example,
#			start dragging out a selection).
# prevPos -		Used when moving up or down lines via the keyboard.
#			Keeps track of the previous insert position, so
#			we can distinguish a series of ups and downs, all
#			in a row, from a new up or down.
# selectMode -		The style of selection currently underway:
#			char, word, or line.
# x, y -		Last known mouse coordinates for scanning
#			and auto-scanning.
#-------------------------------------------------------------------------

#-------------------------------------------------------------------------
# The code below creates the default class bindings for text widgets.
#-------------------------------------------------------------------------

# Standard Motif bindings:

bind TextPlus <1> {
    TextPlus::MarginClick %W %x %y
    TextPlus::Button1 %W %x %y
    %W tag remove sel 0.0 end
}
bind TextPlus <B1-Motion> {
    set TextPlus::Priv(x) %x
    set TextPlus::Priv(y) %y
    TextPlus::MarginMotion %W %x %y
    TextPlus::SelectTo %W %x %y
}
bind TextPlus <Double-1> {
    TextPlus::MarginClick %W %x %y
    set TextPlus::Priv(selectMode) word
    TextPlus::SelectTo %W %x %y
    catch {%W mark set insert sel.first}
}
bind TextPlus <Triple-1> {
    TextPlus::MarginClick %W %x %y
    set TextPlus::Priv(selectMode) line
    TextPlus::SelectTo %W %x %y
    catch {%W mark set insert sel.first}
}
bind TextPlus <Shift-1> {
    TextPlus::ResetAnchor %W @%x,%y
    set TextPlus::Priv(selectMode) char
    TextPlus::SelectTo %W %x %y
}
bind TextPlus <Double-Shift-1>	{
    set TextPlus::Priv(selectMode) word
    TextPlus::SelectTo %W %x %y 1
}
bind TextPlus <Triple-Shift-1>	{
    set TextPlus::Priv(selectMode) line
    TextPlus::SelectTo %W %x %y
}
bind TextPlus <B1-Leave> {
    set TextPlus::Priv(x) %x
    set TextPlus::Priv(y) %y
    TextPlus::AutoScan %W
}
bind TextPlus <B1-Enter> {
    TextPlus::CancelRepeat
}
bind TextPlus <ButtonRelease-1> {
    TextPlus::MarginRelease %W %x %y
    TextPlus::CancelRepeat
}
bind TextPlus <Control-1> {
    %W mark set insert @%x,%y
}
bind TextPlus <Left> {
    TextPlus::SetCursor %W insert-1displayindices
}
bind TextPlus <Right> {
    TextPlus::SetCursor %W insert+1displayindices
}
bind TextPlus <Up> {
    TextPlus::SetCursor %W [TextPlus::UpDownLine %W -1]
}
bind TextPlus <Down> {
    TextPlus::SetCursor %W [TextPlus::UpDownLine %W 1]
}
bind TextPlus <Shift-Left> {
    TextPlus::KeySelect %W [%W index {insert - 1displayindices}]
}
bind TextPlus <Shift-Right> {
    TextPlus::KeySelect %W [%W index {insert + 1displayindices}]
}
bind TextPlus <Shift-Up> {
    TextPlus::KeySelect %W [TextPlus::UpDownLine %W -1]
}
bind TextPlus <Shift-Down> {
    TextPlus::KeySelect %W [TextPlus::UpDownLine %W 1]
}
bind TextPlus <Control-Left> {
    TextPlus::SetCursor %W [TextPlus::PrevPos %W insert tcl_startOfPreviousWord]
}
bind TextPlus <Control-Right> {
    TextPlus::SetCursor %W [TextPlus::NextWord %W insert]
}
bind TextPlus <Control-Up> {
    TextPlus::SetCursor %W [TextPlus::PrevPara %W insert]
}
bind TextPlus <Control-Down> {
    TextPlus::SetCursor %W [TextPlus::NextPara %W insert]
}
bind TextPlus <Shift-Control-Left> {
    TextPlus::KeySelect %W [TextPlus::PrevPos %W insert tcl_startOfPreviousWord]
}
bind TextPlus <Shift-Control-Right> {
    TextPlus::KeySelect %W [TextPlus::NextWord %W insert]
}
bind TextPlus <Shift-Control-Up> {
    TextPlus::KeySelect %W [TextPlus::PrevPara %W insert]
}
bind TextPlus <Shift-Control-Down> {
    TextPlus::KeySelect %W [TextPlus::NextPara %W insert]
}
bind TextPlus <Prior> {
    TextPlus::SetCursor %W [TextPlus::ScrollPages %W -1]
}
bind TextPlus <Shift-Prior> {
    TextPlus::KeySelect %W [TextPlus::ScrollPages %W -1]
}
bind TextPlus <Next> {
    TextPlus::SetCursor %W [TextPlus::ScrollPages %W 1]
}
bind TextPlus <Shift-Next> {
    TextPlus::KeySelect %W [TextPlus::ScrollPages %W 1]
}
bind TextPlus <Control-Prior> {
    %W xview scroll -1 page
}
bind TextPlus <Control-Next> {
    %W xview scroll 1 page
}

bind TextPlus <Home> {
    TextPlus::SetCursor %W {insert display linestart}
}
bind TextPlus <Shift-Home> {
    TextPlus::KeySelect %W {insert display linestart}
}
bind TextPlus <End> {
    TextPlus::SetCursor %W {insert display lineend}
}
bind TextPlus <Shift-End> {
    TextPlus::KeySelect %W {insert display lineend}
}
bind TextPlus <Control-Home> {
    TextPlus::SetCursor %W 1.0
}
bind TextPlus <Control-Shift-Home> {
    TextPlus::KeySelect %W 1.0
}
bind TextPlus <Control-End> {
    TextPlus::SetCursor %W {end - 1 indices}
}
bind TextPlus <Control-Shift-End> {
    TextPlus::KeySelect %W {end - 1 indices}
}

bind TextPlus <Tab> {
    if {[%W cget -state] eq "normal"} {
	TextPlus::Insert %W \t
	focus %W
	break
    }
}
bind TextPlus <Shift-Tab> {
    # Needed only to keep <Tab> binding from triggering;  doesn't
    # have to actually do anything.
    break
}
bind TextPlus <Control-Tab> {
    focus [tk_focusNext %W]
}
bind TextPlus <Control-Shift-Tab> {
    focus [tk_focusPrev %W]
}
bind TextPlus <Control-i> {
    TextPlus::Insert %W \t
}
bind TextPlus <Return> {
    TextPlus::Insert %W \n
    if {[%W cget -autoseparators]} {
	%W edit separator
    }
}
bind TextPlus <Delete> {
    if {[%W tag nextrange sel 1.0 end] ne ""} {
	%W delete sel.first sel.last
    } else {
	%W delete insert
	%W see insert
    }
}
bind TextPlus <BackSpace> {
    if {[%W tag nextrange sel 1.0 end] ne ""} {
	%W delete sel.first sel.last
    } elseif {[%W compare insert != 1.0]} {
	%W delete insert-1c
	%W see insert
    }
}

bind TextPlus <Control-space> {
    %W mark set tk::anchor%W insert
}
bind TextPlus <Select> {
    %W mark set tk::anchor%W insert
}
bind TextPlus <Control-Shift-space> {
    set TextPlus::Priv(selectMode) char
    TextPlus::KeyExtend %W insert
}
bind TextPlus <Shift-Select> {
    set TextPlus::Priv(selectMode) char
    TextPlus::KeyExtend %W insert
}
bind TextPlus <Control-slash> {
    %W tag add sel 1.0 end
}
bind TextPlus <Control-backslash> {
    %W tag remove sel 1.0 end
}
bind TextPlus <<Cut>> {
    tk_textCut %W
}
bind TextPlus <<Copy>> {
    tk_textCopy %W
}
bind TextPlus <<Paste>> {
    tk_textPaste %W
}
bind TextPlus <<Clear>> {
    catch {%W delete sel.first sel.last}
}
bind TextPlus <<PasteSelection>> {
    if {$tk_strictMotif || ![info exists TextPlus::Priv(mouseMoved)]
	    || !$TextPlus::Priv(mouseMoved)} {
	TextPlus::PasteSelection %W %x %y
    }
}
bind TextPlus <Insert> {
    catch {TextPlus::Insert %W [::tk::GetSelection %W PRIMARY]}
}
bind TextPlus <KeyPress> {
    TextPlus::Insert %W %A
}

# Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
# Otherwise, if a widget binding for one of these is defined, the
# <KeyPress> class binding will also fire and insert the character,
# which is wrong.  Ditto for <Escape>.

bind TextPlus <Alt-KeyPress> {# nothing }
bind TextPlus <Meta-KeyPress> {# nothing}
bind TextPlus <Control-KeyPress> {# nothing}
bind TextPlus <Escape> {# nothing}
bind TextPlus <KP_Enter> {# nothing}
if {[tk windowingsystem] eq "aqua"} {
    bind TextPlus <Command-KeyPress> {# nothing}
}

# Additional emacs-like bindings:

bind TextPlus <Control-a> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W {insert display linestart}
    }
}
bind TextPlus <Control-b> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W insert-1displayindices
    }
}
bind TextPlus <Control-d> {
    if {!$tk_strictMotif} {
	%W delete insert
    }
}
bind TextPlus <Control-e> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W {insert display lineend}
    }
}
bind TextPlus <Control-f> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W insert+1displayindices
    }
}
bind TextPlus <Control-k> {
    if {!$tk_strictMotif} {
	if {[%W compare insert == {insert lineend}]} {
	    %W delete insert
	} else {
	    %W delete insert {insert lineend}
	}
    }
}
bind TextPlus <Control-n> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W [TextPlus::UpDownLine %W 1]
    }
}
bind TextPlus <Control-o> {
    if {!$tk_strictMotif} {
	%W insert insert \n
	%W mark set insert insert-1c
    }
}
bind TextPlus <Control-p> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W [TextPlus::UpDownLine %W -1]
    }
}
bind TextPlus <Control-t> {
    if {!$tk_strictMotif} {
	TextPlus::Transpose %W
    }
}

bind TextPlus <<Undo>> {
    catch { %W edit undo }
}

bind TextPlus <<Redo>> {
    catch { %W edit redo }
}

bind TextPlus <Meta-b> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W [TextPlus::PrevPos %W insert tcl_startOfPreviousWord]
    }
}
bind TextPlus <Meta-d> {
    if {!$tk_strictMotif} {
	%W delete insert [TextPlus::NextWord %W insert]
    }
}
bind TextPlus <Meta-f> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W [TextPlus::NextWord %W insert]
    }
}
bind TextPlus <Meta-less> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W 1.0
    }
}
bind TextPlus <Meta-greater> {
    if {!$tk_strictMotif} {
	TextPlus::SetCursor %W end-1c
    }
}
bind TextPlus <Meta-BackSpace> {
    if {!$tk_strictMotif} {
	%W delete [TextPlus::PrevPos %W insert tcl_startOfPreviousWord] insert
    }
}
bind TextPlus <Meta-Delete> {
    if {!$tk_strictMotif} {
	%W delete [TextPlus::PrevPos %W insert tcl_startOfPreviousWord] insert
    }
}

# Macintosh only bindings:

if {[tk windowingsystem] eq "aqua"} {
bind TextPlus <Option-Left> {
    TextPlus::SetCursor %W [TextPlus::PrevPos %W insert tcl_startOfPreviousWord]
}
bind TextPlus <Option-Right> {
    TextPlus::SetCursor %W [TextPlus::NextWord %W insert]
}
bind TextPlus <Option-Up> {
    TextPlus::SetCursor %W [TextPlus::PrevPara %W insert]
}
bind TextPlus <Option-Down> {
    TextPlus::SetCursor %W [TextPlus::NextPara %W insert]
}
bind TextPlus <Shift-Option-Left> {
    TextPlus::KeySelect %W [TextPlus::PrevPos %W insert tcl_startOfPreviousWord]
}
bind TextPlus <Shift-Option-Right> {
    TextPlus::KeySelect %W [TextPlus::NextWord %W insert]
}
bind TextPlus <Shift-Option-Up> {
    TextPlus::KeySelect %W [TextPlus::PrevPara %W insert]
}
bind TextPlus <Shift-Option-Down> {
    TextPlus::KeySelect %W [TextPlus::NextPara %W insert]
}
bind TextPlus <Control-v> {
    TextPlus::ScrollPages %W 1
}

# End of Mac only bindings
}

# A few additional bindings of my own.

bind TextPlus <Control-h> {
    if {!$tk_strictMotif && [%W compare insert != 1.0]} {
	%W delete insert-1c
	%W see insert
    }
}
bind TextPlus <2> {
    if {!$tk_strictMotif} {
	TextPlus::ScanMark %W %x %y
    }
}
bind TextPlus <B2-Motion> {
    if {!$tk_strictMotif} {
	TextPlus::ScanDrag %W %x %y
    }
}
# Maybe my computer is funky, but without this binding ButtonRelease-2 is
# like <<PasteSelection>>
bind TextPlus <ButtonRelease-2> break

set ::TextPlus::Priv(prevPos) {}

# The MouseWheel will typically only fire on Windows and MacOS X.
# However, someone could use the "event generate" command to produce one
# on other platforms.  We must be careful not to round -ve values of %D
# down to zero.

if {[tk windowingsystem] eq "aqua"} {
    bind TextPlus <MouseWheel> {
        %W yview scroll [expr {-15 * (%D)}] pixels
    }
    bind TextPlus <Option-MouseWheel> {
        %W yview scroll [expr {-150 * (%D)}] pixels
    }
    bind TextPlus <Shift-MouseWheel> {
        %W xview scroll [expr {-15 * (%D)}] pixels
    }
    bind TextPlus <Shift-Option-MouseWheel> {
        %W xview scroll [expr {-150 * (%D)}] pixels
    }
} else {
    # We must make sure that positive and negative movements are rounded
    # equally to integers, avoiding the problem that
    #     (int)1/3 = 0,
    # but
    #     (int)-1/3 = -1
    # The following code ensure equal +/- behaviour.
    bind TextPlus <MouseWheel> {
	if {%D >= 0} {
	    %W yview scroll [expr {-%D/3}] pixels
	} else {
	    %W yview scroll [expr {(2-%D)/3}] pixels
	}
    }
}

if {"x11" eq [tk windowingsystem]} {
    # Support for mousewheels on Linux/Unix commonly comes through mapping
    # the wheel to the extended buttons.  If you have a mousewheel, find
    # Linux configuration info at:
    #	http://www.inria.fr/koala/colas/mouse-wheel-scroll/
    bind TextPlus <4> {
	if {!$tk_strictMotif} {
	    %W yview scroll -50 pixels
	}
    }
    bind TextPlus <5> {
	if {!$tk_strictMotif} {
	    %W yview scroll 50 pixels
	}
    }
}

# ::TextPlus::ClosestGap --
# Given x and y coordinates, this procedure finds the closest boundary
# between characters to the given coordinates and returns the index
# of the character just after the boundary.
#
# Arguments:
# w -		The text window.
# x -		X-coordinate within the window.
# y -		Y-coordinate within the window.

proc ::TextPlus::ClosestGap {w x y} {
    set pos [$w index @$x,$y]
    set bbox [$w bbox $pos]
    if {$bbox eq ""} {
	return $pos
    }
    if {($x - [lindex $bbox 0]) < ([lindex $bbox 2]/2)} {
	return $pos
    }
    $w index "$pos + 1 char"
}

# ::TextPlus::Button1 --
# This procedure is invoked to handle button-1 presses in text
# widgets.  It moves the insertion cursor, sets the selection anchor,
# and claims the input focus.
#
# Arguments:
# w -		The text window in which the button was pressed.
# x -		The x-coordinate of the button press.
# y -		The x-coordinate of the button press.

proc ::TextPlus::Button1 {w x y} {
    variable ::TextPlus::Priv

    set Priv(selectMode) char
    set Priv(mouseMoved) 0
    set Priv(pressX) $x
    $w mark set insert [ClosestGap $w $x $y]
    $w mark set tk::anchor$w insert
    # Set the anchor mark's gravity depending on the click position
    # relative to the gap
    set bbox [$w bbox [$w index tk::anchor$w]]
    if {$x > [lindex $bbox 0]} {
	$w mark gravity tk::anchor$w right
    } else {
	$w mark gravity tk::anchor$w left
    }
    # Allow focus in any case on Windows, because that will let the
    # selection be displayed even for state disabled text widgets.
    if {$::tcl_platform(platform) eq "windows" \
	    || [$w cget -state] eq "normal"} {
	focus $w
    }
    if {[$w cget -autoseparators]} {
	$w edit separator
    }
}

# ::TextPlus::SelectTo --
# This procedure is invoked to extend the selection, typically when
# dragging it with the mouse.  Depending on the selection mode (character,
# word, line) it selects in different-sized units.  This procedure
# ignores mouse motions initially until the mouse has moved from
# one character to another or until there have been multiple clicks.
#
# Note that the 'anchor' is implemented programmatically using
# a text widget mark, and uses a name that will be unique for each
# text widget (even when there are multiple peers).  Currently the
# anchor is considered private to Tk, hence the name 'tk::anchor$w'.
#
# Arguments:
# w -		The text window in which the button was pressed.
# x -		Mouse x position.
# y - 		Mouse y position.

proc ::TextPlus::SelectTo {w x y {extend 0}} {
    global tcl_platform
    variable ::TextPlus::Priv

    set cur [ClosestGap $w $x $y]
    if {[catch {$w index tk::anchor$w}]} {
	$w mark set tk::anchor$w $cur
    }
    set anchor [$w index tk::anchor$w]
    if {[$w compare $cur != $anchor] || (abs($Priv(pressX) - $x) >= 3)} {
	set Priv(mouseMoved) 1
    }
    switch -- $Priv(selectMode) {
	char {
	    if {[$w compare $cur < tk::anchor$w]} {
		set first $cur
		set last tk::anchor$w
	    } else {
		set first tk::anchor$w
		set last $cur
	    }
	}
	word {
	    # Set initial range based only on the anchor (1 char min width)
	    if {[$w mark gravity tk::anchor$w] eq "right"} {
		set first "tk::anchor$w"
		set last "tk::anchor$w + 1c"
	    } else {
		set first "tk::anchor$w - 1c"
		set last "tk::anchor$w"
	    }
	    # Extend range (if necessary) based on the current point
	    if {[$w compare $cur < $first]} {
		set first $cur
	    } elseif {[$w compare $cur > $last]} {
		set last $cur
	    }

	    # Now find word boundaries
	    set first [PrevPos $w "$first + 1c" tcl_wordBreakBefore]
	    set last [NextPos $w "$last - 1c" tcl_wordBreakAfter]
	}
	line {
	    # Set initial range based only on the anchor
	    set first "tk::anchor$w linestart"
	    set last "tk::anchor$w lineend"

	    # Extend range (if necessary) based on the current point
	    if {[$w compare $cur < $first]} {
		set first "$cur linestart"
	    } elseif {[$w compare $cur > $last]} {
		set last "$cur lineend"
	    }
	    set first [$w index $first]
	    set last [$w index "$last + 1c"]
	}
    }
    if {$Priv(mouseMoved) || ($Priv(selectMode) ne "char")} {
	$w tag remove sel 0.0 end
	$w mark set insert $cur
	$w tag add sel $first $last
	$w tag remove sel $last end
	update idletasks
    }
}

# ::TextPlus::KeyExtend --
# This procedure handles extending the selection from the keyboard,
# where the point to extend to is really the boundary between two
# characters rather than a particular character.
#
# Arguments:
# w -		The text window.
# index -	The point to which the selection is to be extended.

proc ::TextPlus::KeyExtend {w index} {

    set cur [$w index $index]
    if {[catch {$w index tk::anchor$w}]} {
	$w mark set tk::anchor$w $cur
    }
    set anchor [$w index tk::anchor$w]
    if {[$w compare $cur < tk::anchor$w]} {
	set first $cur
	set last tk::anchor$w
    } else {
	set first tk::anchor$w
	set last $cur
    }
    $w tag remove sel 0.0 $first
    $w tag add sel $first $last
    $w tag remove sel $last end
}

# ::TextPlus::PasteSelection --
# This procedure sets the insertion cursor to the mouse position,
# inserts the selection, and sets the focus to the window.
#
# Arguments:
# w -		The text window.
# x, y - 	Position of the mouse.

proc ::TextPlus::PasteSelection {w x y} {
    $w mark set insert [ClosestGap $w $x $y]
    if {![catch {::tk::GetSelection $w PRIMARY} sel]} {
	set oldSeparator [$w cget -autoseparators]
	if {$oldSeparator} {
	    $w configure -autoseparators 0
	    $w edit separator
	}
	$w insert insert $sel
	if {$oldSeparator} {
	    $w edit separator
	    $w configure -autoseparators 1
	}
    }
    if {[$w cget -state] eq "normal"} {
	focus $w
    }
}

# ::TextPlus::AutoScan --
# This procedure is invoked when the mouse leaves a text window
# with button 1 down.  It scrolls the window up, down, left, or right,
# depending on where the mouse is (this information was saved in
# ::tk::Priv(x) and ::tk::Priv(y)), and reschedules itself as an "after"
# command so that the window continues to scroll until the mouse
# moves back into the window or the mouse button is released.
#
# Arguments:
# w -		The text window.

proc ::TextPlus::AutoScan {w} {
    variable ::TextPlus::Priv
    if {![winfo exists $w]} {
	return
    }
    if {$Priv(y) >= [winfo height $w]} {
	$w yview scroll [expr {1 + $Priv(y) - [winfo height $w]}] pixels
    } elseif {$Priv(y) < 0} {
	$w yview scroll [expr {-1 + $Priv(y)}] pixels
    } elseif {$Priv(x) >= [winfo width $w]} {
	$w xview scroll 2 units
    } elseif {$Priv(x) < 0} {
	$w xview scroll -2 units
    } else {
	return
    }
if {![info exists ::TextPlus::Priv(margin,$w)]} {
    SelectTo $w $Priv(x) $Priv(y)
}
    set Priv(afterId) [after 50 [list TextPlus::AutoScan $w]]
}

# ::TextPlus::SetCursor
# Move the insertion cursor to a given position in a text.  Also
# clears the selection, if there is one in the text, and makes sure
# that the insertion cursor is visible.  Also, don't let the insertion
# cursor appear on the dummy last line of the text.
#
# Arguments:
# w -		The text window.
# pos -		The desired new position for the cursor in the window.

proc ::TextPlus::SetCursor {w pos} {

    if {[$w compare $pos == end]} {
	set pos {end - 1 chars}
    }
    $w mark set insert $pos
    $w tag remove sel 1.0 end
    $w see insert
    if {[$w cget -autoseparators]} {
	$w edit separator
    }
}

# ::TextPlus::KeySelect
# This procedure is invoked when stroking out selections using the
# keyboard.  It moves the cursor to a new position, then extends
# the selection to that position.
#
# Arguments:
# w -		The text window.
# new -		A new position for the insertion cursor (the cursor hasn't
#		actually been moved to this position yet).

proc ::TextPlus::KeySelect {w new} {

    if {[$w tag nextrange sel 1.0 end] eq ""} {
	if {[$w compare $new < insert]} {
	    $w tag add sel $new insert
	} else {
	    $w tag add sel insert $new
	}
	$w mark set tk::anchor$w insert
    } else {
	if {[$w compare $new < tk::anchor$w]} {
	    set first $new
	    set last tk::anchor$w
	} else {
	    set first tk::anchor$w
	    set last $new
	}
	$w tag remove sel 1.0 $first
	$w tag add sel $first $last
	$w tag remove sel $last end
    }
    $w mark set insert $new
    $w see insert
    update idletasks
}

# ::TextPlus::ResetAnchor --
# Set the selection anchor to whichever end is farthest from the
# index argument.  One special trick: if the selection has two or
# fewer characters, just leave the anchor where it is.  In this
# case it doesn't matter which point gets chosen for the anchor,
# and for the things like Shift-Left and Shift-Right this produces
# better behavior when the cursor moves back and forth across the
# anchor.
#
# Arguments:
# w -		The text widget.
# index -	Position at which mouse button was pressed, which determines
#		which end of selection should be used as anchor point.

proc ::TextPlus::ResetAnchor {w index} {
    if {[$w tag ranges sel] eq ""} {
	# Don't move the anchor if there is no selection now; this
	# makes the widget behave "correctly" when the user clicks
	# once, then shift-clicks somewhere -- ie, the area between
	# the two clicks will be selected. [Bug: 5929].
	return
    }
    set a [$w index $index]
    set b [$w index sel.first]
    set c [$w index sel.last]
    if {[$w compare $a < $b]} {
	$w mark set tk::anchor$w sel.last
	return
    }
    if {[$w compare $a > $c]} {
	$w mark set tk::anchor$w sel.first
	return
    }
    scan $a "%d.%d" lineA chA
    scan $b "%d.%d" lineB chB
    scan $c "%d.%d" lineC chC
    if {$lineB < $lineC+2} {
	set total [string length [$w get $b $c]]
	if {$total <= 2} {
	    return
	}
	if {[string length [$w get $b $a]] < ($total/2)} {
	    $w mark set tk::anchor$w sel.last
	} else {
	    $w mark set tk::anchor$w sel.first
	}
	return
    }
    if {($lineA-$lineB) < ($lineC-$lineA)} {
	$w mark set tk::anchor$w sel.last
    } else {
	$w mark set tk::anchor$w sel.first
    }
}

# ::TextPlus::Insert --
# Insert a string into a text at the point of the insertion cursor.
# If there is a selection in the text, and it covers the point of the
# insertion cursor, then delete the selection before inserting.
#
# Arguments:
# w -		The text window in which to insert the string
# s -		The string to insert (usually just a single character)

proc ::TextPlus::Insert {w s} {
    if {$s eq "" || [$w cget -state] eq "disabled"} {
	return
    }
    set compound 0
    if {[llength [set range [$w tag ranges sel]]]} {
	if {[$w compare [lindex $range 0] <= insert] \
		&& [$w compare [lindex $range end] >= insert]} {
	    set oldSeparator [$w cget -autoseparators]
	    if {$oldSeparator} {
		$w configure -autoseparators 0
		$w edit separator
		set compound 1
	    }
	    $w delete [lindex $range 0] [lindex $range end]
	}
    }
    $w insert insert $s
    $w see insert
    if {$compound && $oldSeparator} {
	$w edit separator
	$w configure -autoseparators 1
    }
}

# ::TextPlus::UpDownLine --
# Returns the index of the character one display line above or below the
# insertion cursor.  There are two tricky things here.  First, we want to
# maintain the original x position across repeated operations, even though
# some lines that will get passed through don't have enough characters to
# cover the original column.  Second, don't try to scroll past the
# beginning or end of the text.
#
# Arguments:
# w -		The text window in which the cursor is to move.
# n -		The number of display lines to move: -1 for up one line,
#		+1 for down one line.

proc ::TextPlus::UpDownLine {w n} {
    variable ::TextPlus::Priv

    set i [$w index insert]
    if {$Priv(prevPos) ne $i} {
	set Priv(textPosOrig) $i
    }
    set lines [$w count -displaylines $Priv(textPosOrig) $i]
    set new [$w index \
	    "$Priv(textPosOrig) + [expr {$lines + $n}] displaylines"]
    if {[$w compare $new == end] \
	    || [$w compare $new == "insert display linestart"]} {
	set new $i
    }
    set Priv(prevPos) $new
    return $new
}

# ::TextPlus::PrevPara --
# Returns the index of the beginning of the paragraph just before a given
# position in the text (the beginning of a paragraph is the first non-blank
# character after a blank line).
#
# Arguments:
# w -		The text window in which the cursor is to move.
# pos -		Position at which to start search.

proc ::TextPlus::PrevPara {w pos} {
    set pos [$w index "$pos linestart"]
    while {1} {
	if {([$w get "$pos - 1 line"] eq "\n" && ([$w get $pos] ne "\n")) \
		|| $pos eq "1.0"} {
	    if {[regexp -indices -- {^[ \t]+(.)} \
		    [$w get $pos "$pos lineend"] -> index]} {
		set pos [$w index "$pos + [lindex $index 0] chars"]
	    }
	    if {[$w compare $pos != insert] || [lindex [split $pos .] 0]==1} {
		return $pos
	    }
	}
	set pos [$w index "$pos - 1 line"]
    }
}

# ::TextPlus::NextPara --
# Returns the index of the beginning of the paragraph just after a given
# position in the text (the beginning of a paragraph is the first non-blank
# character after a blank line).
#
# Arguments:
# w -		The text window in which the cursor is to move.
# start -	Position at which to start search.

proc ::TextPlus::NextPara {w start} {
    set pos [$w index "$start linestart + 1 line"]
    while {[$w get $pos] ne "\n"} {
	if {[$w compare $pos == end]} {
	    return [$w index "end - 1c"]
	}
	set pos [$w index "$pos + 1 line"]
    }
    while {[$w get $pos] eq "\n"} {
	set pos [$w index "$pos + 1 line"]
	if {[$w compare $pos == end]} {
	    return [$w index "end - 1c"]
	}
    }
    if {[regexp -indices -- {^[ \t]+(.)} \
	    [$w get $pos "$pos lineend"] -> index]} {
	return [$w index "$pos + [lindex $index 0] chars"]
    }
    return $pos
}

# ::TextPlus::ScrollPages --
# This is a utility procedure used in bindings for moving up and down
# pages and possibly extending the selection along the way.  It scrolls
# the view in the widget by the number of pages, and it returns the
# index of the character that is at the same position in the new view
# as the insertion cursor used to be in the old view.
#
# Arguments:
# w -		The text window in which the cursor is to move.
# count -	Number of pages forward to scroll;  may be negative
#		to scroll backwards.

proc ::TextPlus::ScrollPages {w count} {
    set bbox [$w bbox insert]
    $w yview scroll $count pages
    if {$bbox eq ""} {
	return [$w index @[expr {[winfo height $w]/2}],0]
    }
    return [$w index @[lindex $bbox 0],[lindex $bbox 1]]
}

# ::TextPlus::Transpose --
# This procedure implements the "transpose" function for text widgets.
# It tranposes the characters on either side of the insertion cursor,
# unless the cursor is at the end of the line.  In this case it
# transposes the two characters to the left of the cursor.  In either
# case, the cursor ends up to the right of the transposed characters.
#
# Arguments:
# w -		Text window in which to transpose.

proc ::TextPlus::Transpose w {
    set pos insert
    if {[$w compare $pos != "$pos lineend"]} {
	set pos [$w index "$pos + 1 char"]
    }
    set new [$w get "$pos - 1 char"][$w get  "$pos - 2 char"]
    if {[$w compare "$pos - 1 char" == 1.0]} {
	return
    }
    # ensure this is seen as an atomic op to undo
    set autosep [$w cget -autoseparators]
    if {$autosep} {
	$w configure -autoseparators 0
	$w edit separator
    }
    $w delete "$pos - 2 char" $pos
    $w insert insert $new
    $w see insert
    if {$autosep} {
	$w edit separator
	$w configure -autoseparators $autosep
    }
}

# ::tk_textCopy --
# This procedure copies the selection from a text widget into the
# clipboard.
#
# Arguments:
# w -		Name of a text widget.

proc ::tk_textCopy w {
    if {![catch {set data [$w get sel.first sel.last]}]} {
	clipboard clear -displayof $w
	clipboard append -displayof $w $data
    }
}

# ::tk_textCut --
# This procedure copies the selection from a text widget into the
# clipboard, then deletes the selection (if it exists in the given
# widget).
#
# Arguments:
# w -		Name of a text widget.

proc ::tk_textCut w {
    if {![catch {set data [$w get sel.first sel.last]}]} {
	clipboard clear -displayof $w
	clipboard append -displayof $w $data
	$w delete sel.first sel.last
    }
}

# ::tk_textPaste --
# This procedure pastes the contents of the clipboard to the insertion
# point in a text widget.
#
# Arguments:
# w -		Name of a text widget.

proc ::tk_textPaste w {
    global tcl_platform
    if {![catch {::tk::GetSelection $w CLIPBOARD} sel]} {
	set oldSeparator [$w cget -autoseparators]
	if {$oldSeparator} {
	    $w configure -autoseparators 0
	    $w edit separator
	}
	if {[tk windowingsystem] ne "x11"} {
	    catch { $w delete sel.first sel.last }
	}
	$w insert insert $sel
	if {$oldSeparator} {
	    $w edit separator
	    $w configure -autoseparators 1
	}
    }
}

# ::TextPlus::NextWord --
# Returns the index of the next word position after a given position in the
# text.  The next word is platform dependent and may be either the next
# end-of-word position or the next start-of-word position after the next
# end-of-word position.
#
# Arguments:
# w -		The text window in which the cursor is to move.
# start -	Position at which to start search.

if {$tcl_platform(platform) eq "windows"}  {
    proc ::TextPlus::NextWord {w start} {
	NextPos $w [NextPos $w $start tcl_endOfWord] \
		tcl_startOfNextWord
    }
} else {
    proc ::TextPlus::NextWord {w start} {
	NextPos $w $start tcl_endOfWord
    }
}

# ::TextPlus::NextPos --
# Returns the index of the next position after the given starting
# position in the text as computed by a specified function.
#
# Arguments:
# w -		The text window in which the cursor is to move.
# start -	Position at which to start search.
# op -		Function to use to find next position.

proc ::TextPlus::NextPos {w start op} {
    set text ""
    set cur $start
    while {[$w compare $cur < end]} {
	set text $text[$w get -displaychars $cur "$cur lineend + 1c"]
	set pos [$op $text 0]
	if {$pos >= 0} {
	    return [$w index "$start + $pos display chars"]
	}
	set cur [$w index "$cur lineend +1c"]
    }
    return end
}

# ::TextPlus::PrevPos --
# Returns the index of the previous position before the given starting
# position in the text as computed by a specified function.
#
# Arguments:
# w -		The text window in which the cursor is to move.
# start -	Position at which to start search.
# op -		Function to use to find next position.

proc ::TextPlus::PrevPos {w start op} {
    set text ""
    set cur $start
    while {[$w compare $cur > 0.0]} {
	set text [$w get -displaychars "$cur linestart - 1c" $cur]$text
	set pos [$op $text end]
	if {$pos >= 0} {
	    return [$w index "$cur linestart - 1c + $pos display chars"]
	}
	set cur [$w index "$cur linestart - 1c"]
    }
    return 0.0
}

# ::TextPlus::ScanMark --
#
# Marks the start of a possible scan drag operation
#
# Arguments:
# w -	The text window from which the text to get
# x -	x location on screen
# y -	y location on screen

proc ::TextPlus::ScanMark {w x y} {
    variable ::TextPlus::Priv
    $w scan mark $x $y
    set Priv(x) $x
    set Priv(y) $y
    set Priv(mouseMoved) 0
}

# ::TextPlus::ScanDrag --
#
# Marks the start of a possible scan drag operation
#
# Arguments:
# w -	The text window from which the text to get
# x -	x location on screen
# y -	y location on screen

proc ::TextPlus::ScanDrag {w x y} {
    variable ::TextPlus::Priv
    # Make sure these exist, as some weird situations can trigger the
    # motion binding without the initial press.  [Bug #220269]
    if {![info exists Priv(x)]} {
	set Priv(x) $x
    }
    if {![info exists Priv(y)]} {
	set Priv(y) $y
    }
    if {($x != $Priv(x)) || ($y != $Priv(y))} {
	set Priv(mouseMoved) 1
    }
    if {[info exists Priv(mouseMoved)] && $Priv(mouseMoved)} {
	$w scan dragto $x $y
    }
}

set ::TextPlus::Priv(afterId) {}

# ::tk::CancelRepeat --
# This procedure is invoked to cancel an auto-repeat action described
# by ::tk::Priv(afterId).  It's used by several widgets to auto-scroll
# the widget when the mouse is dragged out of the widget with a
# button pressed.
#
# Arguments:
# None.

proc ::TextPlus::CancelRepeat {} {
    variable ::TextPlus::Priv
    after cancel $Priv(afterId)
    set Priv(afterId) {}
}

bind TextPlus <<TextInsertionCursorMoved>> {
    if {[%W lexer set] ne ""} {
	TextPlus::MatchBrace %W insert
    }
}

proc ::TextPlus::MatchBrace {t index} {
    $t tag remove bracelight 1.0 end
    $t tag remove bracebad 1.0 end
    set bracesStyle [$t lexer cget -bracestyle]
    set braceAtCaret ""
    set braceOpposite ""
    set charBefore ""
    set styleBefore ""
    if {[$t compare $index > 1.0]} {
	set charBefore [$t get "$index - 1 chars"]
	set styleBefore [$t lexer styleat "$index - 1 chars"]
    }
    if {$charBefore ne "" && [string first $charBefore "{}\[]()"] != -1
	&& ($styleBefore eq $bracesStyle || $bracesStyle eq "")} {
	set braceAtCaret "$index - 1 chars"
    } else {
	set charAfter [$t get $index]
	set styleAfter [$t lexer styleat $index]
	if {$charAfter ne "" && [string first $charAfter "{}\[]()"] != -1
	    && ($styleAfter eq $bracesStyle || $bracesStyle eq "")} {
	    set braceAtCaret $index
	}
    }
    if {$braceAtCaret ne ""} {
	set braceOpposite [$t lexer bracematch $braceAtCaret]
	if {$braceOpposite eq ""} {
	    $t tag add bracebad $braceAtCaret
	} else {
	    $t tag add bracelight $braceAtCaret
	    $t tag add bracelight $braceOpposite
	}
    }
    return
}

bind TextPlus <Motion> {
    TextPlus::CheckCursor %W %x %y
    TextPlus::CheckFoldHighlight %W %x %y
}

# PUBLIC COMMAND
proc ::TextPlus::ForceCursor {w cursor} {
    if {[$w cget -cursor] ne $cursor} {
	$w configure -cursor $cursor
    }
    set ::TextPlus::Priv(cursor,$w) $cursor
}

# PUBLIC COMMAND
proc ::TextPlus::ReleaseCursor {w} {
    if {![info exists ::TextPlus::Priv(cursor,$w)]} return
    unset ::TextPlus::Priv(cursor,$w)
    set x [expr {[winfo pointerx $w] - [winfo rootx $w]}]
    set y [expr {[winfo pointery $w] - [winfo rooty $w]}]
    CheckCursor $w $x $y
}

proc ::TextPlus::CheckCursor {w x y} {
    # If set, the cursor is managed by client
    if {[info exists ::TextPlus::Priv(cursor,$w)]} return
    set id [$w identify $x $y]
    if {[scan $id "margin %s line %d" M L] > 0} {
	set cursor right_ptr
    } else {
	set cursor xterm
    }
    if {[$w cget -cursor] ne $cursor} {
	$w configure -cursor $cursor
    }
    return
}

proc ::TextPlus::MouseLeftMargin {w} {
    if {[info exists ::TextPlus::Priv(activeline,$w)]} {
	$w margin configure $::TextPlus::Priv(activeline,$w) -activeline ""
	$w linefoldhighlight ""
	array unset ::TextPlus::Priv activeline,$w
    }
    return
}

bind TextPlus <Leave> {
    ::TextPlus::MouseLeftMargin %W
}

proc ::TextPlus::CheckFoldHighlight {w x y} {
    variable Priv
    set id [$w identify $x $y]
    if {[scan $id "margin %s line %d" M L] == 2} {
	if {![info exists Priv(activeline,$w)] ||
		($M ne $Priv(activeline,$w)) ||
		([$w margin cget $M -activeline] != $L)} {
	    if {[info exists Priv(activeline,$w)] &&
		    ($M ne $Priv(activeline,$w))} {
		$w margin configure $Priv(activeline,$w) -activeline {}
	    }
	    $w margin configure $M -activeline $L
	    set Priv(activeline,$w) $M
	    if {$M eq "fold"} {
		$w linefoldhighlight $L.0
	    } else {
		$w linefoldhighlight ""
	    }
	}
    } else {
	MouseLeftMargin $w
    }
    return
}

proc ::TextPlus::MarginClick {w x y} {
    set id [$w identify $x $y]
    unset -nocomplain ::TextPlus::Priv(margin,$w)
    set Priv(pressX) $x ; # SelectTo needs it...
    if {[scan $id "margin %s line %d" M L] == 2} {
	# Allow focus in any case on Windows, because that will let the
	# selection be displayed even for state disabled text widgets.
	if {$::tcl_platform(platform) eq "windows" || [$w cget -state] eq "normal"} {focus $w}
	if {[$w cget -autoseparators]} {$w edit separator}

	if {$M eq "number"} {
	    $w tag remove sel 1.0 end
	    $w tag add sel $L.0 "$L.end + 1c"
	    catch {$w mark set insert sel.last}
	    $w mark set tk::anchor$w sel.last
	    set ::TextPlus::Priv(initialLine,$w) $L
	}
	if {$M eq "fold"} {
	    $w togglecontraction $L.0
	    CheckFoldHighlight $w $x $y
	}
	set ::TextPlus::Priv(margin,$w) $M
	return -code break
    }
    return
}

proc ::TextPlus::MarginMotion {w x y} {
    if {![info exists ::TextPlus::Priv(margin,$w)]} return
    # Click/drag in the line number margin to select whole lines
    if {$::TextPlus::Priv(margin,$w) eq "number"} {
	set L $::TextPlus::Priv(initialLine,$w)
	if {[$w compare @$x,$y < $L.0]} {
	    set first [$w index "@$x,$y linestart"]
	    set last [$w index "$L.end + 1c"]
	    $w mark set tk::anchor$w $last
	} else {
	    set first [$w index $L.0]
	    set last [$w index "@$x,$y lineend + 1c"]
	    $w mark set tk::anchor$w $first
	}
	if {[$w compare sel.first != $first] ||
		[$w compare sel.last != $last]} {
	    $w tag remove sel 1.0 $first $last end
	    $w tag add sel $first $last
	    catch {$w mark set insert sel.last}
	}
    }
    # Click/drag in fold margin does nothing except update highlight
    if {1 || $::TextPlus::Priv(margin,$w) eq "fold"} {
	CheckFoldHighlight $w $x $y
    }
    return -code break
}

proc ::TextPlus::MarginRelease {w x y} {
    unset -nocomplain ::TextPlus::Priv(margin,$w)
}

