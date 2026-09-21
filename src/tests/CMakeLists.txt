find_package(Qt6 REQUIRED COMPONENTS Test)

function(readary_add_test source)
    cmake_parse_arguments(ARG "" "" "LIBS" ${ARGN})

    get_filename_component(target ${source} NAME_WE)
    string(REGEX REPLACE "Test$" "" test_name ${target})

    qt_add_executable(${target} ${source})
    # This dir on the include path so any test can say #include "support/…".
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE ReadaryCore Qt6::Test ${ARG_LIBS})
    add_test(NAME ${test_name} COMMAND ${target})
endfunction()

readary_add_test(core/app/AppEnvironmentTest.cpp)
readary_add_test(core/db/DatabaseManagerTest.cpp)
readary_add_test(core/db/SqlQueryBuilderTest.cpp)

readary_add_test(services/dto/BookDTOTest.cpp)
readary_add_test(services/dto/CharacterDTOTest.cpp)
readary_add_test(services/dto/ReadingSessionDTOTest.cpp)
readary_add_test(services/storage/BookTableTest.cpp)
readary_add_test(services/storage/BookFileStoreTest.cpp)
readary_add_test(services/caching/SearchCacheTest.cpp)
readary_add_test(services/caching/ReadingSessionCacheTest.cpp)
readary_add_test(services/caching/ReadingProgressCacheTest.cpp)
readary_add_test(services/pdf/PdfMetadataReaderTest.cpp)
readary_add_test(services/filtering/BookFilterCriteriaTest.cpp)
readary_add_test(services/emoji/EmojiResolverTest.cpp)
readary_add_test(services/statistics/BookStatisticsCalculatorTest.cpp)

readary_add_test(models/books/list/BookListModelBaseTest.cpp)
readary_add_test(models/books/list/BookListModelTest.cpp)
readary_add_test(models/books/proxy/BookSortFilterProxyModelTest.cpp)
readary_add_test(models/books/proxy/BookSearchProxyModelTest.cpp)
readary_add_test(models/books/proxy/BookCriteriaFilterProxyModelTest.cpp)
readary_add_test(models/books/details/BookCharactersModelTest.cpp)
readary_add_test(models/books/details/ReadingHistoryModelTest.cpp)
readary_add_test(models/settings/LanguageModelTest.cpp)
readary_add_test(models/settings/FontModelTest.cpp)

readary_add_test(controllers/BookControllerTest.cpp)
readary_add_test(controllers/BookControllerSessionTest.cpp)
readary_add_test(controllers/BookStatisticsControllerTest.cpp)
readary_add_test(controllers/BookFilterControllerTest.cpp)
readary_add_test(controllers/GlobalBookSearchControllerTest.cpp)
readary_add_test(controllers/NavigationControllerTest.cpp)
readary_add_test(controllers/SettingsControllerTest.cpp)
readary_add_test(controllers/QmlSingletonWiringTest.cpp)

readary_add_test(api/LanguageDetectorTest.cpp)
readary_add_test(api/LanguageConverterTest.cpp)
readary_add_test(api/BookSearchAPICompositeTest.cpp)

readary_add_test(qmltypes/BookDTOObjectTest.cpp)
readary_add_test(qmltypes/BookStatisticsObjectTest.cpp)

readary_add_test(utils/IsbnValidatorTest.cpp)

# The network-backed clients are driven against a local QTcpServer stub.
readary_add_test(api/GoogleTranslatorTest.cpp LIBS Qt6::Network)
readary_add_test(api/OpenLibrarySearchAPITest.cpp LIBS Qt6::Network)
readary_add_test(api/GoogleBooksSearchAPITest.cpp LIBS Qt6::Network)
