find_package(Qt6 REQUIRED COMPONENTS Test)

qt_add_executable(BookSearchProxyModelTest
    models/BookSearchProxyModelTest.cpp
)

target_link_libraries(BookSearchProxyModelTest PRIVATE
    ReadaryCore
    Qt6::Test
)

add_test(NAME BookSearchProxyModel COMMAND BookSearchProxyModelTest)

qt_add_executable(LanguageModelTest
    models/LanguageModelTest.cpp
)

target_link_libraries(LanguageModelTest PRIVATE
    ReadaryCore
    Qt6::Test
)

add_test(NAME LanguageModel COMMAND LanguageModelTest)

qt_add_executable(FontModelTest
    models/FontModelTest.cpp
)

target_link_libraries(FontModelTest PRIVATE
    ReadaryCore
    Qt6::Test
)

add_test(NAME FontModel COMMAND FontModelTest)

qt_add_executable(EmojiResolverTest
    services/EmojiResolverTest.cpp
)

target_link_libraries(EmojiResolverTest PRIVATE
    ReadaryCore
    Qt6::Test
)

add_test(NAME EmojiResolver COMMAND EmojiResolverTest)

qt_add_executable(IsbnValidatorTest
    utils/IsbnValidatorTest.cpp
)

target_link_libraries(IsbnValidatorTest PRIVATE
    ReadaryCore
    Qt6::Test
)

add_test(NAME IsbnValidator COMMAND IsbnValidatorTest)
