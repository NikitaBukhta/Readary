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

qt_add_executable(SearchCacheTest
    services/SearchCacheTest.cpp
)

target_link_libraries(SearchCacheTest PRIVATE
    ReadaryCore
    Qt6::Test
)

add_test(NAME SearchCache COMMAND SearchCacheTest)

qt_add_executable(ReadingSessionCacheTest
    services/ReadingSessionCacheTest.cpp
)

target_link_libraries(ReadingSessionCacheTest PRIVATE
    ReadaryCore
    Qt6::Test
)

add_test(NAME ReadingSessionCache COMMAND ReadingSessionCacheTest)

qt_add_executable(BookControllerSessionTest
    controllers/BookControllerSessionTest.cpp
)

target_link_libraries(BookControllerSessionTest PRIVATE
    ReadaryCore
    Qt6::Test
)

add_test(NAME BookControllerSession COMMAND BookControllerSessionTest)

qt_add_executable(GlobalBookSearchControllerTest
    controllers/GlobalBookSearchControllerTest.cpp
)

target_link_libraries(GlobalBookSearchControllerTest PRIVATE
    ReadaryCore
    Qt6::Test
)

add_test(NAME GlobalBookSearchController COMMAND GlobalBookSearchControllerTest)
