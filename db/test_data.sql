INSERT OR IGNORE INTO book_types (id, name) VALUES
(1, 'basic'),
(2, 'delux'),
(3, 'limited');

INSERT OR IGNORE INTO publishers (id, name) VALUES
(1,  'Addison-Wesley'),
(2,  'Prentice Hall'),
(3,  'MIT Press'),
(4,  'O''Reilly'),
(5,  'Pearson'),
(6,  'Packt'),
(7,  'Wiley'),
(8,  'McGraw-Hill'),
(9,  'Эксмо');

INSERT OR IGNORE INTO authors (id, name) VALUES
(1,  'David Thomas, Andrew Hunt'),
(2,  'Robert C. Martin'),
(3,  'Erich Gamma et al.'),
(4,  'Harold Abelson, Gerald Jay Sussman'),
(5,  'Brian Kernighan, Dennis Ritchie'),
(6,  'Thomas H. Cormen et al.'),
(7,  'Martin Fowler'),
(8,  'Steve McConnell'),
(9,  'Frederick P. Brooks Jr.'),
(10, 'Scott Meyers'),
(11, 'Stanley B. Lippman'),
(12, 'Mark Summerfield'),
(13, 'Eric Freeman, Elisabeth Robson'),
(14, 'Michael Feathers'),
(15, 'Eric Evans'),
(16, 'Robert Sedgewick'),
(17, 'Donald E. Knuth'),
(18, 'Alfred V. Aho et al.'),
(19, 'Abraham Silberschatz et al.'),
(20, 'James Kurose, Keith Ross'),
(21, 'Серена Валентино');

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
  (id, name, author, year, publisher, description, coverUrl, isHardcover, type,
   totalPages, pagesRead, globalRating, localRating, userRating, status, inWishList)
VALUES
(1,  'The Pragmatic Programmer',                              1,  1999, 1, 'A modern classic on the craft of software development — pragmatic techniques to write better code, design flexible systems, and grow as a developer.', 'https://covers.openlibrary.org/b/isbn/020161622X-M.jpg', 0, 1, 352, 352, 9.2, 9.0, 9, 3, 0),
(2,  'Clean Code',                                            2,  2008, 2, 'A handbook of agile software craftsmanship: how to read, write, and refactor code so it stays clean as systems grow.', 'https://covers.openlibrary.org/b/isbn/0132350882-M.jpg', 1, 1, 464, 464, 8.8, 8.6, 8, 3, 0),
(3,  'Design Patterns',                                       3,  1994, 1, 'The seminal "Gang of Four" catalog of 23 reusable object-oriented design patterns that shape modern software architecture.', 'https://covers.openlibrary.org/b/isbn/0201633612-M.jpg', 1, 2, 395, 395, 9.0, 9.2, 9, 3, 0),
(4,  'Structure and Interpretation of Computer Programs',     4,  1985, 3, 'The legendary MIT introduction to computing, exploring abstraction, recursion, and language design through Scheme.', 'https://covers.openlibrary.org/b/isbn/0262510871-M.jpg', 1, 1, 657, 657, 9.4, 9.5, 10, 3, 0),
(5,  'The C Programming Language',                            5,  1978, 2, 'The original definitive guide to C by its creators — concise, precise, and still influential decades later.', 'https://covers.openlibrary.org/b/isbn/0131103628-M.jpg', 0, 1, 272, 272, 9.1, 8.9, 9, 3, 0),
(6,  'Introduction to Algorithms',                            6,  2009, 3, 'Comprehensive coverage of algorithms: design, analysis, and the data structures that power modern systems.', 'https://covers.openlibrary.org/b/isbn/0262033844-M.jpg', 1, 1, 1312, 0, 9.3, 9.1, NULL, 1, 0),
(7,  'Refactoring',                                           7,  2018, 1, 'A catalog of refactorings — small behavior-preserving transformations that keep code maintainable as it evolves.', 'https://covers.openlibrary.org/b/isbn/0134757599-M.jpg', 0, 1, 448, 0, 8.7, 8.5, NULL, 1, 0),
(8,  'Code Complete',                                         8,  2004, 5, 'A practical guide to software construction: detailed advice on every stage of building robust, readable code.', 'https://covers.openlibrary.org/b/isbn/0735619670-M.jpg', 0, 1, 960, 0, 8.9, 8.8, NULL, 1, 0),
(9,  'The Mythical Man-Month',                                9,  1975, 1, 'Classic essays on software project management, including the famous insight that adding people to a late project makes it later.', 'https://covers.openlibrary.org/b/isbn/0201835959-M.jpg', 0, 2, 322, 0, 8.5, 8.4, NULL, 1, 0),
(10, 'Effective Modern C++',                                 10,  2014, 4, 'Forty-two specific ways to improve your use of C++11 and C++14, from move semantics to smart pointers and lambdas.', 'https://covers.openlibrary.org/b/isbn/1491903996-M.jpg', 0, 1, 334, 156, 9.0, 8.8, 9, 2, 0),
(11, 'C++ Primer',                                           11,  2012, 1, 'A thorough introduction to modern C++ — from fundamentals through templates, STL, and advanced features.', 'https://covers.openlibrary.org/b/isbn/0321714113-M.jpg', 1, 1, 976, 240, 8.6, 8.7, 8, 2, 0),
(12, 'Qt 6 Programming',                                     12,  2023, 6, 'Build cross-platform applications with Qt 6 — covers QML, Qt Quick, widgets, and modern application architecture.', 'https://covers.openlibrary.org/b/isbn/1800204582-M.jpg', 0, 1, 412, 0, 8.0, 7.8, NULL, 0, 1),
(13, 'Head First Design Patterns',                           13,  2020, 4, 'A brain-friendly guide to the Gang of Four patterns, with rich visuals and worked examples.', 'https://covers.openlibrary.org/b/isbn/149207800X-M.jpg', 0, 1, 672, 0, 8.4, 8.5, NULL, 0, 1),
(14, 'Working Effectively with Legacy Code',                 14,  2004, 2, 'Techniques for safely changing untested code — break dependencies, add tests, and refactor with confidence.', 'https://covers.openlibrary.org/b/isbn/0131177052-M.jpg', 0, 1, 456, 0, 8.7, 8.6, NULL, 0, 1),
(15, 'Domain-Driven Design',                                 15,  2003, 1, 'Tackling complexity at the heart of software through ubiquitous language, bounded contexts, and rich domain models.', 'https://covers.openlibrary.org/b/isbn/0321125215-M.jpg', 1, 2, 560, 0, 8.8, 8.9, NULL, 0, 1),
(16, 'Algorithms',                                           16,  2011, 1, 'A modern, broad treatment of essential algorithms with implementations and applications across domains.', 'https://covers.openlibrary.org/b/isbn/032157351X-M.jpg', 1, 1, 976, 0, 9.0, 9.1, NULL, 0, 0),
(17, 'The Art of Computer Programming',                      17,  1968, 1, 'Knuth''s monumental, multi-volume reference covering the analysis of algorithms in extraordinary depth.', 'https://covers.openlibrary.org/b/isbn/0201896834-M.jpg', 1, 3, 650, 0, 9.5, 9.4, NULL, 0, 0),
(18, 'Compilers: Principles, Techniques, and Tools',         18,  2006, 5, 'The "Dragon Book" — comprehensive introduction to compiler design, parsing, optimization, and code generation.', 'https://covers.openlibrary.org/b/isbn/0321486811-M.jpg', 1, 1, 1009, 0, 8.9, 8.7, NULL, 0, 0),
(19, 'Operating System Concepts',                            19,  2018, 7, 'A standard textbook covering process management, memory, file systems, and concurrent programming.', 'https://covers.openlibrary.org/b/isbn/1119320917-M.jpg', 1, 1, 944, 0, 8.5, 8.3, NULL, 0, 0),
(20, 'Computer Networking: A Top-Down Approach',             20,  2016, 5, 'Networking from the application layer down — covers HTTP, TCP, IP, and the protocols underpinning the modern internet.', 'https://covers.openlibrary.org/b/isbn/0133594149-M.jpg', 1, 1, 864, 0, 8.6, 8.5, NULL, 0, 0),
(21, 'Леди Тремейн. История злой мачехи',                    21,  2021, 9, 'Все знают историю Золушки и её злой мачехи, которая с дочерьми мучила бедную будущую принцессу. Но никто никогда не задумывался о том, почему леди Тремейн была так жестока, а Анастасия и Дризелла — злобны и завистливы. Что привело знатную светскую даму в глушь далёкого затерянного королевства? Каково ей было оказаться вдали от дома, от привычной роскоши и сонма слуг? И зачем отец Золушки решил жениться на такой холодной и жёсткой женщине? Быть может, она не всегда была бессердечной, а стала такой, потому что кто-то разбил ей сердце?', 'https://cdn.eksmo.ru/v2/ITD000000001154470/COVER/cover1__w600.jpg', 1, 1, 320, 0, NULL, NULL, NULL, 0, 1);

