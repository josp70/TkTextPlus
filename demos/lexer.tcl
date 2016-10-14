catch {namespace delete style}
catch {namespace delete language}

proc checkoption {option options {what "option"}} {
    set match [lsearch -all -glob $options $option*]
    set n [llength $match]
    if {$n != 1} {
	if {$n == 0} {
	    set type "unknown"
	    set matches $options
	} else {
	    set type "ambiguous"
	    foreach index $match {
		lappend matches [lindex $options $index]
	    }
	}
	set msg "$type $what \"$option\": must be "
	append msg [join [lrange $matches 0 end-1] ", "]
	append msg " or [lindex $matches end]"
	uplevel [list error $msg]
    }
    return [lindex $options [lindex $match 0]]
}

namespace eval style {
    variable vStyle
    variable vFontOptions {
	-family -size -weight -slant -underline
    }
    variable vValidOptions [concat $vFontOptions -foreground -background]
    variable vTrace {}
    variable vTraceID 0
}

proc style::validateoptlist {options {extra {}}} {
    variable vValidOptions

    set allopts [concat $vValidOptions $extra]
    set i 0
    foreach option $options {
	lset options $i [checkoption $option $allopts]
	incr i
    }
    return $options
}

proc style::validateoptval {options {extra {}}} {
    variable vValidOptions

    set allopts [concat $vValidOptions $extra]
    set i 0
    foreach {option value} $options {
	lset options $i [checkoption $option $allopts]
	incr i 2
    }
    return $options
}

proc style::cget {language name option} {
    variable vStyle
    variable vFontOptions

    language::validate $language
    set option [validateoptlist $option -font]

    if {![info exists vStyle($language,$name)]} return

    array set cfg $vStyle($language,$name)
    set result {}
    if {$option eq "-font"} {
	foreach option $vFontOptions {
	    if {[info exists cfg($option)]} {
		lappend result $option $cfg($option)
	    }
	}
    } else {
	if {[info exists cfg($option)]} {
	    lappend result $cfg($option)
	}
    }
    return $result
}

proc style::configure {language name args} {
    variable vStyle
    variable vFontOptions

    language::validate $language

    # Process 1 or more option-value pairs
    if {[llength $args] > 0} {
	set args [validateoptval $args -font]
	if {[llength $args] % 2} {
	    error "missing value to option \"[lindex $args end]\""
	}
	if {[info exists vStyle($language,$name)]} {
	    array set cfg $vStyle($language,$name)
	}
	array set cfg $args
	foreach {option value} [array get cfg] {
	    # StyleCfg $lang $style -font $font
	    if {$option eq "-font"} {
		if {$value eq ""} {
		    foreach fontOption $vFontOptions {
			array unset cfg $fontOption
		    }
		} else {
		    array set font [font actual $value]
		    foreach fontOption $vFontOptions {
			set cfg($fontOption) $font($fontOption)
		    }
		}
		array unset cfg $option ; # eliminate -font
		continue
	    }
	    if {$value eq ""} {
		array unset cfg $option
	    }
	}
	set vStyle($language,$name) [array get cfg]
	trace _changes
	return
    }

    # Return option-value pairs
    if {[info exists vStyle($language,$name)]} {
	return $vStyle($language,$name)
    }
    return {}
}

proc style::get {language style args} {
    variable vStyle
    variable vMap
    variable vValidOptions
    variable vFontOptions

    language::validate $language

    if {[llength $args] == 0} {
	set args $vValidOptions
    } else {
	set args [validateoptlist $args -font]
	set index [lsearch -exact $args -font]
	if {$index != -1} {
	    set args [eval lreplace {$args $index $index} $vFontOptions]
	}
    }

    array set cfg {}
    foreach option $args {
	# Recursively handle style mappings (possibly forever)
	if {[info exists vMap($language,$style,$option)]} {
	    foreach {lang2 style2} $vMap($language,$style,$option) {}
	    array set cfg [get $lang2 $style2 $option]
	}
	if {[info exists vStyle($language,$style)]} {
	    array unset cfg2
	    array set cfg2 $vStyle($language,$style)
	    if {[info exists cfg2($option)]} {
		set cfg($option) $cfg2($option)
	    }
	} elseif {[info exists vStyle(*,$style)]} {
	    array unset cfg2
	    array set cfg $vStyle(*,$style)
	    if {[info exists cfg2($option)]} {
		set cfg($option) $cfg2($option)
	    }
	}
    }
    return [array get cfg]
}

