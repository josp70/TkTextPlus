#!../TclTk-8.4.13/bin/wish84

if {[catch {
    package require dbwin
}]} {
    proc dbwin {s} {
	puts [string trimright $s "\n"]
    }
}
proc dbwintrace {name1 name2 op} {
    dbwin $::dbwin
}
trace add variable ::dbwin write dbwintrace

if {[tk windowingsystem] ne "x11" && [info commands console] ne ""} {
    console eval {
	wm title . "TkTextPlus Demo - Console"
	.console configure -height 8
	if {[info tclversion] < 8.5} {
	    .console configure -font {Courier 9}
	}
	wm geometry . +0-100
    }
}
wm withdraw .

package require TkTextPlus

source [file join [file dirname [info script]] lexer.tcl]

set family2 Courier ; set size2 10

switch -- [tk windowingsystem] {
    aqua { set family1 {Lucida Grande} ; set size1 11 }
    classic { set family1 Geneva ; set size1 9 }
    win32 { set family1 Verdana ; set size1 9 }
    x11 {
	set family1 Helvetica ; set size1 12
	set family2 Courier ; set size2 12
    }
}

###
# Default styles for all languages
###

style configure * default -family $family1 -size $size1
style configure * monospace -family $family2 -size $size2
style configure * whitespace -foreground #D0D0D0
style configure * comment -foreground #007F00
style configure * keyword -weight bold -foreground #00007F
style configure * operator -weight bold
style configure * number -foreground #007F7F
style configure * character -foreground #7F007F
style configure * preprocessor -foreground #7F7F00
style configure * string -foreground #7F007F
style configure * stringeol -foreground #FFFFFF -background #E0C0E0
style configure * bracelight -foreground black -background gray
style configure * bracebad -foreground #FF0000 -underline yes

###
# Bash
###

language add bash -lexer bash -filepatterns {*.sh *.bsh configure configure.ac install-sh}

language configure bash -keywords1 { ar asa awk banner basename bash bc bdiff break
    bunzip2 bzip2 cal calendar case cat cc cd chmod cksum
    clear cmp col comm compress continue cp cpio crypt
    csplit ctags cut date dc dd declare deroff dev df diff diff3
    dircmp dirname do done du echo ed egrep elif else env
    esac eval ex exec exit expand export expr false fc
    fgrep fi file find fmt fold for function functions
    getconf getopt getopts grep gres hash head help
    history iconv id if in integer jobs join kill local lc
    let line ln logname look ls m4 mail mailx make
    man mkdir more mt mv newgrp nl nm nohup ntps od
    pack paste patch pathchk pax pcat perl pg pr print
    printf ps pwd read readonly red return rev rm rmdir
    sed select set sh shift size sleep sort spell
    split start stop strings strip stty sum suspend
    sync tail tar tee test then time times touch tr
    trap true tsort tty type typeset ulimit umask unalias
    uname uncompress unexpand uniq unpack unset until
    uudecode uuencode vi vim vpax wait wc whence which
    while who wpaste wstart xargs zcat
    chgrp chown chroot dir dircolors
    factor groups hostid install link md5sum mkfifo
    mknod nice pinky printenv ptx readlink seq
    sha1sum shred stat su tac unlink users vdir whoami yes }

style map bash commentline * comment
foreach n {1 2 3 4 5 6 7 8 9} {
    style map bash keyword$n * keyword
}
style configure bash error -background red
style configure bash scalar -foreground #00007F
style configure bash param -foreground red
style map bash backticks * string
style configure bash here_delim -foreground red
style configure bash here_q -foreground red


###
# TOL
###

language add tol -lexer tol -filepatterns {*.tol}

language configure tol -keywords1 {
  Stop Case Do For Eval EvalSet Include If While EQ NE LE GE LT GT AND OR NOT 
  Struct NameBlock Real Text Date Complex Polyn Ratio Matrix Set Serie TimeSet
  Code Anything Class
}

style map tol commentdoc * comment
style map tol commentline * comment
foreach n {1 2 3 4 5 6 7 8 9} {
    style map tol keyword$n * keyword
}

###
# C/C++
###

language add cpp -lexer cpp -filepatterns {*.c *.cc *.cpp *.cxx *.h *.hh *.hpp *.hxx *.sma}

language configure cpp -keywords1 { and and_eq asm auto bitand bitor bool break
    case catch char class compl const const_cast continue
    default delete do double dynamic_cast else enum explicit export extern false float for
    friend goto if inline int long mutable namespace new not not_eq
    operator or or_eq private protected public
    register reinterpret_cast return short signed sizeof static static_cast struct switch
    template this throw true try typedef typeid typename union unsigned using
    virtual void volatile wchar_t while xor xor_eq }

