package require Img
package require snit

snit::type Icons {

  typevariable Singleton ""
  typevariable DefaultSize "16"

  variable Cache
  variable File

  typemethod GetInstance { } {
    return $Singleton
  }

  typeconstructor {
    set Singleton [$type %AUTO%]
  }

  constructor { args } {
    if { $Singleton ne "" } {
      error "The unique instance for this class could only be accessed by calling GetInstance"
    }
  }
  
  method Dump {} {
    parray File
  }

  method Get { name args } {
    set options(-size) $DefaultSize
    array set options $args

    if { ![info exists Cache($name,$options(-size))] } {
      if { [info exists File($name,$options(-size))] } {
        set img [image create photo ${name}-$options(-size) \
                     -file $File($name,$options(-size))]
        set Cache($name,$options(-size)) $img
      } else {
        error "icon $name with size $options(-size) not found"
      }
    }
    return $Cache($name,$options(-size))
  }

  method Search { dir } {
    variable File

    #puts "Search: $dir"
    foreach f [glob -nocomplain -dir $dir *.png *.gif] {
      #puts $f
      # {^(.+)-(\d+).png$} 
      if { [regexp {^([^.-]+)(?:(?:-)(\d+))?.(png|gif)$} [file tail $f] ==> n s] } {
        #puts "set File($n,$s) [file normalize $f]"
        set File($n,$s) [file normalize $f]
      } else {
        set File([string range [file tail $f] 0 end-4],) [file normalize $f]
        #puts "file $f does not match regular expression"
      }
    }
    #puts "icons initialized"
  }
}
