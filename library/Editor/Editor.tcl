package require BWidget
package require snit
package require msgcat
package require Icons

snit::widget Editor {
  
  variable FileName ""
  variable FileTime 0

  component WEditor
  component ToolBar -public toolbar

  option -withtoolbar -default true -type snit::boolean -readonly yes
  option -defaultlanguage -default tol -configuremethod confDefaultLanguage
  option -withclosebutton -default true -type snit::boolean -readonly yes

  delegate method * to WEditor
  delegate option * to WEditor

  method confDefaultLanguage { option value } {
    language::validate $value
    set options($option) $value
  }

  constructor { args } {
    array set arrayArgs {
      -withsearchframe true
    }
    array set arrayArgs $args
    install WEditor using EditorBase $win.editor \
        -withsearchframe $arrayArgs(-withsearchframe)

    $self bind onModified
    array unset arrayArgs -withsearchframe
    $self configurelist [array get arrayArgs]
    if { $options(-withtoolbar) } {
      install ToolBar using ToolBarEditor $win.toolbar -withclosebutton $options(-withclosebutton)
      $ToolBar setTargetEditor $self
    }
    if { ![$self cget -withsearchframe] } {
      bind $WEditor <<ActivateSearch>> [mymethod propagateEvent ActivateSearch]
      bind $WEditor <<NextSearch>> [mymethod propagateEvent NextSearch]
    }
    $self gridComponents
  }
  
  method propagateEvent { e } {
    puts "propagateEvent $win <<$e>>"
    after 0 event generate $win <<$e>>
  }

  method gridComponents { } {
    if { [winfo exists $ToolBar] } {
      grid $ToolBar -row 0 -column 0 -sticky we 
    }
    grid $WEditor -row 1 -column 0 -sticky snew
    grid rowconfigure $win 1 -weight 1
    grid columnconfigure $win 0 -weight 1
  }

  method getLabelFileName {} {
    set name [file tail [$self getFileName]]
    if { $name eq "" } {
      set name [msgcat::mc "Unnamed"]
    }
    return $name
  }

  method askCloseModified { } {
    set name [$self getLabelFileName]
    set ret [MessageDlg $win.ask -title [msgcat::mc "Question"] \
                 -message [msgcat::mc "Buffer %s is modified; close anyway?" $name] \
                 -type okcancel]
    if { $ret==1 || $ret==-1 } {
      return 0
    }
    return 1
  }

  method checkCloseCurrentBuffer { } {
    if { [$WEditor edit modified] } {
      return [$self askCloseModified]
    }
    return 1
  }
  
  method  askOverrideRecentModified { } {
    set name [$self getLabelFileName]
    set ret [MessageDlg $win.ask -title [msgcat::mc "Question"] \
                 -message [msgcat::mc "%s has changed since visited or saved modified; save anyway?" $name] \
                 -type okcancel]
    if { $ret==1 || $ret==-1 } {
      return 0
    }
    return 1
  }

  method checkRecentModified { destFileName } {
    set _destFileName [file normalize $destFileName]
    if { $FileName eq $_destFileName && [file mtime $_destFileName] > $FileTime } {
      return [$self askOverrideRecentModified]
    }
    return 1
  }

  method clearCurrentBuffer {} {
    $self configure -undo no
    $self configure -startline {} -endline {}
    $self delete 1.0 end
    $self edit reset
    $self configure -undo yes
    $self edit modified 0
  }

  method isNewBuffer {} {
    return [expr {$FileName eq "" && $FileTime==0}]
  }

  typemethod getFileTypes { } {
    set filetypes [language::filetypes]
    lappend filetypes [list all *.*]
    return $filetypes
  }

  method cmpFileTypes { t0 type1 type2 } {
    set t1 [lindex $type1 0]
    set t2 [lindex $type2 0]
    if { $t1 eq $t0 } {
      return -1
    }  elseif { $t2 eq $t0 } {
      return 1
    } else {
      return [string compare $t1 $t2]
    }
  }

  method getSortedFileTypes { {fileType0 ""}} {
    if { $fileType0 eq "" } {
      set fileName [$self getFileName]
      set fileType0 [string range [file extension $fileName] 1 end]
      if { $fileType0 eq "" } {
        set fileType0 $options(-defaultlanguage)
      }
    }
    return [lsort -command [mymethod cmpFileTypes $fileType0] \
                [Editor getFileTypes]]
  }

  method readFile { fileName } {
    if { ![ file exists $fileName ] } {
      error [msgcat::mc "file '%s' does not exists" $fileName]
    }
    if { ![ file readable $fileName ] } {
      error [msgcat::mc "file '%s' is no readable" $fileName]
    }
    if { 0 } {
      set language [ EditorBase MatchLanguage $fileName ]
      if { $language ne [$self GetTextLanguage] } {
        $self SetTextLanguage $language
      }
    }
    $self setFileName $fileName
    $self configure -undo no
    $self configure -startline {} -endline {}
    $self delete 1.0 end
    $self edit reset
    set chan [ open $fileName ]
    while { ![ eof $chan ] } {
      $self insert end [read $chan 100000]
    }
    close $chan

    $self configure -undo yes

    $self edit modified 0
    #update
    $self mark set insert 1.0
    $self see insert

    return    
  }

  method getFileName {} {
    return $FileName
  }
  
  method getFileTime {} {
    return FileTime
  }

  method setFileName { name } {
    set newFileName [file normalize $name]
    if { $newFileName ne "" } {
      set FileTime [file mtime $newFileName]
      set newTextLanguage [ EditorBase MatchLanguage $newFileName ]
    } else {
      set FileTime 0
      set newTextLanguage $options(-defaultlanguage)
    }
    if { $newTextLanguage ne [$self GetTextLanguage] } {
      $self SetTextLanguage $newTextLanguage
    }
    set FileName $newFileName
    $self propagateEvent "FileNameChanged"
  }

  method cmdNewFile { } {
    if { ![$self checkCloseCurrentBuffer] } {
      return
    }
    $self clearCurrentBuffer
    $self setFileName ""
  }

  method cmdCloseCurrentBuffer { } {
    $self cmdNewFile
  }

  method getOpenFile {} {
    set defLan $options(-defaultlanguage)
    set fileSelected [tk_getOpenFile -multiple 0 \
                          -parent $win -title [msgcat::mc "Open file"] \
                          -filetypes [$self getSortedFileTypes $defLan]]
    return $fileSelected
  }

  method cmdOpenFile { } {
    set fileSelected [$self getOpenFile]
    if { $fileSelected ne "" } {
      if { [file normalize $fileSelected] eq $FileName } {
        # already loaded
        return
      }
      if { ![$self checkCloseCurrentBuffer] } {
        return
      }
      $self readFile $fileSelected
      #$self setFileName $fileSelected
    }
  }

  method cmdRevertFile {} {
    if { $FileName eq "" } {
      return
    }
    if { ![$self checkCloseCurrentBuffer] } {
      return
    }
    $self readFile $FileName
  }

  method cmdSaveBufferAs {} {
    set fileSelected [tk_getSaveFile -parent $win \
                          -title [msgcat::mc "Save as"] \
                          -filetypes [$self getSortedFileTypes]]
    if { $fileSelected eq "" || ![$self checkRecentModified $fileSelected] } {
      return
    }
    $self writeBufferToFile $fileSelected
  }

  method cmdSaveBuffer { } {
    set fileName [$self getFileName]
    if { $fileName eq "" } {
      $self cmdSaveBufferAs
    } else {
      if { ![$self checkRecentModified $fileName] } {
        return
      }
      $self writeBufferToFile $fileName
    }
  }

  method writeBufferToFile { fileName } {
    $WEditor writeBufferToFile $fileName
    $WEditor edit modified 0
    $self setFileName $fileName
  }

}