style map cpp commentdoc * comment
style map cpp commentline * comment
foreach n {1 2 3 4 5 6 7 8 9} {
    style map cpp keyword$n * keyword
}

###
# Lua
###

language add lua -lexer lua -filepatterns *.lua

language configure lua -keywords1 { and break do else elseif end false for function if
    in local nil not or repeat return then true until while }

language configure lua -keywords2 { _VERSION assert collectgarbage dofile error gcinfo loadfile loadstring
    print rawget rawset require tonumber tostring type unpack
    _ALERT _ERRORMESSAGE _INPUT _PROMPT _OUTPUT
    _STDERR _STDIN _STDOUT call dostring foreach foreachi getn globals newtype
    sort tinsert tremove
    _G getfenv getmetatable ipairs loadlib next pairs pcall
    rawequal setfenv setmetatable xpcall
    string table math coroutine io os debug
    load module select }

language configure lua -keywords3 { abs acos asin atan atan2 ceil cos deg exp
    floor format frexp gsub ldexp log log10 max min mod rad random randomseed
    sin sqrt strbyte strchar strfind strlen strlower strrep strsub strupper tan
    string.byte string.char string.dump string.find string.len
    string.lower string.rep string.sub string.upper string.format string.gfind string.gsub
    table.concat table.foreach table.foreachi table.getn table.sort table.insert table.remove table.setn
    math.abs math.acos math.asin math.atan math.atan2 math.ceil math.cos math.deg math.exp
    math.floor math.frexp math.ldexp math.log math.log10 math.max math.min math.mod
    math.pi math.pow math.rad math.random math.randomseed math.sin math.sqrt math.tan
    string.gmatch string.match string.reverse table.maxn
    math.cosh math.fmod math.modf math.sinh math.tanh math.huge }

language configure lua -keywords4 { openfile closefile readfrom writeto appendto
    remove rename flush seek tmpfile tmpname read write
    clock date difftime execute exit getenv setlocale time
    coroutine.create coroutine.resume coroutine.status
    coroutine.wrap coroutine.yield
    io.close io.flush io.input io.lines io.open io.output io.read io.tmpfile io.type io.write
    io.stdin io.stdout io.stderr
    os.clock os.date os.difftime os.execute os.exit os.getenv os.remove os.rename
    os.setlocale os.time os.tmpname
    coroutine.running package.cpath package.loaded package.loadlib package.path
    package.preload package.seeall io.popen }

style map lua commentdoc * comment
style map lua commentline * comment
foreach n {1 2 3 4 5 6 7 8 9} {
    style map lua keyword$n * keyword
}
style map lua literalstring * string

###
# Make
###

language add makefile -lexer makefile -filepatterns {Makefile Makefile.in}

style configure makefile target -foreground blue
style configure makefile variable -foreground #00007F
style configure makefile substitution -foreground #00007F

###
# Python
###

language add python -lexer python -filepatterns {*py *pyw}

language configure python -keywords1 { and assert break class continue def del elif
    else except exec finally for from global if import in is lambda None
    not or pass print raise return try while yield }

style map python commentblock * comment
style map python commentline * comment
foreach n {1 2 3 4 5 6 7 8 9} {
    style map python keyword$n * keyword
}
style map python triple * string
style map python tripledouble * string
style configure python classname -foreground #0000FF
style configure python defname -foreground #007F7F

###
# Tcl
###

language add tcl -lexer tcl -filepatterns {*.tcl *.test}

language configure tcl -keywords1  { after append array auto_execok
    auto_import auto_load auto_load_index auto_qualify
    beep bgerror binary break case catch cd clock
    close concat continue dde default echo else elseif
    encoding eof error eval exec exit expr fblocked
    fconfigure fcopy file fileevent flush for foreach format
    gets glob global history http if incr info
    interp join lappend lindex linsert list llength load
    loadTk lrange lreplace lsearch lset lsort memory msgcat
    namespace open package pid pkg::create pkg_mkIndex Platform-specific proc
    puts pwd re_syntax read regexp registry regsub rename
    resource return scan seek set socket source split
    string subst switch tclLog tclMacPkgSearch tclPkgSetup tclPkgUnknown tell
    time trace unknown unset update uplevel upvar variable
    vwait while }

