package require BWidget
package require snit

snit::widget FontSelector {
  component cbFamily -public family
  component cbSize -public size

  typevariable FontFamilies
  typevariable FontSizes {4 5 6 7 8 9 10 11 12 13 14 15 16 \
                              17 18 19 20 21 22 23 24}

  variable CurrentFamily ""
  variable CurrentSize 12

  typeconstructor {
    set FontFamilies [lsort -unique [font families]]
  }

  typemethod getFontFamilies {} {
    return $FontFamilies
  }

  typemethod getFontSizes {} {
    return $FontSizes
  }

  delegate method * to hull
  delegate option * to hull

  constructor { args } {
    install cbFamily using ComboBox $win.cbFamily \
        -autocomplete 1 -autopost 1 \
        -values [$type getFontFamilies] -validate key \
        -validatecommand [mymethod validateEntry %P "Family"] \
        -command [mymethod onAccept $win.cbFamily "Family"] \
        -modifycmd [mymethod onSelect $win.cbFamily "Family"]

    bind $cbFamily.e <FocusOut> +[mymethod entryFocusOut $win.cbFamily "Family" ]
    bind $cbFamily.e <Escape> +[mymethod onCancel $cbFamily.e "Family"]

    install cbSize using ComboBox $win.cbSize \
        -autocomplete 1 -autopost 1 -width 4 \
        -values [$type getFontSizes] -validate key \
        -validatecommand [mymethod validateEntry %P "Size"] \
        -command [mymethod onAccept $win.cbSize "Size"] \
        -modifycmd [mymethod onSelect $win.cbSize "Size"]

    if { [string compare $::tk_version "8.4"] <= 0 } {
      $cbFamily configure -entrybg white
      $cbSize configure -entrybg white
    }

    bind $cbSize.e <FocusOut> +[mymethod entryFocusOut $win.cbSize "Size" ]
    bind $cbSize.e <Escape> +[mymethod onCancel $cbSize.e "Size"]

    $self setFamily [lindex [$type getFontFamilies] 0]
    $self setSize 12

    $self configurelist $args
    grid $cbFamily -row 0 -column 0 -sticky w
    grid $cbSize -row 0 -column 1 -sticky w
  }

  method getFamily {} {
    return $CurrentFamily
  }

  method getSize {} {
    return $CurrentSize
  }

  method setComboIndex { cb idx } {
      $cb configure -validate none
      $cb setvalue @$idx
      $cb configure -validate key
      set lb [$cb getlistbox]
      $lb selection clear 0 end
      $lb activate $idx
      $lb selection anchor $idx
      $lb selection set $idx
  }

  method setFamily { family } {
    puts "setFamily $family"
    set idx [$self findClosestFamily $family]
    puts "idx = $idx"
    if { $idx != -1 } {
      $self setComboIndex $cbFamily $idx
      if { $family ne $CurrentFamily } {
        set CurrentFamily $family
        after 0 event generate $win <<FamilyChanged>>
      }
    }
  }

  method setSize { size } {
    puts "setSize $size"
    set idx [$self findClosestSize $size]
    if { $idx != -1 } {
      $self setComboIndex $cbSize $idx
      if { $size ne $CurrentSize } {
        set CurrentSize $size
        after 0 event generate $win <<SizeChanged>>
      }
    }
  }

  method findClosestFamily { family } {
    return [lsearch -glob [$type getFontFamilies] "${family}*"]
  }

  method findClosestSize { size } {
    if { $size eq "" || ![string is double $size] } {
      return -1
    }
    set size [expr {round($size)}]
    set sizes [$type getFontSizes]
    set min [lindex $sizes 0]
    set max [lindex $sizes end]
    set s [expr {$size-$min}]
    if { $s < 0 } {
      return 0
    } elseif { $s >= [llength $sizes] } {
      return [expr {[llength $sizes]-1}]
    } else {
      return $s
    }
  }
  
  method validateEntry { s what } {
    set s [string trim $s \n]
    set idx [$self findClosest$what $s]
    return [expr {$idx!=-1}]
  }

  method entryFocusOut { cb what } {
    set idx [$self findClosest${what} [$cb get]]
    $cb configure -validate none
    if { $idx == -1 } {
      $cb setvalue @0
    } else {
      $cb setvalue @$idx
    }
    $cb configure -validate key
    $self onSelect $cb $what
  }

  method checkChangeFamily  { family } {
    return [expr {$family ne $CurrentFamily}]
  }

  method onCancel { cb what } {
    $self set${what} [$self get${what}]
  }

  method onSelect { cb what } {
    set new [$cb get]
    $self set$what $new
    after 0 $cb.e selection clear
  }

  method onAccept { cb what } {
    set idx [$self findClosest${what} [$cb get]]
    if { $idx == -1 } {
      $cb setvalue @0
    } else {
      $cb setvalue @$idx
    }
    $self onSelect $cb $what
  }

}