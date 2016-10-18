package require BWidget
package require snit
package require msgcat

snit::widget MultiEditor {

  variable LastPageIndex 0
  component Document
  component ActiveEditor
  component ToolBar -public toolbar
  component SearchBar

  option -defaultlanguage -default tol -configuremethod confDefaultLanguage

  delegate method * to ActiveEditor
  delegate option * to ActiveEditor

  method confDefaultLanguage { option value } {
    language::validate $value
    set options($option) $value
  }

  constructor { args } {
    $self createSearchBar
    $self createToolBar
    $self createNoteBook
    $self gridComponents
    $self configurelist $args
  }
  
  method getNewPageIndex { } {
    return [incr LastPageIndex]
  }

  method createNoteBook { } {
    set Document [NoteBook $win.document]
    set ActiveEditor [$self addNewEditor]
  }

  method addNewEditor { } {
    set page [$self getNewPageIndex]
    set pageFrame [$Document insert end $page]
    set editor [Editor $pageFrame.editor \
                    -defaultlanguage $options(-defaultlanguage) \
                    -withsearchframe 0 -withtoolbar 0]
    set ActiveEditor $editor
    $editor bind <<Modified>> [mymethod onModified]
    bind $editor <<ActivateSearch>> [mymethod onActivateSearch]
    bind $editor <<NextSearch>> [mymethod onNextSearch]
    puts "binding <<FileNameChanged>> on $editor"
    bind $editor <<FileNameChanged>> [mymethod onFileNameChanged]
    grid $editor -row 0 -column 0 -sticky snew
    grid rowconfigure $pageFrame 0 -weight 1
    grid columnconfigure $pageFrame 0 -weight 1
    $Document itemconfigure $page -text [$self getLabelFileName] \
        -raisecmd [mymethod onActivateEditor $editor] 

    $Document raise $page
    $Document see $page
    return $editor
  }

  method createAsSingleEditor { } {
    set Document [EditorBase $win.editor]
    set ActiveEditor $Document
  }

  method onActivateEditor { editor } {
    set ActiveEditor $editor
    $SearchBar setTargetEditor $self
    $ToolBar setTargetEditor $self
    $editor setFocus
  }

  method onModified { } {
    set p [expr {[$ActiveEditor edit modified]?"*":""}]
    $Document itemconfigure [$Document raise] \
        -text ${p}[$self getLabelFileName]
  }

  method gridComponents { } {
    grid $win.toolbar -row 0 -column 0 -sticky we 
    grid $Document -row 1 -column 0 -sticky snew
    grid $SearchBar -row 2 -column 0 -sticky ew
    grid remove $SearchBar
    grid rowconfigure $win 1 -weight 1
    grid columnconfigure $win 0 -weight 1
  }

  method onActivateSearch {} {
    grid $SearchBar
    $SearchBar setFocus
  }

  method onNextSearch {} {
    $SearchBar searchNext
  }

  method onCloseSearch {} {
    grid remove $SearchBar
    $ActiveEditor setFocus
  }

  method createSearchBar {} {
    install SearchBar using SearchEditor $win.search
    bind $SearchBar <<CloseSearch>> [mymethod onCloseSearch]
  }

  method createToolBar {} {
    install ToolBar using ToolBarEditor $win.toolbar
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
    if { [$ActiveEditor edit modified] } {
      return [$self askCloseModified]
    }
    return 1
  }

  method cmdNewFile { } {
    #profile on
    $self addNewEditor
    $self SetTextLanguage $options(-defaultlanguage)
    #$self clearCurrentBuffer
    #profile off ::profileNewFile
  }

  typemethod getFileTypes { } {
    set filetypes [language::filetypes]
    lappend filetypes [list all *.*]
    return $filetypes
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
      # check if the file is already opened
      foreach p [$Document pages] {
        set ed [$Document getframe $p].editor
        if { [$ed getFileName] eq [file normalize $fileSelected] } {
          $Document raise $p
          return
        }
      }
      if { ![$self isNewBuffer] || [$self edit modified] } {
        $self addNewEditor
      }
      $self readFile $fileSelected
    }
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

  method onFileNameChanged {} {
    puts "onFileNameChanged"
    $Document itemconfigure [$Document raise] \
        -text [$self getLabelFileName]
  }

  method getNumberOfBuffers {} {
    return [llength [$Document pages]] 
  }

  method cmdCloseCurrentBuffer { } {
    if { ![$self checkCloseCurrentBuffer] } {
      return
    }
    if { [$self getNumberOfBuffers] > 1 } {
      set page [$Document raise]
      set idx [$Document index $page]
      $Document delete $page
      set pages [$Document pages]
      if { $idx < [llength $pages] } {
        set page [lindex $pages $idx]
      } else {
        set page [lindex $pages [expr {$idx-1}]]
      }
      $Document raise $page
      $Document see $page
    } else {
      $self clearCurrentBuffer
      $self setFileName ""
    }
  }
}
