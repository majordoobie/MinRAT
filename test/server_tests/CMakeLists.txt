add_executable(
        gtest_server
        gtest_server_crypto.cpp
        gtest_server_file_api.cpp
        gtest_server_args.cpp
        gtest_server_db.cpp
)
target_link_libraries(
        gtest_server
        PUBLIC
        server_ctrl
        server_file_api
)
GTest_add_target(gtest_server)
