set __cwd__ [file normalize [file dirname [info script]]]

lappend auto_path [file join $__cwd__ .. ..]

package require autoscroll
package require Editor


MultiEditor .ed 

set icons [Icons GetInstance]

.ed toolbar add Separator
set bb [.ed toolbar add ButtonBox -spacing 0 -padx 1]
$bb add -image [$icons Get compile] -command "puts compile" \
  -relief link -helptext [msgcat::mc "Compile file"]
$bb add -image [$icons Get uncompile] -command "puts uncompile" \
  -relief link -helptext [msgcat::mc "Uncompile file"]
$bb add -image [$icons Get spell_check] -command "puts {check syntax}" \
  -relief link -helptext [msgcat::mc "Check syntaxis"]

grid .ed -row 0 -column 0 -sticky snew
grid rowconfigure . 0 -weight 1
grid columnconfigure . 0 -weight 1

#.ed readFile /home/jsperez/eval.tol
#.ed readFile Icons.tcl
#.ed SetLanguage tol
