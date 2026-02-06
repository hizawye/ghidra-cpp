add_executable(ghidra_headless ghidra_headless/main.cpp)
add_executable(ghidra_server ghidra_server/main.cpp)
add_executable(sleighc sleighc/main.cpp)
add_executable(ghidra_export ghidra_export/main.cpp)

if(GHIRDA_BUILD_GUI)
  add_executable(ghidra_gui ghidra_gui/main.cpp)
  target_link_libraries(ghidra_gui PRIVATE ghirda_ui)
endif()

target_link_libraries(ghidra_headless PRIVATE ghirda_decompiler ghirda_loader ghirda_script ghirda_plugin)
target_link_libraries(ghidra_server PRIVATE ghirda_server)
target_link_libraries(sleighc PRIVATE ghirda_sleigh)
target_link_libraries(ghidra_export PRIVATE ghirda_core ghirda_loader)