language configure tcl -keywords2 { bell bind bindtags bitmap button canvas checkbutton clipboard
    colors console cursors destroy entry event focus font
    frame grab grid image Inter-client keysyms label labelframe
    listbox lower menu menubutton message option options pack
    panedwindow photo place radiobutton raise scale scrollbar selection
    send spinbox text tk tk_chooseColor tk_chooseDirectory tk_dialog tk_focusNext
    tk_getOpenFile tk_messageBox tk_optionMenu tk_popup tk_setPalette tkerror tkvars tkwait
    toplevel winfo wish wm }

style map tcl commentline * comment
style map tcl comment_box * comment
style map tcl comment_block * comment
foreach n {1 2 3 4 5 6 7 8 9} {
    style map tcl keyword$n * keyword
}
style map tcl word_in_quote * string
style configure tcl substitution -foreground #00007F
style configure tcl sub_brace -foreground #00007F
#style configure tcl modifier -foreground #00007F
style configure tcl expand -background #80ff00

proc GetANumber {parent cur min max nullok status} {
    global GAN

    set parent [winfo toplevel $parent]

    set w [toplevel .gan]
    wm title $w "Ye Olde Dialog"
    wm resizable $w no no
    wm transient $w $parent

    set prompt "Enter a number ($min - $max):"
    if {$nullok} {
	append prompt "\n(or an empty string)"
    }

    set f1 [frame $w.frame1 -borderwidth 0]
    label $f1.label -text $prompt -justify left
    entry $f1.entry -width 10
    $f1.entry insert end $cur
    pack $f1.label -side left
    pack $f1.entry -side right -padx {10 0}

    set f2 [frame $w.frame2 -borderwidth 0]
    button $f2.ok -text OK -command {set GAN ok} -width 10
    button $f2.cancel -text Cancel -command {set GAN cancel} -width 10
    pack $f2.ok -side left
    pack $f2.cancel -side right -padx {10 0}

    pack $f1 -side top -pady 8 -padx 10 -anchor w
    pack $f2 -side bottom -pady {0 8} -padx 10 -anchor e

    bind $w <Destroy> "set GAN cancel"
    bind $w <KeyPress-Return> "::tk::ButtonInvoke $f2.ok"
    bind $w <KeyPress-Escape> "::tk::ButtonInvoke $f2.cancel"

    wm geometry $w +[expr {[winfo x $parent] + 100}]+[expr {[winfo y $parent] + 100}]
    
    update
    focus $f1.entry

    while 1 {
	tkwait variable GAN
	if {$GAN eq "cancel"} {
	    break
	}
	set cur [$f1.entry get]
	if {$cur eq "" && $nullok} {
	    break
	}
	if {![string is integer -strict $cur]} {
	    bell
	    continue
	}
	if {$cur < $min || $cur > $max} {
	    bell
	    continue
	}
	break
    }

    catch {
	bind $w <Destroy> {}
	destroy $w
    }
    upvar $status _status
    set _status $GAN
    if {$GAN eq "cancel"} return
    return $cur
}

proc FindLanguageForFile {file} {
    return [lindex [language matchfile $file] 0]
}

proc GetStyleAtIndex {t index} {
    set text ""
    set style [$t lexer styleat $index]
    set ch [$t get $index]
    if {$ch == "\n"} {
	set ch "\\n"
    } elseif {$ch == "\r"} {
	set ch "\\r"
    } elseif {$ch == "\t"} {
	set ch "\\t"
    }
    if {$text ne ""} {
	append text " | "
    }
    append text "'$ch'"
    if {$style ne ""} {
	append text " $style"
    } else {
	append text " NO_STYLE"
    }
    return $text
}

proc ShowStyleAtInsert {t} {
    append text [$t index insert] " "
    if {[$t compare "insert-1c" != insert]} {
	append text [GetStyleAtIndex $t insert-1c] " | "
    }
    append text [GetStyleAtIndex $t insert]
    [winfo toplevel $t].status configure -text $text
    return
}

proc StyleToTagOptions {w language style basefont {force 0}} {
    array set font [font actual $basefont]
    set hasFont 0
    foreach spec [$w tag configure sel] {
	set cfg([lindex $spec 0]) [lindex $spec 3]
    }
    foreach {option value} [style get $language $style] {
	switch -- $option {
	    -family -
	    -size -
	    -slant {
		if {!$force} {
		    set font($option) $value
		    set hasFont 1
		}
	    }
	    -weight -
	    -underline {
		set font($option) $value
		set hasFont 1
	    }
	    -foreground -
	    -background {
		set cfg($option) $value
	    }
	}
    }
    if {$hasFont} {
	set cfg(-font) [array get font]
    }
    return [array get cfg]
}

