
get_filename_component(TBLGEN_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_FILE}" PATH)
get_filename_component(TBLGEN_INSTALL_PREFIX "${TBLGEN_INSTALL_PREFIX}" PATH)

set(TBLGEN_INCLUDE_DIR "${TBLGEN_INSTALL_PREFIX}/include")
set(TBLGEN_MAIN_INCLUDE_DIR "${TBLGEN_INSTALL_PREFIX}/include")