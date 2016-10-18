package require snit
package require TkTextPlus
package require Icons

set dump_insert 0
snit::widgetadaptor textplusUR {

  option -tabtospace -default false -type snit::boolean

  variable countRedo 0
  variable editFromStack 0

  method resetRedo {} {
    if { $countRedo > 0 } {
      set countRedo 0
      event generate $win <<RedoChanged>>
    }
  }

  method insert { args } {
    if { $::dump_insert } {
      puts '$args'
      if { $args eq "\t" } {
        puts "pille un tab"
      }
    }
    if { $args eq "\t" && $options(-tabtospace) } {
      set args [string repeat " " [$hull cget -tabwidth]]
    }
    if { [$self cget -undo] && !$editFromStack } {
      $self resetRedo
    }
    eval $hull insert $args
  }

  method delete { args } {
    if { [$self cget -undo] && !$editFromStack } {
      $self resetRedo
    }
    eval $hull delete $args
  }

  method edit { option args } {
    if { $option eq "redo" || $option eq "undo" } {
      set editFromStack 1
    }
    set result [eval $hull edit $option $args]
   
    if { ($option eq "redo" || $option eq "undo") && $editFromStack } {
      set editFromStack 0
    }
    if { $option eq "undo" } {
      incr countRedo
      if { $countRedo == 1 } {
        event generate $win <<RedoChanged>>
      }
    } elseif { $option eq "redo" } {
      incr countRedo -1
      if { $countRedo == 0 } {
        event generate $win <<RedoChanged>>
      }
    } elseif { $option eq "reset" } {
      $self resetRedo
    }
    return $result
  }

  method hasredo {} {
    return [expr {$countRedo>0}]
  }

  method getCountRedo {} {
    return $countRedo
  }

  delegate option * to hull
  delegate method * to hull  

  constructor { args } {
    eval installhull using textplus $args
    bind $win <Key-Tab> {
      if { [%W cget -tabtospace] } {
        %W insert insert [string repeat " " [%W cget -tabwidth]]
        break
      }
    }
  }
}