proc style::map {language style args} {
    variable vMap
    variable vValidOptions
    variable vFontOptions

    language::validate $language

    # style map $language
    if {[llength $args] == 0} {
	set result {}
	foreach name [array names vMap $language,$style,*] {
	    set len [string length $language,$style,]
	    lappend result [string range $name $len end]
	    eval lappend result $vMap($name)
	}
	return $result
    }

    if {[llength $args] < 2} {
	error "wrong # args: must be \"style map language style ?otherlanguage otherstyle? ?option ...?"
    }
    set lang2 [lindex $args 0]
    set style2 [lindex $args 1]

    language::validate $lang2

    if {[llength $args] == 2} {
	set args $vValidOptions
    } else {
	set args [validateoptlist [lrange $args 2 end] -font]
	set index [lsearch -exact $args -font]
	if {$index != -1} {
	    set args [eval lreplace {$args $index $index} $vFontOptions]
	}
    }
    foreach option $args {
	set vMap($language,$style,$option) [list $lang2 $style2]
    }
    trace _changes
    return
}

proc style::unmap {language style args} {
    variable vMap
    variable vValidOptions
    variable vFontOptions

    language::validate $language

    if {[llength $args] == 0} {
	set args $vValidOptions
    } else {
	set args [validateoptlist $args -font]
	set index [lsearch -exact $args -font]
	if {$index != -1} {
	    set args [eval lreplace {$args $index $index} $vFontOptions]
	}
    }
    foreach option $args {
	array unset vMap $language,$style,$option
    }
    trace _changes
    return
}

proc style::tracecmd {} {
    variable vTrace
    variable vTraceAfterID

    unset vTraceAfterID
    foreach {id script} $vTrace {
	if {[catch {
	    uplevel #0 $script
	} err]} {
	    # FIXME: background error
	    puts $err
	}    
    }
    return
}

proc style::trace {option args} {
    variable vTrace
    variable vTraceID
    variable vTraceAfterID

    switch -- [checkoption $option {add _changes remove}] {
	add {
	    if {[llength $args] != 1} {
		error "wrong # args: must be \"style trace add script\""
	    }
	    set id [incr vTraceID]
	    lappend vTrace $id [lindex $args 0]
	    return $id
	}
	_changes {
	    if {![info exists vTraceAfterID]} {
		set vTraceAfterID [after idle style::tracecmd]
	    }
	}
	remove {
	    if {[llength $args] != 1} {
		error "wrong # args: must be \"style trace remove id\""
	    }
	    set id [lindex $args 0]
	    set index [lsearch -exact $vTrace $id]
	    if {$index != -1} {
		set vTrace [lreplace $vTrace $index [incr index]]
	    }
	}
    }
    return
}

proc style {option args} {
    switch -- [checkoption $option {cget configure get map trace unmap}] {
	cget {
	    return [eval style::cget $args]
	}
	configure {
	    return [eval style::configure $args]
	}
	get {
	    return [eval style::get $args]
	}
	map {
	    return [eval style::map $args]
	}
	trace {
	    return [eval style::trace $args]
	}
	unmap {
	    return [eval style::unmap $args]
	}
    }
    return
}

namespace eval language {
    variable vFilePatterns
    variable vKeywords
    variable vLexer
    variable vValidOptions {-lexer -filepatterns -keywords1 -keywords2
	-keywords3 -keywords4 -keywords5 -keywords6 -keywords7
	-keywords8 -keywords9}
    variable vKnownLanguages
    set vKnownLanguages(*) 1
}

