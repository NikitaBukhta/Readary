find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(BookSearchProxyModelTest
    models/BookSearchProxyModelTest.cpp
)

target_link_libraries(BookSearchProxyModelTest PRIVATE
    BeeLibraryCore
    Qt6::Test
)

add_test(NAME BookSearchProxyModel COMMAND BookSearchProxyModelTest)
