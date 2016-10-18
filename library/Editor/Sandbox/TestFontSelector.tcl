source FontSelector.tcl

FontSelector .font
pack .font

bind .font <<FamilyChanged>> {puts "font family change to [.font getFamily]"}
bind .font <<SizeChanged>> {puts "font size change to [.font getSize]"}