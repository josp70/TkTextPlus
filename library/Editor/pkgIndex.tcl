package ifneeded Editor 1.0 [string map [list %d $dir] {
  source [file join "%d" Lexer.tcl]
  source [file join "%d" FontSelector.tcl]
  source [file join "%d" EditorBase.tcl]
  source [file join "%d" Editor.tcl]
  source [file join "%d" MultiEditor.tcl]
  package provide Editor 1.0
}]
