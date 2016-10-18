package require BWidget

set families [lsort -unique [font families]]


pack [ComboBox .font -values $families -modifycmd {compo .font} \
          -command {cmd .font} \
          -validate key -validatecommand {validateEntry .font %P} \
          -autocomplete 1 -autopost 1 ]

bind .font.e <FocusOut> +[list entryFocusOut .font]

proc compo { cb } {
  puts "you have selected: [$cb get]"
  after 0 $cb.e selection clear
}

proc cmd { cb } {
  puts "cmd get: [$cb get]"
  puts "cmd getvalue: [$cb getvalue]"
  set idx [findClosest $cb [$cb get]]
  if { $idx == -1 } {
    $cb setvalue @0
  } else {
    $cb setvalue @$idx
  }
  after 0 $cb.e selection clear
} 

proc findClosest { cb v } {
  global families

  return [lsearch -glob $families "${v}*"]
}

proc entryFocusOut { cb } {
  set idx [findClosest $cb [$cb get]]
  $cb configure -validate none
  if { $idx == -1 } {
    $cb setvalue @0
  } else {
    $cb setvalue @$idx
  }
  $cb.e selection clear
  $cb configure -validate key
}

proc validateEntry { cb s } {
  global families

  set s [string trim $s \n]
  set idx [findClosest $cb $s]
  return [expr {$idx!=-1}]
}