proc StylesToTags {w mono} {
    set language $::gLanguageForText($w)
    if {$language eq ""} return
    set opts(-font) [$w cget -font]
    if {$mono} {
	array set opts [StyleToTagOptions $w $language monospace [$w cget -font]]
    } else {
	array set opts [StyleToTagOptions $w $language default [$w cget -font]]
    }
    set defaultFont $opts(-font)
    foreach style [concat [$w lexer stylenames] bracelight bracebad] {
	eval $w tag configure $style [StyleToTagOptions $w $language $style $defaultFont $mono]
    }
    $w configure -font $defaultFont
    return
}

proc ReadFile {w file} {

    $w.text configure -undo no
    $w.text configure -startline {} -endline {}
    $w.text delete 1.0 end
    $w.text edit reset

    set language [FindLanguageForFile $file]
    if {[lsearch -exact [$w.text lexer names] $language] != -1} {
	$w.text lexer set [language cget $language -lexer]
	set ::gLanguageForText($w.text) $language

	StylesToTags $w.text $::Options($w,monospace)

	foreach n {1 2 3 4 5 6 7 8 9} {
	    $w.text lexer keywords $n [language cget $language -keywords$n]
	}

	$w.text tag raise bracelight
	$w.text tag raise bracebad
	$w.text tag raise sel
	$w.text lexer configure -bracestyle operator
    } else {
	$w.text lexer set ""
    }

    set chan [open $file]
    while {![eof $chan]} {
	$w.text insert end [read $chan 10000]
    }
    close $chan

    $w.text configure -undo yes

    set ::gFileForWindow($w) $file
    SetWindowTitle $w
    foreach peer [$w.text peer names] {
	set ::gFileForWindow([winfo toplevel $peer]) $::gFileForWindow($w)
	set ::gLanguageForText($peer) $::gLanguageForText($w.text)
	$peer configure -font [$w.text cget -font]
    }
    return
}

proc OpenFile {w} {
    set path [tk_getOpenFile -parent $w]
    if {$path ne ""} {
	ReadFile $w $path
    }
    return
}

proc SetEdgeColumn {w} {
    set column [GetANumber $w [$w.text cget -edgecolumn] 0 128 false status]
    if {$column ne ""} {
	$w.text configure -edgecolumn $column
    }
    return
}

proc SetStartEnd {w option} {
    set min 1
    set start [$w.text cget -startline]
    set end [$w.text cget -endline]
    $w.text conf -startline {} -endline {}
    scan [$w.text index end] "%d." max
    incr max -1
    $w.text conf -startline $start -endline $end
    if {$option eq "-startline"} {
	if {$end ne ""} {
	    set max $end
	}
    } else {
	if {$start ne ""} {
	    set min $start
	}
    }
    set line [GetANumber $w [$w.text cget $option] $min $max true status]
    if {$status eq "ok"} {
	$w.text configure $option $line
    }
    return
}

proc ColorBoxClick {option nullok} {
    set label $::ColorBox(color,$option)
    set color [tk_chooseColor -parent [winfo toplevel $label] \
	-initialcolor [$label cget -background]]
    if {$color ne ""} {
	ColorBoxSet $option $color
	return 0
    }
    return 1
}

proc ColorBoxToggle {option} {
    if {$::ColorBox(empty,$option)} {
	ColorBoxSet $option ""
    } else {
	set ::ColorBox(empty,$option) [ColorBoxClick $option 0]
    }
    return
}

proc ColorBox {b title option {nullok 0}} {
    frame $b -borderwidth 0
    label $b.l -text $title
    label $b.c -borderwidth 2 -relief sunken -width 6 -height 2
    checkbutton $b.cb -text "None" -variable ::ColorBox(empty,$option) \
	-command "ColorBoxToggle $option"
    if {!$nullok} {
	$b.cb configure -state disabled
    }
    bind $b.c <ButtonPress-1> "ColorBoxClick $option $nullok"
    pack $b.l -side left -padx {4 8}
    pack $b.c -side right -padx {0 8}
    pack $b.cb -side right -padx {0 4}
    set ::ColorBox(color,$option) $b.c
    return $b
}

proc string_is_pixels {string} {
    if {[catch {
	winfo pixels . $string
	set result 1
    }]} {
	set result 0
    }
    return $result
}