proc language::matchfile {file} {
    variable vFilePatterns

    set result {}
    set file [file tail $file]
    foreach {language patterns} [array get vFilePatterns] {
	foreach pattern $patterns {
	    if {[string match $pattern $file]} {
		lappend result $language
		break
	    }
	}
    }
    return $result
}

proc language::validateoptlist {options {extra {}}} {
    variable vValidOptions

    set allopts [concat $vValidOptions $extra]
    set i 0
    foreach option $options {
	lset options $i [checkoption $option $allopts]
	incr i 2
    }
    return $options
}

proc language::validateoptval {options {extra {}}} {
    variable vValidOptions

    set allopts [concat $vValidOptions $extra]
    set i 0
    foreach {option value} $options {
	lset options $i [checkoption $option $allopts]
	incr i 2
    }
    return $options
}

proc language::validate {language} {
    variable vKnownLanguages

    if {$language eq "*"} return
    set list [array names vKnownLanguages]
    return [checkoption $language $list "language"]
}

proc language::cget {language option} {
    variable vFilePatterns
    variable vKeywords
    variable vLexer

    validate $language
    set option [validateoptlist $option]

    switch -glob -- $option {
	-filepatterns {
	    return $vFilePatterns($language)
	}
	-lexer {
	    return $vLexer($language)
	}
	-keyword* {
	    scan $option "-keywords%d" n
	    return $vKeywords($language,$n)
	}
    }
}

proc language::configure {language args} {
    variable vFilePatterns
    variable vKeywords
    variable vLexer
    variable vValidOptions

    validate $language

    if {[llength $args] % 2} {
	error "missing value to option \"[lindex $args end]\""
    }

    # Process 1 or more option-value pairs
    if {[llength $args] > 0} {
	set args [validateoptval $args]
	foreach {option value} $args {
	    switch -glob -- $option {
		-filepatterns {
		    set vFilePatterns($language) $value
		}
		-lexer {
		    set vLexer($language) $value
		}
		-keyword* {
		    scan $option "-keywords%d" n
		    set vKeywords($language,$n) $value
		}
	    }
	}
	return
    }

    # Return option-value pairs
    set result {}
    foreach option $vValidOptions {
	lappend result $option
	switch -glob -- $option {
	    -filepatterns {
		lappend result $vFilePatterns($language)
	    }
	    -lexer {
		lappend result $vLexer($language)
	    }
	    -keywords* {
		scan $option "-keywords%d" n
		lappend result $vKeywords($language,$n)
	    }
	}
    }
    return $result
}

proc language::forget {language} {
    variable vFilePatterns
    variable vKeywords
    variable vKnownLanguages
    variable vLexer

    validate $language

    array unset vFilePatterns $language
    array unset vKeywords $language,*
    array unset vLexer $language
    array unset vKnownLanguages $language

    return
}

proc language::add {language args} {
    variable vFilePatterns
    variable vKeywords
    variable vKnownLanguages
    variable vLexer

    if {[info exists vKnownLanguages($language)]} {
	error "language \"$language\" exists"
    }

    set vKnownLanguages($language) 1
    set vFilePatterns($language) {}
    for {set n 1} {$n <=9} {incr n} {
	set vKeywords($language,$n) {}
    }
    set vLexer($language) {}

    if {[catch {
	eval { configure $language } $args
    } error]} {
	set errorInfo $::errorInfo
	forget $language
	return -code error -errorinfo $errorInfo $error
    }
    return
}

proc language {option args} {
    switch -- [checkoption $option {add cget configure forget matchfile}] {
	add {
	    return [eval language::add $args]
	}
	cget {
	    return [eval language::cget $args]
	}
	configure {
	    return [eval language::configure $args]
	}
	forget {
	    return [eval language::forget $args]
	}
	matchfile {
	    return [eval language::matchfile $args]
	}
    }
    return
}

