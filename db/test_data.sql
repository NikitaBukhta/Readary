INSERT OR IGNORE INTO genres (id, name) VALUES
(1, 'Software Engineering'),
(2, 'Programming'),
(3, 'Algorithms'),
(4, 'Computer Science'),
(5, 'C++'),
(6, 'Design Patterns'),
(7, 'Networking'),
(8, 'Operating Systems'),
(9, 'Refactoring'),
(10, 'Fantasy'),
(11, 'Children''s Literature');

-- Seed distribution per UI category:
--   Finished     (status=3):       ids 1-5
--   WantToRead   (status=1):       ids 6-9
--   InProgress   (status=2):       ids 10-11
--   WantToBuy    (inWishList=1):   ids 12-15, 21
--   Uncategorized:                 ids 16-20
-- Cover URLs use Open Library Covers API by ISBN; missing ISBNs fall back to the QML placeholder.
INSERT OR REPLACE INTO books
  (isbn, name, author, year, publisher, description, coverUrl, isHardcover, type,
   totalPages, pagesRead, globalRating, localRating, userRating, status, inWishList)
VALUES
(9780201616224,  'The Pragmatic Programmer', 'David Thomas, Andrew Hunt',  1999, 'Addison-Wesley', 'A modern classic on the craft of software development — pragmatic techniques to write better code, design flexible systems, and grow as a developer.', 'https://covers.openlibrary.org/b/isbn/020161622X-M.jpg', 0, 'basic', 352, 352, 9.2, 9.0, 9, 3, 0),
(9780132350884,  'Clean Code', 'Robert C. Martin',  2008, 'Prentice Hall', 'A handbook of agile software craftsmanship: how to read, write, and refactor code so it stays clean as systems grow.', 'https://covers.openlibrary.org/b/isbn/0132350882-M.jpg', 1, 'basic', 464, 464, 8.8, 8.6, 8, 3, 0),
(9780201633610,  'Design Patterns', 'Erich Gamma et al.',  1994, 'Addison-Wesley', 'The seminal "Gang of Four" catalog of 23 reusable object-oriented design patterns that shape modern software architecture.', 'https://covers.openlibrary.org/b/isbn/0201633612-M.jpg', 1, 'delux', 395, 395, 9.0, 9.2, 9, 3, 0),
(9780262510875,  'Structure and Interpretation of Computer Programs', 'Harold Abelson, Gerald Jay Sussman',  1985, 'MIT Press', 'The legendary MIT introduction to computing, exploring abstraction, recursion, and language design through Scheme.', 'https://covers.openlibrary.org/b/isbn/0262510871-M.jpg', 1, 'basic', 657, 657, 9.4, 9.5, 10, 3, 0),
(9780131103627,  'The C Programming Language', 'Brian Kernighan, Dennis Ritchie',  1978, 'Prentice Hall', 'The original definitive guide to C by its creators — concise, precise, and still influential decades later.', 'https://covers.openlibrary.org/b/isbn/0131103628-M.jpg', 0, 'basic', 272, 272, 9.1, 8.9, 9, 3, 0),
(9780262033848,  'Introduction to Algorithms', 'Thomas H. Cormen et al.',  2009, 'MIT Press', 'Comprehensive coverage of algorithms: design, analysis, and the data structures that power modern systems.', 'https://covers.openlibrary.org/b/isbn/0262033844-M.jpg', 1, 'basic', 1312, 0, 9.3, 9.1, NULL, 1, 0),
(9780134757599,  'Refactoring', 'Martin Fowler',  2018, 'Addison-Wesley', 'A catalog of refactorings — small behavior-preserving transformations that keep code maintainable as it evolves.', 'https://covers.openlibrary.org/b/isbn/0134757599-M.jpg', 0, 'basic', 448, 0, 8.7, 8.5, NULL, 1, 0),
(9780735619678,  'Code Complete', 'Steve McConnell',  2004, 'Pearson', 'A practical guide to software construction: detailed advice on every stage of building robust, readable code.', 'https://covers.openlibrary.org/b/isbn/0735619670-M.jpg', 0, 'basic', 960, 0, 8.9, 8.8, NULL, 1, 0),
(9780201835953,  'The Mythical Man-Month', 'Frederick P. Brooks Jr.',  1975, 'Addison-Wesley', 'Classic essays on software project management, including the famous insight that adding people to a late project makes it later.', 'https://covers.openlibrary.org/b/isbn/0201835959-M.jpg', 0, 'delux', 322, 0, 8.5, 8.4, NULL, 1, 0),
(9781491903995, 'Effective Modern C++', 'Scott Meyers',  2014, 'O''Reilly', 'Forty-two specific ways to improve your use of C++11 and C++14, from move semantics to smart pointers and lambdas.', 'https://covers.openlibrary.org/b/isbn/1491903996-M.jpg', 0, 'basic', 334, 156, 9.0, 8.8, 9, 2, 0),
(9780321714114, 'C++ Primer', 'Stanley B. Lippman',  2012, 'Addison-Wesley', 'A thorough introduction to modern C++ — from fundamentals through templates, STL, and advanced features.', 'https://covers.openlibrary.org/b/isbn/0321714113-M.jpg', 1, 'basic', 976, 240, 8.6, 8.7, 8, 2, 0),
(9781800204584, 'Qt 6 Programming', 'Mark Summerfield',  2023, 'Packt', 'Build cross-platform applications with Qt 6 — covers QML, Qt Quick, widgets, and modern application architecture.', 'https://covers.openlibrary.org/b/isbn/1800204582-M.jpg', 0, 'basic', 412, 0, 8.0, 7.8, NULL, 0, 1),
(9781492078005, 'Head First Design Patterns', 'Eric Freeman, Elisabeth Robson',  2020, 'O''Reilly', 'A brain-friendly guide to the Gang of Four patterns, with rich visuals and worked examples.', 'https://covers.openlibrary.org/b/isbn/149207800X-M.jpg', 0, 'basic', 672, 0, 8.4, 8.5, NULL, 0, 1),
(9780131177055, 'Working Effectively with Legacy Code', 'Michael Feathers',  2004, 'Prentice Hall', 'Techniques for safely changing untested code — break dependencies, add tests, and refactor with confidence.', 'https://covers.openlibrary.org/b/isbn/0131177052-M.jpg', 0, 'basic', 456, 0, 8.7, 8.6, NULL, 0, 1),
(9780321125217, 'Domain-Driven Design', 'Eric Evans',  2003, 'Addison-Wesley', 'Tackling complexity at the heart of software through ubiquitous language, bounded contexts, and rich domain models.', 'https://covers.openlibrary.org/b/isbn/0321125215-M.jpg', 1, 'delux', 560, 0, 8.8, 8.9, NULL, 0, 1),
(9780321573513, 'Algorithms', 'Robert Sedgewick',  2011, 'Addison-Wesley', 'A modern, broad treatment of essential algorithms with implementations and applications across domains.', 'https://covers.openlibrary.org/b/isbn/032157351X-M.jpg', 1, 'basic', 976, 0, 9.0, 9.1, NULL, 0, 0),
(9780201896831, 'The Art of Computer Programming', 'Donald E. Knuth',  1968, 'Addison-Wesley', 'Knuth''s monumental, multi-volume reference covering the analysis of algorithms in extraordinary depth.', 'https://covers.openlibrary.org/b/isbn/0201896834-M.jpg', 1, 'limited', 650, 0, 9.5, 9.4, NULL, 0, 0),
(9780321486813, 'Compilers: Principles, Techniques, and Tools', 'Alfred V. Aho et al.',  2006, 'Pearson', 'The "Dragon Book" — comprehensive introduction to compiler design, parsing, optimization, and code generation.', 'https://covers.openlibrary.org/b/isbn/0321486811-M.jpg', 1, 'basic', 1009, 0, 8.9, 8.7, NULL, 0, 0),
(9781119320913, 'Operating System Concepts', 'Abraham Silberschatz et al.',  2018, 'Wiley', 'A standard textbook covering process management, memory, file systems, and concurrent programming.', 'https://covers.openlibrary.org/b/isbn/1119320917-M.jpg', 1, 'basic', 944, 0, 8.5, 8.3, NULL, 0, 0),
(9780133594140, 'Computer Networking: A Top-Down Approach', 'James Kurose, Keith Ross',  2016, 'Pearson', 'Networking from the application layer down — covers HTTP, TCP, IP, and the protocols underpinning the modern internet.', 'https://covers.openlibrary.org/b/isbn/0133594149-M.jpg', 1, 'basic', 864, 0, 8.6, 8.5, NULL, 0, 0),
(9780000000021, 'Леди Тремейн. История злой мачехи', 'Серена Валентино',  2021, 'Эксмо', 'Все знают историю Золушки и её злой мачехи, которая с дочерьми мучила бедную будущую принцессу. Но никто никогда не задумывался о том, почему леди Тремейн была так жестока, а Анастасия и Дризелла — злобны и завистливы. Что привело знатную светскую даму в глушь далёкого затерянного королевства? Каково ей было оказаться вдали от дома, от привычной роскоши и сонма слуг? И зачем отец Золушки решил жениться на такой холодной и жёсткой женщине? Быть может, она не всегда была бессердечной, а стала такой, потому что кто-то разбил ей сердце?', 'https://cdn.eksmo.ru/v2/ITD000000001154470/COVER/cover1__w600.jpg', 1, 'basic', 320, 0, NULL, NULL, NULL, 0, 1);

