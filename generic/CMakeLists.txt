include_directories(${TCL_INCLUDE_PATH} ${TCL_INCLUDE_PATH}/tcl-private/generic
  ${TK_INCLUDE_PATH} ${TK_INCLUDE_PATH}/tk-private/generic
  ${TK_INCLUDE_PATH}/tk-private/unix ${CMAKE_CURRENT_SOURCE_DIR})

add_library(TkTextPlus SHARED
  LexBash.c LexCPP.c LexLua.c LexMake.c LexPython.c LexTcl.c LexTOL.c
  tkText.c tkTextBTree.c tkTextDisp.c tkTextHighlight.c tkTextImage.c
  tkTextIndex.c tkTextLineMarker.c tkTextMargin.c tkTextMark.c tkTextTag.c
  tkTextWind.c tkUndo.c)

set_target_properties(TkTextPlus PROPERTIES OUTPUT_NAME 
  ${PACKAGE_NAME}${TKTEXTPLUS_VERSION_MAJOR}.${TKTEXTPLUS_VERSION_MINOR})

target_link_libraries(TkTextPlus
  ${TCL_STUB_LIBRARY} ${TK_STUB_LIBRARY})

set_target_properties(TkTextPlus
  PROPERTIES COMPILE_FLAGS "-DUSE_TCL_STUBS -DUSE_TCL_STUBS -DPACKAGE_PATCHLEVEL=\\\"${PACKAGE_PATCHLEVEL}\\\" -DPACKAGE_NAME=\\\"${PACKAGE_NAME}\\\"")

if(WIN32)
  install(TARGETS TkTextPlus RUNTIME DESTINATION "${DEST_DIR}" )
else( WIN32 )
  install(TARGETS TkTextPlus RUNTIME LIBRARY DESTINATION "${DEST_DIR}")
endif(WIN32)