proc NumberBoxValidate {option nullok string} {
    set t $::ConfigMargins(widget)
    set m $::ConfigMargins(margin)
    if {$nullok} {
	if {$string eq "" || [string_is_pixels $string]} {
	    if {$string ne [$t margin cget $m $option]} {
		$t margin configure $m $option $string
	    }
	}
    } else {
	if {[string_is_pixels $string]} {
	    if {$string ne [$t margin cget $m $option]} {
		$t margin configure $m $option $string
	    }
	}
    }
    return 1
}

proc NumberBox {b title option {nullok 0}} {
    frame $b -borderwidth 0
    label $b.l -text $title:
    entry $b.e -width 10 -textvariable ::NumberBox($option) \
	-validate key \
	-validatecommand "NumberBoxValidate $option $nullok %P"
    pack $b.l -side left -padx {4 8}
    pack $b.e -side right -padx {0 8}
    return $b
}

proc ColorBoxSet {option value} {
    set t $::ConfigMargins(widget)
    set m $::ConfigMargins(margin)
    if {$value ne [$t margin cget $m $option]} {
	$t margin configure $m $option $value
    }
    set empty 0
    if {$value eq ""} {
	set value [lindex [$::ColorBox(color,$option) configure -background] 3]
	set empty 1
    }
    $::ColorBox(color,$option) configure -background $value
    set ::ColorBox(empty,$option) $empty
    return
}

proc SelectMargin {w margin} {
    set lb $::ConfigMargins(listbox)
    if {[lsearch -exact [$w margin order] $margin] != [$lb curselection]} {
	set script [bind $lb <<ListboxSelect>>]
	bind $lb <<ListboxSelect>> {}
	$lb selection clear 0 100
	$lb selection set [lsearch -exact [$w margin order] $margin]
	bind $lb <<ListboxSelect>> $script
    }
    set ::ConfigMargins(margin) $margin
    foreach option {
	-background -activebackground
	-foreground -activeforeground
	-leftedge -rightedge
    } {
	ColorBoxSet $option [$w margin cget $margin $option]
    }
    set ::NumberBox(-width) [$w margin cget $margin -width]
    set ::NumberBox(-padx) [$w margin cget $margin -padx]
    set ::RadioGroup(-justify) [$w margin cget $margin -justify]
    set ::RadioGroup(-side) [$w margin cget $margin -side]
    set ::ConfigMargins(-visible) [$w margin cget $margin -visible]
    return
}

proc RadioGroupToggle {option} {
    set t $::ConfigMargins(widget)
    set m $::ConfigMargins(margin)
    if {$::RadioGroup($option) ne [$t margin cget $m $option]} {
	$t margin configure $m $option $::RadioGroup($option)
    }
    return
}

proc RadioGroup {f option title values} {
    set lf [labelframe $f -text $title -foreground blue]
    foreach value $values {
	radiobutton $lf.$value -text $value \
	    -variable ::RadioGroup($option) \
	    -value $value -command "RadioGroupToggle $option"
	pack $lf.$value -side top -anchor w -padx 10
    }
    return $lf
}

proc ToggleVisible {} {
    set t $::ConfigMargins(widget)
    set m $::ConfigMargins(margin)
    set lb $::ConfigMargins(listbox)
    set visible $::ConfigMargins(-visible)
    if {$visible ne [$t margin cget $m -visible]} {
	$t margin configure $m -visible $visible
	SetMarginListboxColors
    }
    return
}

proc SetMarginListboxColors {} {
    set t $::ConfigMargins(widget)
    set lb $::ConfigMargins(listbox)
    set i 0
    foreach margin [$t margin order] {
	if {[$t margin cget $margin -visible]} {
	    set fg ""
	} else {
	    set fg gray
	}
	$lb itemconfigure $i -foreground $fg -selectforeground $fg
	incr i
    }
}

proc ReorderMargins {direction} {
    set t $::ConfigMargins(widget)
    set lb $::ConfigMargins(listbox)
    set margins $::ConfigMargins(margins)
    set index [$lb curselection]
    if {$index + $direction < 0} return
    if {$index + $direction >= [llength $margins]} return
    set margin [$lb get $index]
    set margins [lreplace $margins $index $index]
    incr index $direction
    $lb selection clear 0 100
    $lb selection anchor $index
    $lb selection set $index
    set ::ConfigMargins(margins) [linsert $margins $index $margin]
    $t margin order $::ConfigMargins(margins)
    SetMarginListboxColors
    return
}