INSERT OR IGNORE INTO book_genres (book_isbn, genre_id) VALUES
(9780201616224, 1), (9780201616224, 2),
(9780132350884, 1), (9780132350884, 9),
(9780201633610, 6), (9780201633610, 1),
(9780262510875, 4), (9780262510875, 2),
(9780131103627, 2), (9780131103627, 5),
(9780262033848, 3), (9780262033848, 4),
(9780134757599, 9), (9780134757599, 1),
(9780735619678, 1),
(9780201835953, 1),
(9781491903995, 5), (9781491903995, 2),
(9780321714114, 5),
(9781800204584, 5),
(9781492078005, 6),
(9780131177055, 9),
(9780321125217, 6), (9780321125217, 1),
(9780321573513, 3),
(9780201896831, 3), (9780201896831, 4),
(9780321486813, 4), (9780321486813, 2),
(9781119320913, 8),
(9780133594140, 7),
(9780000000021, 10), (9780000000021, 11);

INSERT OR IGNORE INTO book_characters (book_isbn, name, role) VALUES
(9781491903995, 'auto&&',     'Universal type deduction'),
(9781491903995, 'std::move',  'Casts to rvalue reference'),
(9781491903995, 'unique_ptr', 'Exclusive ownership guard'),
(9781492078005, 'Strategy',   'Encapsulates a family of algorithms'),
(9781492078005, 'Observer',   'Reacts to subject state changes'),
(9781492078005, 'Decorator',  'Adds responsibilities dynamically');

