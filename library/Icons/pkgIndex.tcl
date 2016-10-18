package ifneeded Icons 1.0 [string map [list %d $dir] {
  source [file join "%d" Icons.tcl]
  [Icons GetInstance] Search [file join "%d" Images]
  package provide Icons 1.0
}]