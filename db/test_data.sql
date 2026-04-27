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
(8,  'McGraw-Hill');

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
(20, 'James Kurose, Keith Ross');

-- status: 0 = NONE, 1 = WantToRead, 2 = InProgress, 3 = Finished
-- Distribution to seed each category in the UI:
--   Finished     (status=3): ids 1-5
--   WantToRead   (status=1): ids 6-9
--   InProgress   (status=2): ids 10-11
--   WantToBuy    (inWishList=1): ids 12-15
--   Uncategorized: ids 16-20
INSERT OR REPLACE INTO books
  (id, name, author, year, publisher, description, isHardcover, type, globalRating, localRating, userRating, status, inWishList)
VALUES
(1,  'The Pragmatic Programmer',                              1,  1999, 1, NULL, 0, 1, NULL, NULL, NULL, 3, 0),
(2,  'Clean Code',                                            2,  2008, 2, NULL, 1, 1, NULL, NULL, NULL, 3, 0),
(3,  'Design Patterns',                                       3,  1994, 1, NULL, 1, 2, NULL, NULL, NULL, 3, 0),
(4,  'Structure and Interpretation of Computer Programs',     4,  1985, 3, NULL, 1, 1, NULL, NULL, NULL, 3, 0),
(5,  'The C Programming Language',                            5,  1978, 2, NULL, 0, 1, NULL, NULL, NULL, 3, 0),
(6,  'Introduction to Algorithms',                            6,  2009, 3, NULL, 1, 1, NULL, NULL, NULL, 1, 0),
(7,  'Refactoring',                                           7,  2018, 1, NULL, 0, 1, NULL, NULL, NULL, 1, 0),
(8,  'Code Complete',                                         8,  2004, 5, NULL, 0, 1, NULL, NULL, NULL, 1, 0),
(9,  'The Mythical Man-Month',                                9,  1975, 1, NULL, 0, 2, NULL, NULL, NULL, 1, 0),
(10, 'Effective Modern C++',                                 10,  2014, 4, NULL, 0, 1, NULL, NULL, NULL, 2, 0),
(11, 'C++ Primer',                                           11,  2012, 1, NULL, 1, 1, NULL, NULL, NULL, 2, 0),
(12, 'Qt 6 Programming',                                     12,  2023, 6, NULL, 0, 1, NULL, NULL, NULL, 0, 1),
(13, 'Head First Design Patterns',                           13,  2020, 4, NULL, 0, 1, NULL, NULL, NULL, 0, 1),
(14, 'Working Effectively with Legacy Code',                 14,  2004, 2, NULL, 0, 1, NULL, NULL, NULL, 0, 1),
(15, 'Domain-Driven Design',                                 15,  2003, 1, NULL, 1, 2, NULL, NULL, NULL, 0, 1),
(16, 'Algorithms',                                           16,  2011, 1, NULL, 1, 1, NULL, NULL, NULL, 0, 0),
(17, 'The Art of Computer Programming',                      17,  1968, 1, NULL, 1, 3, NULL, NULL, NULL, 0, 0),
(18, 'Compilers: Principles, Techniques, and Tools',         18,  2006, 5, NULL, 1, 1, NULL, NULL, NULL, 0, 0),
(19, 'Operating System Concepts',                            19,  2018, 7, NULL, 1, 1, NULL, NULL, NULL, 0, 0),
(20, 'Computer Networking: A Top-Down Approach',             20,  2016, 5, NULL, 1, 1, NULL, NULL, NULL, 0, 0);