proc ConfigMargins {w {margin ""}} {
    set w2 .margins
    if {![winfo exists $w2]} {
	toplevel $w2
	wm title $w2 "TkTextPlus Demo - Margins"

	set fl [frame $w2.left -borderwidth 0]
	set fr [frame $w2.right -borderwidth 0]

	#
	# Margin list
	#

	set lf [labelframe $fl.margins -text "Margins" -foreground blue]

	set lb [listbox $lf.lb -width 20 -exportselection no \
	    -listvariable ::ConfigMargins(margins)]
	bind $lb <<ListboxSelect>> "SelectMargin $w \[%W get \[%W curselection]]"
	set ::ConfigMargins(listbox) $lb
	set f [frame $lf.buttons -borderwidth 0]
	button $f.up -text "Move Up" -width 10 -command "ReorderMargins -1"
	button $f.down -text "Move Down" -width 10 -command "ReorderMargins +1"
	pack $f.up $f.down -side top -pady {0 8}

	pack $lb -side left -expand yes -fill y -padx {4 6} -pady 4
	pack $f -side right -padx {0 6}

	checkbutton $f.visible -text "Visible" \
	    -variable ::ConfigMargins(-visible) \
	    -command "ToggleVisible"
	pack $f.visible -side top -pady 10

	pack $lf -side top -expand yes -fill x -pady 6

	#
	# Justify, Side
	#

	set f [frame $fl.js -borderwidth 0]

	set lf [RadioGroup $f.justify -justify "Justify" {left center right}]
	pack $lf -side left -expand yes -fill both -padx {0 10} -pady {0 6}

	set lf [RadioGroup $f.side -side "Side" {left right}]
	pack $lf -side left -expand yes -fill both -pady {0 6}

	pack $f -side top -expand yes -fill x -pady {0 6}

	#
	# Colors
	#

	set lf [labelframe $fr.lfColors -text "Colors" -foreground blue]
	set b1 [ColorBox $lf.bg "Background" -background]
	set b2 [ColorBox $lf.bgactive "Background (active)" -activebackground 1]
	set b3 [ColorBox $lf.fg "Foreground" -foreground]
	set b4 [ColorBox $lf.fgactive "Foreground (active)" -activeforeground 1]
	set b5 [ColorBox $lf.leftedge "Left edge" -leftedge 1]
	set b6 [ColorBox $lf.rightedge "Right edge" -rightedge 1]

	pack $b1 -side top -expand yes -fill x
	pack $b2 -side top -expand yes -fill x
	pack $b3 -side top -expand yes -fill x
	pack $b4 -side top -expand yes -fill x
	pack $b5 -side top -expand yes -fill x
	pack $b6 -side top -expand yes -fill x -pady {0 4}

	pack $lf -side top -expand yes -fill x -pady 6

	#
	# Width and padding
	#

	set lf [labelframe $fr.lfWidth -text "Width & Padding" -foreground blue]
	message $lf.msg -text "If the width is a negative number it indicates\
	    the number of characters wide. If the width is unspecified the\
	    margin will be as wide as needed." -width 250
	set b1 [NumberBox $lf.width "Width" -width 1]
	message $lf.msg2 -text "Padding has no effect on the width of a\
	    margin unless its width is unspecified." -width 250
	set b2 [NumberBox $lf.padx "Pad X" -padx]

	pack $lf.msg -side top -anchor w
	pack $b1 -side top -anchor w -padx {20 0} -pady 4
	pack $lf.msg2 -side top -anchor w
	pack $b2 -side top -anchor w -padx {20 0} -pady 4

	pack $lf -side top -expand yes -fill x -pady {0 6}

	pack $fl -side left -anchor n -padx 10
	pack $fr -side right -padx {0 10}
    }

    set ::ConfigMargins(widget) $w
    set ::ConfigMargins(margins) [$w margin order]

    # Select the initial margin
    if {$margin eq ""} {
	set margin [lindex [$w margin order] 0]
    }
    SelectMargin $w $margin

    # Hidden margins are grayed out
    SetMarginListboxColors

    after idle "raise $w2 ; focus $w2"

    return
}

