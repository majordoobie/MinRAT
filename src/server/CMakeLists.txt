include(build_utils)

add_library(util SHARED utils.c)
set_project_properties(util ${PROJECT_SOURCE_DIR}/include)

add_library(server_file_api SHARED server_db.c server_file_api.c server_crypto.c)
target_link_libraries(server_file_api PUBLIC util ssl crypto hashtable dl_list)
set_project_properties(server_file_api ${PROJECT_SOURCE_DIR}/include)

add_library(server_ctrl SHARED server_ctrl.c server_args.c server_sock.c)
target_link_libraries(server_ctrl PUBLIC util thread_pool server_file_api)
set_project_properties(server_ctrl ${PROJECT_SOURCE_DIR}/include)

add_executable(server server_main.c)
target_link_libraries(server PUBLIC server_ctrl)
set_project_properties(server ${PROJECT_SOURCE_DIR}/include)