-- Reading sessions; sums of (pages_to - pages_from) per book match books.pagesRead.
-- Stamps are UTC ISO-8601, the shape BookTable::insertReadingSession writes;
-- started_at is TEXT, so ORDER BY compares it lexically and a second shape
-- would sort wrong against these rows.
INSERT INTO reading_sessions (book_isbn, started_at, ended_at, pages_from, pages_to) VALUES
(9780201616224, '2026-03-01T19:00:00Z', '2026-03-01T20:30:00Z',   0,  90),
(9780201616224, '2026-03-05T18:30:00Z', '2026-03-05T20:00:00Z',  90, 180),
(9780201616224, '2026-03-09T19:00:00Z', '2026-03-09T20:30:00Z', 180, 270),
(9780201616224, '2026-03-12T19:00:00Z', '2026-03-12T20:25:00Z', 270, 352),
(9781491903995, '2026-04-15T19:30:00Z', '2026-04-15T20:15:00Z',   0,  30),
(9781491903995, '2026-04-18T21:00:00Z', '2026-04-18T21:50:00Z',  30,  75),
(9781491903995, '2026-04-22T20:00:00Z', '2026-04-22T21:10:00Z',  75, 120),
(9781491903995, '2026-04-26T19:00:00Z', '2026-04-26T19:50:00Z', 120, 156),
(9780321714114, '2026-04-10T18:00:00Z', '2026-04-10T19:30:00Z',   0,  80),
(9780321714114, '2026-04-14T19:30:00Z', '2026-04-14T20:45:00Z',  80, 160),
(9780321714114, '2026-04-20T18:30:00Z', '2026-04-20T19:50:00Z', 160, 240);
