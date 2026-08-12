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

readary_add_test(models/BookSearchProxyModelTest.cpp)
readary_add_test(models/LanguageModelTest.cpp)
readary_add_test(models/FontModelTest.cpp)

readary_add_test(services/EmojiResolverTest.cpp)
readary_add_test(services/SearchCacheTest.cpp)
readary_add_test(services/ReadingSessionCacheTest.cpp)
readary_add_test(services/BookFilterCriteriaTest.cpp)

readary_add_test(utils/IsbnValidatorTest.cpp)

readary_add_test(controllers/BookControllerSessionTest.cpp)
readary_add_test(controllers/GlobalBookSearchControllerTest.cpp)

readary_add_test(api/LanguageDetectorTest.cpp)
readary_add_test(api/LanguageConverterTest.cpp)
readary_add_test(api/BookSearchAPICompositeTest.cpp)

# The network-backed clients are driven against a local QTcpServer stub.
readary_add_test(api/GoogleTranslatorTest.cpp LIBS Qt6::Network)
readary_add_test(api/OpenLibrarySearchAPITest.cpp LIBS Qt6::Network)
readary_add_test(api/GoogleBooksSearchAPITest.cpp LIBS Qt6::Network)
