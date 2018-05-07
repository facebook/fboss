add_executable(qsfp_service
    fboss/qsfp_service/oss/QsfpServer.cpp
    fboss/qsfp_service/Main.cpp
    fboss/qsfp_service/QsfpServiceHandler.cpp
    fboss/qsfp_service/sff/QsfpModule.cpp
    fboss/qsfp_service/sff/SffFieldInfo.cpp
    fboss/qsfp_service/sff/oss/QsfpModule.cpp
    fboss/qsfp_service/platforms/mlnx/MlnxQsfp.cpp
    fboss/qsfp_service/platforms/mlnx/MlnxManagerInit.cpp
    fboss/qsfp_service/platforms/mlnx/MlnxManager.cpp
    fboss/qsfp_service/platforms/mlnx/mlnx_qsfp.cpp
)

target_link_libraries(qsfp_service
                      fboss_agent
)

