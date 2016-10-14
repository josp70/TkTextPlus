package require stext

# Create the widget and scrollbar
stext .t -font {Times 12} -padx 4 -undo yes -yscrollcommand ".y set"
scrollbar .y -orient vertical -command ".t yview"
pack .t -side left -expand yes -fill both
pack .y -side right -expand yes -fill y

# Configure the margins
.t margin configure number -visible yes -background gray
.t margin configure fold -visible yes -foreground gray60 -activeforeground blue

# Set the lexer
.t lexer set tcl

# Set lists of keywords (from 1 to 9)
.t lexer keywords 1 { puts set return }
.t lexer keywords 2 { proc }

# Configure the tags used by the lexer
.t tag configure keyword1 -font {Times -16 bold}
.t tag configure keyword2 -font {Helvetica -16 bold} -foreground blue
.t tag configure string -foreground #7F007F
.t tag configure commentline -foreground #007F00

# Configure the tags used for brace-matching
.t tag configure bracelight -background gray80
.t tag configure bracebad -background #dd0000

.t tag raise sel


.t insert end {proc hw {} {
    set string "Hello"
    puts "$string, world!"
    return
}}

.t insert end "\n\n# The available lexers are:
#\t[join [.t lexer names] \n#\t]"

.t insert end "\n\n# The \"[.t lexer set]\" lexer uses these styles:
#\t[join [.t lexer stylenames] \n#\t]"