proc InitMenus {w} {

    set mb [menu $w.menubar]

    set m1 [menu $mb.m1 -tearoff no]
    $m1 add command -label "New Window" -command [list NewWindow]
    $m1 add command -label "Create Peer" -command [list CreatePeer $w]
    $m1 add command -label "Open..." -command [list OpenFile $w]
    if {[info commands ::console] ne ""} {
	$m1 add separator
	$m1 add command -label "Console" -command {
	    if {[console eval {winfo ismapped .}]} {
		console hide
	    } else {
		console show
	    }
	}
    }
    $m1 add separator
    $m1 add command -label "Close" -command [list CloseWindow $w]
    $m1 add command -label "Exit" -command exit
    $mb add cascade -menu $m1 -label "Demo"

    set m2 [menu $mb.m2 -tearoff no]
    foreach option {-blockcursor -linenumbers -showeol -showtabs -wrap} {
	set ::Options($w,$option) [$w.text cget $option]
    }

	$m2 add command -label "Margins..." -command "ConfigMargins $w.text"

	set m3 [menu $m2.mLineNumbers -tearoff no]
	foreach value {document relative} {
	    $m3 add radiobutton -label $value \
		-variable Options($w,-linenumbers) -value $value \
		-command "$w.text configure -linenumbers \$Options($w,-linenumbers)"
	}
	$m2 add cascade -menu $m3 -label "Line Numbers"

	set m3 [menu $m2.mWrap -tearoff no]
	foreach value {none char word} {
	    $m3 add radiobutton -label $value \
		-variable Options($w,-wrap) -value $value \
		-command "$w.text configure -wrap \$Options($w,-wrap)"
	}
	$m2 add cascade -menu $m3 -label "Wrap"

    $m2 add checkbutton -label "Draw Newlines" \
	-variable Options($w,-showeol) \
	-command "$w.text configure -showeol \$Options($w,-showeol)"
    $m2 add checkbutton -label "Draw Tabs" \
	-variable Options($w,-showtabs) \
	-command "$w.text configure -showtabs \$Options($w,-showtabs)"
    $m2 add command -label "Edge Column..." -command [list SetEdgeColumn $w]
    $m2 add checkbutton -label "Block Cursor" \
	-variable Options($w,-blockcursor) \
	-command "$w.text configure -blockcursor \$Options($w,-blockcursor)"
if 0 { # Dangerous atm
    $m2 add separator
    $m2 add command -label "Start Line..." -command [list SetStartEnd $w -startline]
    $m2 add command -label "End Line..." -command [list SetStartEnd $w -endline]
}
    $m2 add separator
    set ::Options($w,monospace) 1
    $m2 add checkbutton -label "Monospace" \
	-variable Options($w,monospace) \
	-command "StylesToTags $w.text \$Options($w,monospace)"
    $mb add cascade -menu $m2 -label "Options"

    $w configure -menu $mb
    return
}

proc SetWindowTitle {w} {
    set title $::gFileForWindow($w)
    if {$title eq ""} {
	set title Untitled
    }
    set title "TkTextPlus Demo - $title"
    set peers [$w.text peer names]
    set numPeers [llength $peers]
    if {$numPeers} {
	incr numPeers
	set peers [lsort -dictionary [lappend peers $w]]
	set n 1
	foreach peer $peers {
	    wm title [winfo toplevel $peer] "$title \[Peer $n/$numPeers\]"
	    incr n
	}
    } else {
	wm title $w $title
    }
    return
}

proc NewWindow {{peer ""}} {

    set n 1
    while {[winfo exists .w$n]} { incr n }
    set w .w$n

    toplevel $w
    lappend options \
	-tabwidth 8 \
	-wrap none -linenumbers document -showtabs yes -showeol yes \
	-edgecolumn 78 -undo yes \
	-height 30 -width 90 -tabstyle wordprocessor \
	-inactiveselectbackground gray \
	-yscrollcommand "$w.yscroll set" \
	-xscrollcommand "$w.xscroll set"
    lappend options -background white ; # gray on X11
    if {$peer ne ""} {
	eval $peer peer create $w.text $options
	$w.text configure -font [$peer cget -font]
    } else {
	eval textplus $w.text $options
    }

    $w.text tag configure activeline -background gray95

    $w.text margin configure number -visible yes -background gray \
	-activebackground gray70
    $w.text margin configure fold -visible yes -foreground gray50 \
	-activeforeground blue
    $w.text margin configure marker1 -visible yes -background linen \
	-activebackground wheat1 -side right -justify center \
	-leftedge #f1d5b8
    # -background #f1f8f8 -activebackground #e1f0f0
    $w.text margin configure marker2 -visible yes -background #ffe7d7 \
	-activebackground #ffd5d5 -justify center -width 20 \
	-rightedge #ffd5d5

    $w.text linemarker create mark1 -text ":-)"
    $w.text linemarker create mark2 -text ";-\}"
    $w.text linemarker create stop -image stop

    set ::SymbolToggle1() mark1
    set ::SymbolToggle1(mark1) mark2
    set ::SymbolToggle1(mark2) ""

    set ::SymbolToggle2() stop
    set ::SymbolToggle2(stop) ""

#    $w.text debug yes

    bindtags $w.text [linsert [bindtags $w.text] 0 TestText]

    scrollbar $w.yscroll \
	-orient vertical -command "$w.text yview" \
	-highlightthickness 0
    scrollbar $w.xscroll \
	-orient horizontal -command "$w.text xview" \
	-highlightthickness 0

    label $w.status -anchor w -font {Courier 10}

    grid rowconfigure $w 0 -weight 1
    grid rowconfigure $w 1 -weight 0
    grid columnconfigure $w 0 -weight 1

    grid $w.text -row 0 -column 0 -sticky news
    grid $w.yscroll -row 0 -column 1 -sticky ns
    grid $w.xscroll -row 1 -column 0 -sticky we
    grid $w.status -row 2 -column 0 -sticky we -columnspan 2

    wm protocol $w WM_DELETE_WINDOW [list CloseWindow $w]

    InitMenus $w

    wm geometry $w +4+4
    wm title $w Untitled
    update

    set ::gFileForWindow($w) ""
    set ::gLanguageForText($w.text) ""

    StylesToTags $w.text $::Options($w,monospace) ; # set font
    set ::gStyleTrace($w) [style trace add "StylesToTags $w.text \$Options($w,monospace)"]

    return $w
}