snit::widget EditorBase {
  
  typevariable family1
  typevariable size1
  typevariable family2
  typevariable size2

  component editor

  typeconstructor {
    $type InitFonts
    $type InitDefaultStyles
    $type InitLangBash
    $type InitLangTol
    $type InitLangCpp
    $type InitLangLua
    $type InitLangMake    
    $type InitLangPython
    $type InitLangTcl
  }

  typemethod InitFonts { } {
    set family2 Courier
    set size2 10

    switch -- [tk windowingsystem] {
      aqua {
        set family1 {Lucida Grande}
        set size1 11
      }
      classic {
        set family1 Geneva
        set size1 9
      }
      win32 {
        set family1 Verdana
        set size1 9
      }
      x11 {
        set family1 Helvetica
        set size1 12
	set family2 Courier
        set size2 12
      }
    }
  }

  typemethod InitDefaultStyles { } {
    ###
    # Default styles for all languages
    ###

    style::configure * default -family $family1 -size $size1
    style::configure * monospace -family $family2 -size $size2
    style::configure * whitespace -foreground #D0D0D0
    style::configure * comment -foreground #007F00
    style::configure * keyword -weight bold -foreground #00007F
    style::configure * operator -weight bold
    style::configure * number -foreground #007F7F
    style::configure * character -foreground #7F007F
    style::configure * preprocessor -foreground #7F7F00
    style::configure * string -foreground #7F007F
    style::configure * stringeol -foreground #FFFFFF -background #E0C0E0
    style::configure * bracelight -foreground black -background gray
    style::configure * bracebad -foreground #FF0000 -underline yes
  }

  typemethod InitLangBash { } {
    ###
    # Bash
    ###

    language::add bash -lexer bash \
        -filepatterns {*.sh *.bsh configure configure.ac install-sh}

    language::configure bash -keywords1 { 
      ar asa awk banner basename bash bc bdiff break
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
      sha1sum shred stat su tac unlink users vdir whoami yes
    }
    
    style::map bash commentline * comment
    foreach n {1 2 3 4 5 6 7 8 9} {
      style::map bash keyword$n * keyword
    }
    style::configure bash error -background red
    style::configure bash scalar -foreground #00007F
    style::configure bash param -foreground red
    style::map bash backticks * string
    style::configure bash here_delim -foreground red
    style::configure bash here_q -foreground red
  }

  typemethod InitLangTol { } {
    ###
    # TOL
    ###

    language::add tol -lexer tol -filepatterns {*.tol}

    language::configure tol -keywords1 {
      Stop Case Do For Eval EvalSet Include If While EQ NE LE GE LT GT AND OR
      NOT Struct NameBlock Real Text Date Complex Polyn Ratio VMatrix Matrix 
      Set Serie TimeSet Code Anything Class
    }

    style::map tol commentdoc * comment
    style::map tol commentline * comment
    foreach n {1 2 3 4 5 6 7 8 9} {
      style::map tol keyword$n * keyword
    }
  }
  
  typemethod InitLangCpp { } {
    ###
    # C/C++
    ###
    
    language::add cpp -lexer cpp -filepatterns {
      *.c *.cc *.cpp *.cxx *.h *.hh *.hpp *.hxx *.sma
    }

    language::configure cpp -keywords1 {
      and and_eq asm auto bitand bitor bool break
      case catch char class compl const const_cast continue
      default delete do double dynamic_cast else enum explicit export extern
      false float for friend goto if inline int long mutable namespace new
      not not_eq operator or or_eq private protected public
      register reinterpret_cast return short signed sizeof static static_cast
      struct switch template this throw true try typedef typeid typename
      union unsigned using virtual void volatile wchar_t while xor xor_eq
    }

    style::map cpp commentdoc * comment
    style::map cpp commentline * comment
    foreach n {1 2 3 4 5 6 7 8 9} {
      style::map cpp keyword$n * keyword
    }
  }

  typemethod InitLangLua { } {
    ###
    # Lua
    ###
    
    language::add lua -lexer lua -filepatterns *.lua

    language::configure lua -keywords1 {
      and break do else elseif end false for function if
      in local nil not or repeat return then true until while
    }

    language::configure lua -keywords2 {
      _VERSION assert collectgarbage dofile error gcinfo loadfile loadstring
      print rawget rawset require tonumber tostring type unpack
      _ALERT _ERRORMESSAGE _INPUT _PROMPT _OUTPUT
      _STDERR _STDIN _STDOUT call dostring foreach foreachi getn globals newtype
      sort tinsert tremove
      _G getfenv getmetatable ipairs loadlib next pairs pcall
      rawequal setfenv setmetatable xpcall
      string table math coroutine io os debug
      load module select
    }

    language::configure lua -keywords3 {
      abs acos asin atan atan2 ceil cos deg exp
      floor format frexp gsub ldexp log log10 max min mod rad random randomseed
      sin sqrt strbyte strchar strfind strlen strlower strrep strsub strupper
      tan string.byte string.char string.dump string.find string.len
      string.lower string.rep string.sub string.upper string.format
      string.gfind string.gsub table.concat table.foreach table.foreachi
      table.getn table.sort table.insert table.remove table.setn
      math.abs math.acos math.asin math.atan math.atan2 math.ceil math.cos
      math.deg math.exp math.floor math.frexp math.ldexp math.log math.log10
      math.max math.min math.mod math.pi math.pow math.rad math.random
      math.randomseed math.sin math.sqrt math.tan string.gmatch string.match
      string.reverse table.maxn math.cosh math.fmod math.modf math.sinh
      math.tanh math.huge
    }

    language::configure lua -keywords4 {
      openfile closefile readfrom writeto appendto
      remove rename flush seek tmpfile tmpname read write
      clock date difftime execute exit getenv setlocale time
      coroutine.create coroutine.resume coroutine.status
      coroutine.wrap coroutine.yield
      io.close io.flush io.input io.lines io.open io.output io.read io.tmpfile
      io.type io.write io.stdin io.stdout io.stderr
      os.clock os.date os.difftime os.execute os.exit os.getenv os.remove
      os.rename os.setlocale os.time os.tmpname
      coroutine.running package.cpath package.loaded package.loadlib
      package.path package.preload package.seeall io.popen
    }

    style::map lua commentdoc * comment
    style::map lua commentline * comment
    foreach n {1 2 3 4 5 6 7 8 9} {
      style::map lua keyword$n * keyword
    }
    style::map lua literalstring * string
  }

  typemethod InitLangMake { } {
    ###
    # Make
    ###
    
    language::add makefile -lexer makefile -filepatterns {Makefile Makefile.in}

    style::configure makefile target -foreground blue
    style::configure makefile variable -foreground #00007F
    style::configure makefile substitution -foreground #00007F
  }

  typemethod InitLangPython { } {
    ###
    # Python
    ###

    language::add python -lexer python -filepatterns {*.py *.pyw}

    language::configure python -keywords1 {
      and assert break class continue def del elif
      else except exec finally for from global if import in is lambda None
      not or pass print raise return try while yield
    }

    style::map python commentblock * comment
    style::map python commentline * comment
    foreach n {1 2 3 4 5 6 7 8 9} {
      style::map python keyword$n * keyword
    }
    style::map python triple * string
    style::map python tripledouble * string
    style::configure python classname -foreground #0000FF
    style::configure python defname -foreground #007F7F
  }

  typemethod InitLangTcl { } {
    ###
    # Tcl
    ###

    language::add tcl -lexer tcl -filepatterns {*.tcl *.test}

    language::configure tcl -keywords1  {
      after append array auto_execok
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
      vwait while
    }

    language::configure tcl -keywords2 {
      bell bind bindtags bitmap button canvas checkbutton clipboard
      colors console cursors destroy entry event focus font
      frame grab grid image Inter-client keysyms label labelframe
      listbox lower menu menubutton message option options pack
      panedwindow photo place radiobutton raise scale scrollbar selection
      send spinbox text tk tk_chooseColor tk_chooseDirectory tk_dialog
      tk_focusNext tk_getOpenFile tk_messageBox tk_optionMenu tk_popup
      tk_setPalette tkerror tkvars tkwait toplevel winfo wish wm
    }

    style::map tcl commentline * comment
    style::map tcl comment_box * comment
    style::map tcl comment_block * comment
    foreach n {1 2 3 4 5 6 7 8 9} {
      style::map tcl keyword$n * keyword
    }
    style::map tcl word_in_quote * string
    style::configure tcl substitution -foreground #00007F
    style::configure tcl sub_brace -foreground #00007F
    #style::configure tcl modifier -foreground #00007F
    style::configure tcl expand -background #80ff00
  }

  typemethod MatchLanguage { file } {
    return [ lindex [ language::matchfile $file ] 0 ]
  }

  variable TextLanguage "*"
  variable TagOptions -array {}

  option -monospace -default 1 -configuremethod ConfMonospace
  option -withsearchframe -default true -type snit::boolean -readonly yes

  delegate option * to editor
  delegate method * to editor  

  constructor { args } {
    install editor using textplusUR $win.editor
    $self createScrollBar
    $self defaultConfiguration
    # Apply an options passed at creation time.
    $self configurelist $args  
    $self createSearchFrame
    $self gridComponents
  }

  method getFontConfiguration { } {
    set fn [$editor cget -font]
    if {[llength $fn]>1} {
      return $fn
    } else {
      return [font configure $fn]
    }
  }

  method defaultConfiguration { } {
    $editor configure \
        -tabwidth 4 \
        -wrap none -linenumbers document -showtabs yes -showeol yes \
        -edgecolumn 78 -undo yes \
        -height 30 -width 90 -tabstyle wordprocessor \
        -inactiveselectbackground gray \
        -background white \
        -xscrollcommand "$win.scrollx set" \
        -yscrollcommand "$win.scrolly set"
    bind $editor <Control-s> [mymethod searchActivate]
    bind $editor <Key-F3> [mymethod searchNext]
    $editor tag configure activeline -background gray95
    $editor tag config hilite -background yellow
    $editor tag config hiliteall -background gray75
    $editor margin configure number -visible yes -background gray \
        -activebackground gray70
    $editor margin configure fold -visible yes -foreground gray50 \
        -activeforeground blue
    array set opts [$self StyleToTagOptions monospace [$self getFontConfiguration]]
    $editor configure -font [font actual $opts(-font)]
  }

  method gridComponents { } {
    grid $editor -row 0 -column 0 -sticky snew
    grid $win.scrolly -row 0 -column 1 -sticky ns
    grid $win.scrollx -row 1 -column 0 -sticky ew
    if { [ winfo exists $win.search ] } {
      grid $win.search -row 2 -column 0 -sticky snew -columnspan 2
      grid remove $win.search
    }
    grid rowconfigure $win 0 -weight 1
    grid columnconfigure $win 0 -weight 1
    $self setFocus
  }

  method getSelection {{defaultAll yes}} {
    set idxs [$editor tag ranges sel]
    if [llength $idxs] {
      return [$editor get [lindex $idxs 0] [lindex $idxs 1]]
    } elseif {$defaultAll} {
      return [$editor get 0.0 end-1c]
    } else {
      return ""
    }
  }

  method bind { args } {
    eval bind $editor $args
  }

  method setFocus {} {
    focus $editor
  }

  method createScrollBar { } {
    scrollbar $win.scrollx -orient h -command "$editor xview"
    scrollbar $win.scrolly -orient v -command "$editor yview"
  }

  method createSearchFrame { } {
    if { !$options(-withsearchframe) } {
      return
    }
    SearchEditor $win.search
    $win.search setTargetEditor $editor
    bind $win <<ActivateSearch>> [mymethod onActivateSearch]
    bind $win <<NextSearch>> [mymethod onNextSearch]
    bind $win.search <<CloseSearch>> [mymethod onCloseSearch]
  }

  method onCloseSearch {} {
    grid remove $win.search
    $self setFocus
  }

  method searchActivate {} {
    event generate $win <<ActivateSearch>>
  }

  method onActivateSearch {} {
    grid $win.search
    $win.search setFocus
  }

  method searchNext {} {
    #$self searchActivate
    event generate $win <<ActivateSearch>>
    event generate $win <<NextSearch>>
  }

  method onNextSearch {} {
    $win.search searchNext
  }

  method GetTextLanguage { } {
    return $TextLanguage
  }

  method delayedSetTextLanguage {} {
    $self StylesToTags
    
    foreach n {1 2 3 4 5 6 7 8 9} {
      $win lexer keywords $n [ language::cget $TextLanguage -keywords$n ]
    }
    
    $win tag raise bracelight
    $win tag raise bracebad
    $win tag raise sel
    $win lexer configure -bracestyle operator
  }

  method SetTextLanguage { language } {
    if { [ lsearch -exact [$win lexer names ] $language ] != -1 } {
      $win lexer set [ language::cget $language -lexer ]
      set TextLanguage $language
      
      after idle [mymethod delayedSetTextLanguage]
    } else {
      $win lexer set ""
    }
  }

  method ConfMonospace { o v } {
    if { [ string is boolean $v ] } {
      StylesToTags
    } else {
      error "invalid boolean value $v"
    }
  }

  method StyleToTagOptions { style basefont { force 0 } } {
    array set font [font actual $basefont]
    set hasFont 0
    foreach spec [ $win tag configure sel ] {
      set cfg([lindex $spec 0]) [lindex $spec 3]
    }
    foreach {option value} [ style::get $TextLanguage $style ] {
      switch -- $option {
        -family -
        -size -
        -slant {
          if { !$force } {
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
    if { $hasFont } {
      set cfg(-font) [array get font]
    }
    return [array get cfg]
  }

  method StylesToTags { } {
    if { $TextLanguage eq ""} return
    set opts(-font) [ $win cget -font ]
    array set wFont $opts(-font)
    if { $options(-monospace) } {
      array set opts [ $self StyleToTagOptions monospace [ $win cget -font ]]
    } else {
      array set opts [ $self StyleToTagOptions default [ $w cget -font ]]
    }
    set defaultFont $opts(-font)
    if { [lsearch $defaultFont -size] != -1 } {
      lappend defaultFont -size $wFont(-size)
    }
    puts "defaultFont = $defaultFont"
    foreach style [concat [ $win lexer stylenames ] bracelight bracebad ] {
      set TagOptions($style) [ $self StyleToTagOptions $style $defaultFont $options(-monospace) ]
      eval $win tag configure $style $TagOptions($style)
          
    }
    #$win configure -font $defaultFont
    #event generate $editor <<FontChanged>>
    return
  }

  method getTagOptions { style } {
    array get TagOptions $style
  }

  method changeFont { font } {
    $win configure -font $font
    foreach style [concat [ $win lexer stylenames ] bracelight bracebad ] {
      array unset opts
      array set opts $TagOptions($style)
      array unset fontOpts
      array set fontOpts $opts(-font)
      if { [info exists fontOpts(-family)] } {
        array set fontOpts $font
        set opts(-font) [array get fontOpts]
      }
      eval $win tag configure $style [array get opts]
    }
  }

  method writeBufferToFile { fileName } {
    set fd [open $fileName w]
    puts -nonewline $fd [$editor get 0.0 end-1c]
    close $fd
  }
}

snit::widget SearchEditor {

  # the editor where searching take place
  component editor

  variable SearchOption -array {
    IgnoreCase 0
    Forwards 1
    String ""
    ReplaceString ""
    HiliteAll 0
    Wrap 0
    ReplaceAll 0
  }

  method DumpSearchOption { } {
    parray SearchOption
  }

  delegate option * to hull
  delegate method * to hull

  constructor { } {
    set icons [Icons GetInstance]
    button $win.close -image [$icons Get gnome_window_close] -relief flat \
        -command [mymethod searchClose]
    label $win.lbSearch -text [msgcat::mc "Search"]:
    entry $win.entSearch -textvariable [myvar SearchOption(String)]
    bind $win.entSearch <Key-F3> [mymethod searchNext]
    bind $win.entSearch <Key-Escape> [mymethod searchClose]
    trace add variable [myvar SearchOption(String)] \
        write [mymethod searchNext]
    checkbutton $win.icase -text [msgcat::mc "Ignore case"] \
        -variable [myvar SearchOption(IgnoreCase)]
    checkbutton $win.forward -text [msgcat::mc "Forwards"] \
        -variable [myvar SearchOption(Forwards)]
    checkbutton $win.hiliteall -text [msgcat::mc "Hilite all"] \
        -variable [myvar SearchOption(HiliteAll)] \
        -command [mymethod searchHiliteAll]
    checkbutton $win.wrap -text [msgcat::mc "Wrap"] \
        -variable [myvar SearchOption(Wrap)]
    frame $win.sep1 -relief groove -borderwidth 2 -width 2 -height 2 
    label $win.lbReplace -text [msgcat::mc "Replace"]:
    entry $win.entReplace \
        -textvariable [myvar SearchOption(ReplaceString)] -background white
    button $win.replace -image [$icons Get edit_replace] -relief flat \
        -command [mymethod searchReplace]
    checkbutton $win.replaceall -text [msgcat::mc "Replace all"] \
        -variable [myvar SearchOption(ReplaceAll)]
    bind $win.entReplace <Key-F3> [mymethod searchNext]
    bind $win.entReplace <Key-Escape> [mymethod searchClose]
    grid $win.close     -row 0 -column 0 -sticky sn
    grid $win.lbSearch  -row 0 -column 1 -sticky ew
    grid $win.entSearch -row 0 -column 2 -sticky ew
    grid $win.icase -row 0 -column 3 -sticky e
    grid $win.forward -row 0 -column 4 -sticky e
    grid $win.hiliteall -row 0 -column 5 -sticky e
    grid $win.wrap -row 0 -column 6 -sticky e
    grid  $win.sep1 -row 0 -column 7 -sticky nse
    grid $win.lbReplace  -row 0 -column 8 -sticky ew
    grid $win.entReplace -row 0 -column 9 -sticky ew
    grid $win.replace -row 0 -column 10 -sticky ew
    grid $win.replaceall -row 0 -column 11 -sticky ew
    grid columnconfigure $win 12 -weight 1
  }

  method setTargetEditor { ed } {
    set editor $ed
    #bind $editor <<NextSearch>> [mymethod searchNext]
  }

  method getEditor {} {
    return $editor
  }

  method searchClose { } {
    $self clearTag hilite
    $self clearTag hiliteall
    event generate $win <<CloseSearch>>
  }

  method setFocus { } {
    focus $win.entSearch
    $win.entSearch configure -background [$editor cget -background]
  }

  method clearTag { tag } {
    foreach {from to} [$editor tag ranges $tag] {
      $editor tag remove $tag $from $to
    }
  }

  method searchHiliteAll { } {
    if { $SearchOption(String) eq "" } {
      return
    }
    if { !$SearchOption(HiliteAll) } {
      $self clearTag hiliteall
      return
    }
    set sizes {}
    set starts [$editor search -count sizes -all $SearchOption(String) 0.0]
    foreach s $starts n $sizes {
      $editor tag add hiliteall $s $s+${n}c
    }
  }

  method searchNotFound { } {
    $win.entSearch configure -background red
    $win.entSearch configure -foreground white
  }

  method searchFound { } {
    $win.entSearch configure -background [$editor cget -background]
    $win.entSearch configure -foreground black
  }

  method searchReplace { } {
    if { $SearchOption(ReplaceAll) } {
      if { $SearchOption(String) eq "" } {
        return
      }
      set sizes {}
      set saveInsert [$editor index insert]
      set starts [$editor search -count sizes -all $SearchOption(String) 0.0]
      $editor configure -autoseparator 0
      $editor edit separator
      foreach s $starts n $sizes {
        $editor delete $s $s+${n}c
        $editor insert $s $SearchOption(ReplaceString)
      }
      $editor edit separator
      $editor configure -autoseparator 1
      $editor edit separator
      $editor mark set insert $saveInsert
      $editor see insert
      $self searchNext
    } else {
      set pos [$editor tag ranges hilite]
      if { [llength $pos] } {
        $editor configure -autoseparator 0
        $editor edit separator
        $editor delete [lindex $pos 0] [lindex $pos 1]
        $editor insert [lindex $pos 0] $SearchOption(ReplaceString)
        $editor edit separator
        $editor configure -autoseparator 1
        $editor edit separator
	$self searchNext
        return 1
      } else {
        return 0
      }
    }
  }

  method searchNext { args } {
    $self clearTag hilite
    if { $SearchOption(String) eq "" } {
      $self clearTag hiliteall
      return
    }
    set cmd [list $editor search -count n]
    if { $SearchOption(IgnoreCase) } {
      lappend cmd "-nocase"
    }
    if { $SearchOption(Forwards) } {
      lappend cmd "-forwards"
    } else {
      lappend cmd "-backwards"
    }
    
    lappend cmd -- $SearchOption(String)
    if { [llength $args] } {
      # triggered by trace
      if { $SearchOption(HiliteAll) } {
        $self clearTag hiliteall
        $self searchHiliteAll
      }
      if { !$SearchOption(Forwards) } {
        set relPos "+1c"
      } else {
        set relPos ""
      }
    } else {
      # triggered by F3
      if { $SearchOption(Forwards) } {
        set relPos "+1c"
      } else {
        set relPos ""
      }
    }
    lappend cmd "insert${relPos}"
    if { !$SearchOption(Wrap) } {
      if { $SearchOption(Forwards) } {
        lappend cmd end
      } else {
        lappend cmd 0.0
      }
    }
    set pos [eval $cmd]
    if {$pos ne ""} {
      $self searchFound
      $editor mark set insert $pos
      $editor see insert
      $editor tag add hilite $pos $pos+${n}c
      $editor tag raise hilite
    } else {
      $self searchNotFound
    }
  }
}

