lappend auto_path /home/jsperez/TOL/FROM_SVN/tolp/trunk/tolbase/lib
package require autoscroll

source lexer.tcl
source EditorBase.tcl

EditorBase .ed \
    -tabwidth 4 \
    -wrap none -linenumbers document -showtabs yes -showeol yes \
    -edgecolumn 78 -undo yes \
    -height 30 -width 90 -tabstyle wordprocessor \
    -inactiveselectbackground gray \
    -background white \
    -xscrollcommand ".scrollx set" \
    -yscrollcommand ".scrolly set"

.ed tag configure activeline -background gray95

.ed margin configure number -visible yes -background gray \
    -activebackground gray70
.ed margin configure fold -visible yes -foreground gray50 \
    -activeforeground blue

scrollbar .scrollx -orient h -command ".ed xview"
scrollbar .scrolly -orient v -command ".ed yview"

grid .ed -row 0 -column 0 -sticky snew
grid .scrollx -row 1 -column 0 -sticky ew
grid .scrolly -row 0 -column 1 -sticky ns
grid rowconfigure . 0 -weight 1
grid columnconfigure . 0 -weight 1
#autoscroll::autoscroll .scrollx
#autoscroll::autoscroll .scrolly

.ed ReadFile /home/jsperez/eval.tol
.ed SetLanguage tol