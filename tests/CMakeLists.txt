# Unit tests for Yasu.
#
# The tests are headless: they run with QT_QPA_PLATFORM=offscreen and only
# exercise the non-UI logic (config, sign-up, export parsing, arbiter stats).

set(YASU_TEST_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

# --- ConfigManager ---------------------------------------------------------
add_executable(test_config
    test_config.cpp
    ../configmanager.cpp
)
target_include_directories(test_config PRIVATE ${YASU_TEST_INCLUDE_DIR})
target_link_libraries(test_config PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    tomlplusplus::tomlplusplus
)

# --- Yasu sign-up logic ----------------------------------------------------
add_executable(test_signup
    test_signup.cpp
    ../signup.cpp
    ../configmanager.cpp
)
target_include_directories(test_signup PRIVATE ${YASU_TEST_INCLUDE_DIR})
target_link_libraries(test_signup PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    tomlplusplus::tomlplusplus
)

# --- Export parsing logic --------------------------------------------------
add_executable(test_export
    test_export.cpp
    ../exportdialog.cpp
    ../signlogparser.cpp
    ../configmanager.cpp
)
target_include_directories(test_export PRIVATE ${YASU_TEST_INCLUDE_DIR})
target_link_libraries(test_export PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
    QXlsx
    tomlplusplus::tomlplusplus
)

# --- Rewardly export logic -------------------------------------------------
add_executable(test_rewardly
    test_rewardly.cpp
    ../rewardlyexportdialog.cpp
    ../signlogparser.cpp
    ../configmanager.cpp
)
target_include_directories(test_rewardly PRIVATE ${YASU_TEST_INCLUDE_DIR})
target_link_libraries(test_rewardly PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
    QXlsx
    tomlplusplus::tomlplusplus
)

# --- Arbiter statistics ----------------------------------------------------
add_executable(test_arbiter
    test_arbiter.cpp
    ../arbiter.cpp
    ../arbiter.ui
    ../signlogparser.cpp
    ../configmanager.cpp
)
target_include_directories(test_arbiter PRIVATE ${YASU_TEST_INCLUDE_DIR})
target_link_libraries(test_arbiter PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
    tomlplusplus::tomlplusplus
)

foreach(test_name test_config test_signup test_export test_rewardly test_arbiter)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
        RUN_SERIAL TRUE
    )
endforeach()