snit::widget ToolBarEditor {
  component editor
  component ToolFile
  component ToolSelection
  component ToolUndoRedo
  component Font
  component FrameStart
  component FrameEnd

  variable idlePending ""

  delegate method {editor *} to editor
  delegate option * to hull
  option -withclosebutton -default true -type snit::boolean -readonly yes

  variable TagContainer -array {}

  constructor { args } {
    set icons [Icons GetInstance]
    $self configurelist $args
    install FrameStart using frame $win.frameStart
    install FrameEnd using frame $win.frameEnd
    grid $FrameStart -row 0 -column 0 -sticky w
    grid $FrameEnd -row 0 -column 2 -sticky e
    grid columnconfigure $win 1 -weight 1
    set ToolFile [$self _newButtonBox ToolFile]
    $ToolFile add -image [$icons Get document_new] -relief link \
        -command [mymethod editor cmdNewFile] \
        -helptext [msgcat::mc "New file"]
    $ToolFile add -image [$icons Get document_open] -relief link \
        -command [mymethod editor cmdOpenFile] \
        -helptext [msgcat::mc "Open file"]
    $ToolFile add -image [$icons Get document_revert] -relief link \
        -command [mymethod editor cmdRevertFile] \
        -helptext [msgcat::mc "Revert file"]
    $ToolFile add -image [$icons Get gnome_document_save] -relief link \
        -command [mymethod editor cmdSaveBuffer] \
        -helptext [msgcat::mc "Save file"]
    $ToolFile add -image [$icons Get gnome_document_saveas] -relief link \
        -command [mymethod editor cmdSaveBufferAs] \
        -helptext [msgcat::mc "Save as file"]
    $self add Separator -tag ToolFile
    set ToolUndoRedo [$self _newButtonBox ToolUndoRedo]
    $ToolUndoRedo add -image [$icons Get old_edit_undo] \
        -relief link -state normal \
        -command [mymethod editor edit undo] \
        -helptext [msgcat::mc "Undo last operation"]
    $ToolUndoRedo add -image [$icons Get old_edit_redo] \
        -relief link -state normal \
        -command [mymethod cmdRedo] \
        -helptext [msgcat::mc "Redo last operation"]
    $self add Separator -tag ToolUndoRedo
    set ToolSelection [$self _newButtonBox ToolSelection]
    $ToolSelection add -image [$icons Get cut] -relief link -state disabled \
        -command [mymethod cmdCut] \
        -helptext [msgcat::mc "Cut selection"]
    $ToolSelection add -image [$icons Get copy] -relief link -state disabled \
        -command [mymethod cmdCopy] \
        -helptext [msgcat::mc "Copy selection"]
    $ToolSelection add -image [$icons Get paste] -relief link \
        -command [mymethod cmdPaste] \
        -helptext [msgcat::mc "Paste selection"]
    $self add Separator -tag ToolSelection
    set Font [$self add FontSelector]
    bind $Font <<FamilyChanged>> [mymethod cmdChangeFont]
    bind $Font <<SizeChanged>> [mymethod cmdChangeFont]
    if {$options(-withclosebutton)} {
      Button $FrameEnd.btnClose -image [$icons Get gnome_window_close] \
          -command [mymethod editor cmdCloseCurrentBuffer] \
          -relief link -helptext [msgcat::mc "Close current file"]
      grid $FrameEnd.btnClose -row 0 -column 0 -sticky e
    }
  }
  
  method _newButtonBox { tag } {
    return [$self add ButtonBox -spacing 0 -padx 1 -tag $tag]
  }

  method hide { tag } {
    foreach w $TagContainer($tag) {
      grid remove $w
    }
  }

  method add { wtype args } {
    foreach {ncol nrow} [grid size $FrameStart] break
    if { $wtype eq "Separator" } {
      set _args(-sticky) ns
      set _args(-orient) vertical
    } else {
      set _args(-sticky) w
    }
    set _args(-tag) ""
    array set _args $args
    set tag $_args(-tag)
    array unset _args -tag
    set sticky $_args(-sticky)
    array unset _args -sticky
    set child [eval $wtype $FrameStart.w${wtype}${ncol} [array get _args]]
    grid $child -row 0 -column $ncol -sticky $sticky
    lappend TagContainer($tag) $child
    return $child
  }

  method grid { w } {
    foreach {ncol ncol} [grid size [winfo parent $w]] break
    grid $w -row 0 -column $ncol -sticky w
  }

  method fillFontFamily { cb } {
    set lb [$cb getlistbox]
    foreach ff [lsort [font families]] {
      $lb insert end $ff
    }
  }

  method cmdCheckClipboard {} {
    #update idletasks
    #puts "ENTER: cmdCheckClipboard"
      if {[catch {::tk::GetSelection $editor CLIPBOARD} sel]} {
        $ToolSelection itemconfigure 2 -state disabled
      } else {
        $ToolSelection itemconfigure 2 -state normal
      }
      $self scheduleIdle
    #puts "LEAVE: cmdCheckClipboard"
  }

  method checkTarget { ed where} {
    if { $ed != $editor } {
      error "$where invoked on $ed while current editor is $editor"
    }
  }

  method onChangeSelection { ed } {
    $self checkTarget $ed "onChangeSelection"
    if { [llength [$editor tag ranges sel]] } {
      set state normal
    } else {
      set state disabled
    }
    foreach ib {0 1} {
      $ToolSelection itemconfigure $ib -state $state
    }
  }

  method onBufferModified { ed } {
    $self checkTarget $ed "onBufferModified"
    set dirty [$editor edit modified]
    $ToolUndoRedo itemconfigure 0 -state [expr {$dirty?"normal":"disabled"}]
    $ToolUndoRedo itemconfigure 1 -state [expr {[$editor hasredo]?"normal":"disabled"}]
  }

  method onRedoChanged { ed } {
    $self checkTarget $ed "onRedoChanged"
    puts "onRedoChanged: [$editor hasredo]"
    $ToolUndoRedo itemconfigure 1 -state [expr {[$editor hasredo]?"normal":"disabled"}]
  }

  method cmdRedo {} {
    puts "innvoking redo: [$editor getCountRedo]"
    $editor edit redo
    if {![$editor hasredo]} {
      $ToolUndoRedo itemconfigure 1 -state disabled
    }
  }

  method cmdCut {} {
    tk_textCut $editor
  }

  method cmdCopy {} {
    tk_textCopy $editor
  }

  method cmdPaste {} {
    tk_textPaste $editor
  }

  method cmdChangeFont { } {
    set family [$Font getFamily]
    set size [$Font getSize]
    array set fontOpts [$self editor getFontConfiguration]
    set fontOpts(-family) $family
    set fontOpts(-size) $size
    $editor changeFont [list -family $family -size $size]
  }

  method bindMyMethod { event action } {
    set script [mymethod $action $editor]
    set allBinded [$editor bind $event]
    if { [lsearch $allBinded $script] == -1 } {
      $editor bind $event +$script
    }
  }

  method setTargetEditor { ed } {
    set editor $ed
    $self bindMyMethod <<Selection>> onChangeSelection
    $self bindMyMethod <<Modified>> onBufferModified
    $self bindMyMethod <<RedoChanged>> onRedoChanged
    puts "$editor cget -font = '[$editor cget -font]'"
    array set font [$self editor getFontConfiguration]
    $Font setFamily $font(-family)
    $Font setSize $font(-size)
    # update state of undo/redo tool
    $self onBufferModified $editor
    # update state of selection tool
    $self onChangeSelection $editor
  }  
}
