configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/pkgIndex.tcl.in"
  "${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl.in"
  )

file(GENERATE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl"
  INPUT  "${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl.in")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pkgIndex.tcl" "textplus.tcl"
  COMPONENT RUNTIME DESTINATION ${DEST_DIR})
