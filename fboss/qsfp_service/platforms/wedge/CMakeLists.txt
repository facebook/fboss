add_executable(qsfp_service
    fboss/qsfp_service/oss/QsfpServer.cpp
    fboss/qsfp_service/Main.cpp
    fboss/qsfp_service/QsfpServiceHandler.cpp
    fboss/qsfp_service/sff/QsfpModule.cpp
    fboss/qsfp_service/sff/SffFieldInfo.cpp
    fboss/qsfp_service/sff/oss/QsfpModule.cpp
    fboss/qsfp_service/platforms/wedge/WedgeQsfp.cpp
    fboss/qsfp_service/platforms/wedge/WedgeManagerInit.cpp
    fboss/qsfp_service/platforms/wedge/oss/WedgeManagerInit.cpp
    fboss/qsfp_service/platforms/wedge/WedgeManager.cpp
    fboss/qsfp_service/platforms/wedge/Wedge100Manager.cpp
    fboss/qsfp_service/platforms/wedge/GalaxyManager.cpp
    fboss/qsfp_service/platforms/wedge/Wedge40Manager.cpp
    fboss/agent/platforms/wedge/WedgeProductInfo.cpp
    fboss/agent/platforms/wedge/oss/WedgeProductInfo.cpp
)

target_link_libraries(qsfp_service
                      fboss_agent
)