INSERT OR IGNORE INTO book_genres (book_id, genre_id) VALUES
(1, 1), (1, 2),
(2, 1), (2, 9),
(3, 6), (3, 1),
(4, 4), (4, 2),
(5, 2), (5, 5),
(6, 3), (6, 4),
(7, 9), (7, 1),
(8, 1),
(9, 1),
(10, 5), (10, 2),
(11, 5),
(12, 5),
(13, 6),
(14, 9),
(15, 6), (15, 1),
(16, 3),
(17, 3), (17, 4),
(18, 4), (18, 2),
(19, 8),
(20, 7),
(21, 10), (21, 11);

INSERT OR IGNORE INTO book_characters (book_id, name, role) VALUES
(10, 'auto&&',     'Universal type deduction'),
(10, 'std::move',  'Casts to rvalue reference'),
(10, 'unique_ptr', 'Exclusive ownership guard'),
(13, 'Strategy',   'Encapsulates a family of algorithms'),
(13, 'Observer',   'Reacts to subject state changes'),
(13, 'Decorator',  'Adds responsibilities dynamically');

-- Reading sessions; sums of (pages_to - pages_from) per book match books.pagesRead.
INSERT INTO reading_sessions (book_id, started_at, ended_at, pages_from, pages_to) VALUES
(1,  '2026-03-01 19:00:00', '2026-03-01 20:30:00',   0,  90),
(1,  '2026-03-05 18:30:00', '2026-03-05 20:00:00',  90, 180),
(1,  '2026-03-09 19:00:00', '2026-03-09 20:30:00', 180, 270),
(1,  '2026-03-12 19:00:00', '2026-03-12 20:25:00', 270, 352),
(10, '2026-04-15 19:30:00', '2026-04-15 20:15:00',   0,  30),
(10, '2026-04-18 21:00:00', '2026-04-18 21:50:00',  30,  75),
(10, '2026-04-22 20:00:00', '2026-04-22 21:10:00',  75, 120),
(10, '2026-04-26 19:00:00', '2026-04-26 19:50:00', 120, 156),
(11, '2026-04-10 18:00:00', '2026-04-10 19:30:00',   0,  80),
(11, '2026-04-14 19:30:00', '2026-04-14 20:45:00',  80, 160),
(11, '2026-04-20 18:30:00', '2026-04-20 19:50:00', 160, 240);