proc CreatePeer {w} {
    set w2 [NewWindow $w.text]
    set ::gFileForWindow($w2) $::gFileForWindow($w)
    set ::gLanguageForText($w2.text) $::gLanguageForText($w.text)
    SetWindowTitle $w2
    return
}

proc CloseWindow {w} {
    set peers [$w.text peer names]
    unset ::gFileForWindow($w)
    unset ::gLanguageForText($w.text)
    style trace remove $::gStyleTrace($w)
    unset ::gStyleTrace($w)
    destroy $w
    if {[llength $peers]} {
	SetWindowTitle [winfo toplevel [lindex $peers 0]]
    }
    foreach w [winfo children .] {
	if {[winfo class $w] eq "Toplevel"} return
    }
    exit
}

bind TestText <<TextInsertionCursorMoved>> {
    if {[%W lexer set] ne ""} {
	ShowStyleAtInsert %W
    }
}

proc Motion {w x y} {
    if {[$w lexer set] eq ""} return
    set id [$w identify $x $y]
    if {[info exists ::Motion($w)] && $id eq $::Motion($w)} return
    set ::Motion($w) $id
    set text $id
    if {[scan $id "index %s" I] == 1} {
	append text " " [GetStyleAtIndex $w $I]
    }
    $w tag remove activeline 1.0 end
    if {[scan $id "margin %s line %d" M L] == 2} {
	$w tag add activeline $L.0 $L.end+1c
    }
    [winfo toplevel $w].status configure -text $text
    return
}
bind TestText <Motion> {
    Motion %W %x %y
}

bind TestText <ButtonPress-1> {
    if {[scan [%W identify %x %y] "margin %%s line %%d" M L] == 2} {
	if {$M eq "marker1"} {
	    set marker [%W linemarker set $L.0 $M]
	    %W linemarker set $L.0 $M $SymbolToggle1($marker)
	}
	if {$M eq "marker2"} {
	    set marker [%W linemarker set $L.0 $M]
	    %W linemarker set $L.0 $M $SymbolToggle2($marker)
	}
	if {[winfo exists .margins] && [winfo ismapped .margins]} {
	    SelectMargin %W $M
	}
    }
}

set dir [file dirname [info script]]
image create photo stop -file $dir/stop.gif
set dir [file dirname $dir]

#Window .w1 C:/Programming/cloids/src/gui.lua
#Window .w2 "C:/Tim's Folder/Programming/ProjectTk/Modules/editor.tcl"
#Window .w2 "C:/Programming/tk8.4.7/library/console.tcl"

ReadFile [NewWindow] [info script]
if {[file exists $dir/generic/tkTextDisp.c]} {
    ReadFile [NewWindow] $dir/generic/tkTextDisp.c
}
if {[file exists $dir/configure]} {
    ReadFile [NewWindow] $dir/configure
}
if {[file exists $dir/Makefile]} {
    ReadFile [NewWindow] $dir/Makefile
}

#Window .w4 D:/Programming/RUBY/lib/ruby/1.6/cgi.rb
#Window .w5 C:/Programming/Python24/Lib/aifc.py

