-- LMS demo seed data. Apply after schema.sql. Idempotent.
BEGIN;

INSERT INTO public.branches (id, name) VALUES
  ('cs', 'Computer Science & Engineering'),
  ('it', 'Information Technology'),
  ('ec', 'Electronics & Communication Engineering'),
  ('ee', 'Electrical Engineering'),
  ('me', 'Mechanical Engineering'),
  ('ce', 'Civil Engineering'),
  ('mgmt', 'Management Studies'),
  ('sc', 'General Science & Humanities')
ON CONFLICT (id) DO NOTHING;

INSERT INTO public.admins (id, name, email, role, title, contact, password_hash) VALUES
  ('ad1', 'Meenakshi Raman', 'librarian@mitsgwalior.in', 'admin', 'Chief Librarian', '+91 44 2857 0100', 'scrypt$16384$8$1$eb95463da38c408141f927a66f6f9952$f6cf341d2917580ca97a9e02879ba8628dbd93fee58581e0bdc3b255bea1e752')
ON CONFLICT (id) DO NOTHING;

INSERT INTO public.students
  (id, name, email, student_id, branch, program, semester, contact, joined_date, status, password_hash) VALUES
  ('st1', 'Vijay Verma', 'vijay.verma@mitsgwl.ac.in', 'BTCS2601146', 'Computer Science', 'B.Tech', 5, '+91 98401 22114', '2025-07-15', 'active', 'scrypt$16384$8$1$eb95463da38c408141f927a66f6f9952$f6cf341d2917580ca97a9e02879ba8628dbd93fee58581e0bdc3b255bea1e752'),
  ('st2', 'Virash', 'virash@mitsgwl.ac.in', '2024EC1018', 'Electronics & Communication', 'B.Tech', 7, '+91 98401 33425', '2024-07-12', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st3', 'Meera Krishnan', 'meera.krishnan@mitsgwl.ac.in', '2025CS1057', 'Computer Science', 'B.Tech', 5, '+91 98401 44119', '2025-07-15', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st4', 'Arjun Nair', 'arjun.nair@mitsgwl.ac.in', '2024MG2011', 'Management Studies', 'MBA', 3, '+91 98401 55336', '2024-08-01', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st5', 'Sneha Patil', 'sneha.patil@mitsgwl.ac.in', '2025CS1063', 'Computer Science', 'B.Tech', 5, '+91 98401 66547', '2025-07-15', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st6', 'Karthik Reddy', 'karthik.reddy@mitsgwl.ac.in', '2024MG2004', 'Management Studies', 'MBA', 3, '+91 98401 77258', '2024-08-01', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st7', 'Ishita Deshpande', 'ishita.deshpande@mitsgwl.ac.in', '2024CS1009', 'Computer Science', 'B.Tech', 7, '+91 98401 88369', '2024-07-12', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st8', 'Vikram Joshi', 'vikram.joshi@mitsgwl.ac.in', '2024ME1023', 'Mechanical Engineering', 'B.Tech', 7, '+91 98401 99470', '2024-07-12', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st9', 'Priya Menon', 'priya.menon@mitsgwl.ac.in', '2025CS1071', 'Computer Science', 'B.Tech', 5, '+91 98402 10581', '2025-07-15', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st10', 'Aditya Kulkarni', 'aditya.kulkarni@mitsgwl.ac.in', '2025CS1088', 'Computer Science', 'B.Tech', 5, '+91 98402 21692', '2025-07-15', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st11', 'Divya Sharma', 'divya.sharma@mitsgwl.ac.in', '2025CS1094', 'Computer Science', 'B.Tech', 5, '+91 98402 32703', '2025-07-15', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st12', 'Fahad Khan', 'fahad.khan@mitsgwl.ac.in', '2024EC1027', 'Electronics & Communication', 'B.Tech', 7, '+91 98402 43814', '2024-07-12', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st13', 'Nikhil Rao', 'nikhil.rao@mitsgwl.ac.in', '2024CE1015', 'Civil Engineering', 'B.Tech', 7, '+91 98402 54925', '2024-07-12', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st14', 'Tanvi Bhosale', 'tanvi.bhosale@mitsgwl.ac.in', '2025EC1032', 'Electronics & Communication', 'B.Tech', 5, '+91 98402 65036', '2025-07-15', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st15', 'Sarthak Gupta', 'sarthak.gupta@mitsgwl.ac.in', '2024ME1031', 'Mechanical Engineering', 'B.Tech', 7, '+91 98402 76147', '2024-07-12', 'suspended', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa'),
  ('st16', 'Lakshmi Narayan', 'lakshmi.narayan@mitsgwl.ac.in', '2025MG2019', 'Management Studies', 'MBA', 1, '+91 98402 87258', '2025-08-05', 'active', 'scrypt$16384$8$1$ee8c9ffedece9c00ec4ee0d8e160c496$9c9f7390dc52620754a7815992f41a11ef53d9efebf72eb79fc066da31a2f0fa')
ON CONFLICT (id) DO NOTHING;

INSERT INTO public.books
  (id, title, author, isbn, category, publisher, publication_date, description, cover,
   total_copies, available_copies, year, branch_id) VALUES
  ('bk1', 'Clean Code', 'Robert C. Martin', '9780132350884', 'Computer Science', 'Prentice Hall', '2008-08-20', 'A handbook of agile software craftsmanship. The book explains how to write code that is readable, maintainable and free of common defects, with practical case studies and step-by-step refactoring examples.', '#2F4858', 5, 3, 2, 'cs'),
  ('bk2', 'The Pragmatic Programmer', 'David Thomas, Andrew Hunt', '9780135957059', 'Computer Science', 'Addison-Wesley', '2019-09-23', 'Timeless lessons for software developers covering career growth, personal responsibility and practical techniques for building better software faster.', '#33658A', 4, 2, 2, 'cs'),
  ('bk3', 'Introduction to Algorithms', 'Thomas H. Cormen, Charles Leiserson', '9780262033848', 'Computer Science', 'MIT Press', '2009-07-31', 'The classic algorithms textbook used across universities worldwide. Covers sorting, searching, graph algorithms, dynamic programming and complexity analysis.', '#4527A0', 6, 4, 2, 'cs'),
  ('bk4', 'Designing Data-Intensive Applications', 'Martin Kleppmann', '9781449373320', 'Computer Science', 'O''Reilly Media', '2017-03-16', 'A deep dive into the principles of reliable, scalable and maintainable systems: storage engines, replication, partitioning, batch processing and stream processing.', '#7B2D26', 3, 0, 2, 'cs'),
  ('bk5', 'The Psychology of Money', 'Morgan Housel', '9780857197689', 'Finance', 'Harriman House', '2020-09-08', 'Short stories about money, behaviour and decision making that explain how people think about wealth, greed and happiness.', '#1B5E20', 5, 3, 1, 'mgmt'),
  ('bk6', 'Atomic Habits', 'James Clear', '9780735211292', 'Self Development', 'Avery', '2018-10-16', 'A proven framework for improving every day. Tiny changes in behaviour build remarkable results over time through the four laws of behaviour change.', '#AD1457', 6, 4, 4, 'mgmt'),
  ('bk7', 'Operating System Concepts', 'Abraham Silberschatz, Peter Galvin', '9781119320913', 'Computer Science', 'Wiley', '2018-11-13', 'The well-known "Dinosaur book" covering processes, threads, memory management, file systems, I/O and security with Linux and Windows examples.', '#00695C', 5, 2, 2, 'cs'),
  ('bk8', 'Computer Networking: A Top-Down Approach', 'James Kurose, Keith Ross', '9780132126953', 'Computer Science', 'Pearson', '2021-02-15', 'Builds understanding from the application layer downward, with strong coverage of wireless networks, network security and multimedia protocols.', '#E65100', 4, 1, 2, 'it'),
  ('bk9', 'Database System Concepts', 'Abraham Silberschatz, Henry Korth', '9780078022159', 'Computer Science', 'McGraw-Hill', '2019-06-01', 'Comprehensive coverage of the relational model, SQL, database design, transactions, storage and emerging data models for database courses.', '#283593', 5, 3, 2, 'it'),
  ('bk10', 'Software Engineering', 'Ian Sommerville', '9780134409870', 'Computer Science', 'Pearson', '2016-07-15', 'Covers agile methods, requirements engineering, system design, verification, maintenance and the tools used throughout a software project lifecycle.', '#37474F', 4, 2, 2, 'cs'),
  ('bk11', 'Artificial Intelligence: A Modern Approach', 'Stuart Russell, Peter Norvig', '9780134610993', 'Computer Science', 'Pearson', '2020-12-28', 'The standard AI textbook covering intelligent agents, search, planning, machine learning, deep learning, NLP and the societal impact of AI.', '#00838F', 3, 1, 2, 'cs'),
  ('bk12', 'The C Programming Language', 'Brian Kernighan, Dennis Ritchie', '9780131103627', 'Computer Science', 'Prentice Hall', '1988-04-01', 'The original C book by the creators of the language. A compact guide to pointers, structures, memory and the standard library.', '#5D4037', 4, 3, 2, 'cs'),
  ('bk13', 'Head First Design Patterns', 'Eric Freeman, Elisabeth Robson', '9781492078005', 'Computer Science', 'O''Reilly Media', '2020-12-03', 'A visual, brain-friendly guide to the classic Gang of Four design patterns with real Java examples and practical application design advice.', '#6A1B9A', 3, 2, 2, 'cs'),
  ('bk14', 'Don''t Make Me Think', 'Steve Krug', '9780134394909', 'Web Design', 'New Riders', '2014-09-02', 'A common-sense approach to web usability. Short, practical chapters on intuitive navigation, first-time visitors and mobile usability testing.', '#004D40', 3, 1, 3, 'it'),
  ('bk15', 'Discrete Mathematics and Its Applications', 'Kenneth H. Rosen', '9780073371290', 'Mathematics', 'McGraw-Hill', '2018-04-03', 'Covers logic, proofs, sets, functions, relations, graph theory, combinatorics and probability for undergraduate computer science students.', '#B71C1C', 6, 5, 1, 'cs'),
  ('bk16', 'Engineering Mathematics', 'B.S. Grewal', '9788174093226', 'Mathematics', 'Khanna Publishers', '2016-01-01', 'The standard Indian engineering mathematics reference covering calculus, differential equations, transforms, vectors and complex analysis.', '#33691E', 8, 6, 1, 'ee'),
  ('bk17', 'Fundamentals of Electric Circuits', 'Matthew Alexander, Sadiku', '9780073380667', 'Electronics', 'McGraw-Hill', '2021-01-15', 'A circuit-analysis text with a problems-first approach, covering nodal analysis, AC circuits, frequency response and two-port networks.', '#4E342E', 5, 3, 2, 'ee'),
  ('bk18', 'Signals and Systems', 'Alan V. Oppenheim, Alan Willsky', '9780132071222', 'Electronics', 'Pearson', '2012-09-03', 'Classic treatment of continuous and discrete-time signals, LTI systems, Fourier analysis, Laplace and Z transforms.', '#1A237E', 4, 2, 2, 'ec'),
  ('bk19', 'Thermodynamics: An Engineering Approach', 'Yunus Cengel, Michael Boles', '9780073398174', 'Mechanical Engineering', 'McGraw-Hill', '2019-01-09', 'An engineering-first presentation of thermodynamics with illustrations drawn from real systems such as engines, power plants and refrigerators.', '#BF360C', 5, 3, 3, 'me'),
  ('bk20', 'Fluid Mechanics', 'Frank M. White', '9780073398273', 'Mechanical Engineering', 'McGraw-Hill', '2020-01-01', 'Balances fundamental theory with applied engineering examples covering control volumes, viscous flow, dimensional analysis and open-channel flow.', '#01579B', 4, 2, 3, 'me'),
  ('bk21', 'Principles of Management', 'P.C. Tripathi, P.N. Reddy', '9780195688979', 'Management', 'Oxford University Press', '2017-07-01', 'A widely used management text covering planning, organising, staffing, directing and controlling with Indian business examples.', '#2E7D32', 6, 4, 1, 'mgmt'),
  ('bk22', 'Marketing Management', 'Philip Kotler, Kevin Keller', '9789386873246', 'Management', 'Pearson', '2016-01-01', 'The definitive marketing reference covering market analysis, STP strategy, the marketing mix, digital channels and brand management.', '#880E4F', 5, 3, 1, 'mgmt'),
  ('bk23', 'Organizational Behaviour', 'Stephen P. Robbins, Timothy Judge', '9789386423160', 'Management', 'Pearson', '2019-03-01', 'Explores individual, group and structural factors that influence behaviour inside organisations, with research-based models and caselets.', '#F57F17', 5, 4, 1, 'mgmt'),
  ('bk24', 'The Lean Startup', 'Eric Ries', '9780307888082', 'Management', 'Crown Business', '2011-09-13', 'A method for developing products and businesses through validated learning, rapid experimentation and iterative product releases.', '#4A148C', 3, 1, 1, 'mgmt'),
  ('bk25', 'A Brief History of Time', 'Stephen Hawking', '9780553380163', 'Science', 'Bantam Books', '1998-09-01', 'A cosmology classic for general readers covering the Big Bang, black holes, the arrow of time and the search for a unified theory.', '#263238', 4, 3, 1, 'sc'),
  ('bk26', 'Wings of Fire', 'A.P.J. Abdul Kalam', '9788173711466', 'Biography', 'Universities Press', '1999-01-01', 'The autobiography of Dr. A.P.J. Abdul Kalam, tracing his journey from Rameswaram to India’s space and missile programmes.', '#C62828', 5, 4, 4, 'sc'),
  ('bk27', 'Think and Grow Rich', 'Napoleon Hill', '9788192495453', 'Self Development', 'Vigyan Prakashan', '2017-05-01', 'A classic personal-development book built on thirteen principles of success gathered from studying hundreds of accomplished people.', '#F9A825', 4, 2, 4, 'mgmt'),
  ('bk28', 'Reinforced Concrete Design', 'S. Ramamrutham', '9788120327276', 'Civil Engineering', 'Dhanpat Rai Publishing', '2014-01-01', 'Design reference for limit-state method problems covering beams, slabs, columns, footings and prestressed concrete as per IS codes.', '#006064', 5, 4, 3, 'ce')
ON CONFLICT (id) DO NOTHING;

INSERT INTO public.transactions
  (id, student_id, book_id, issue_date, due_date, return_date, fine, status) VALUES
  ('tx1', 'st1', 'bk1', '2026-09-28', '2026-10-12', NULL, 0, 'issued'),
  ('tx2', 'st1', 'bk5', '2026-09-26', '2026-10-10', NULL, 0, 'issued'),
  ('tx3', 'st1', 'bk7', '2026-10-01', '2026-10-15', NULL, 0, 'issued'),
  ('tx4', 'st1', 'bk6', '2026-09-05', '2026-09-19', '2026-09-18', 0, 'returned'),
  ('tx5', 'st1', 'bk12', '2026-08-18', '2026-09-01', '2026-09-03', 20, 'returned'),
  ('tx6', 'st1', 'bk26', '2026-08-02', '2026-08-16', '2026-08-15', 0, 'returned'),
  ('tx7', 'st1', 'bk14', '2026-09-12', '2026-10-04', '2026-10-07', 30, 'returned'),
  ('tx8', 'st1', 'bk16', '2026-07-10', '2026-07-24', '2026-07-23', 0, 'returned'),
  ('tx9', 'st2', 'bk3', '2026-09-20', '2026-10-04', NULL, 0, 'issued'),
  ('tx10', 'st3', 'bk9', '2026-09-25', '2026-10-09', NULL, 0, 'issued'),
  ('tx11', 'st4', 'bk22', '2026-09-18', '2026-10-02', NULL, 0, 'issued'),
  ('tx12', 'st5', 'bk11', '2026-10-02', '2026-10-16', NULL, 0, 'issued'),
  ('tx13', 'st6', 'bk21', '2026-09-30', '2026-10-14', NULL, 0, 'issued'),
  ('tx14', 'st7', 'bk15', '2026-09-22', '2026-10-06', NULL, 0, 'issued'),
  ('tx15', 'st8', 'bk19', '2026-10-03', '2026-10-17', NULL, 0, 'issued'),
  ('tx16', 'st10', 'bk13', '2026-09-29', '2026-10-13', NULL, 0, 'issued'),
  ('tx17', 'st12', 'bk17', '2026-09-27', '2026-10-11', NULL, 0, 'issued'),
  ('tx18', 'st14', 'bk18', '2026-09-24', '2026-10-08', NULL, 0, 'issued'),
  ('tx19', 'st16', 'bk24', '2026-10-06', '2026-10-20', NULL, 0, 'issued'),
  ('tx20', 'st3', 'bk10', '2026-10-05', '2026-10-19', NULL, 0, 'issued'),
  ('tx21', 'st9', 'bk2', '2026-09-14', '2026-09-28', '2026-09-28', 0, 'returned'),
  ('tx22', 'st9', 'bk2', '2026-08-20', '2026-09-03', '2026-09-10', 70, 'returned'),
  ('tx23', 'st11', 'bk25', '2026-09-06', '2026-09-20', '2026-09-19', 0, 'returned'),
  ('tx24', 'st13', 'bk28', '2026-08-17', '2026-08-31', '2026-09-02', 20, 'returned'),
  ('tx25', 'st15', 'bk8', '2026-09-06', '2026-09-20', '2026-09-25', 50, 'returned'),
  ('tx26', 'st5', 'bk27', '2026-09-16', '2026-09-29', '2026-09-30', 10, 'returned'),
  ('tx27', 'st12', 'bk1', '2026-08-25', '2026-09-08', '2026-09-07', 0, 'returned'),
  ('tx28', 'st2', 'bk4', '2026-08-14', '2026-08-28', '2026-09-01', 40, 'returned'),
  ('tx29', 'st6', 'bk23', '2026-09-02', '2026-09-16', '2026-09-15', 0, 'returned'),
  ('tx30', 'st8', 'bk20', '2026-08-24', '2026-09-07', '2026-09-06', 0, 'returned')
ON CONFLICT (id) DO NOTHING;

INSERT INTO public.reservations
  (id, student_id, book_id, reservation_date, queue_position, status,
   expected_availability, pickup_deadline) VALUES
  ('rs1', 'st1', 'bk4', '2026-10-02', 2, 'waiting', '2026-10-14', NULL),
  ('rs2', 'st1', 'bk11', '2026-10-05', 1, 'ready', NULL, '2026-10-09'),
  ('rs3', 'st1', 'bk8', '2026-09-18', 1, 'expired', '2026-09-22', '2026-09-25'),
  ('rs4', 'st3', 'bk24', '2026-10-04', 1, 'waiting', '2026-10-18', NULL),
  ('rs5', 'st6', 'bk4', '2026-10-06', 3, 'waiting', '2026-10-21', NULL),
  ('rs6', 'st9', 'bk13', '2026-10-01', 1, 'ready', NULL, '2026-10-08'),
  ('rs7', 'st12', 'bk22', '2026-09-15', 1, 'fulfilled', NULL, '2026-09-20'),
  ('rs8', 'st15', 'bk19', '2026-09-10', 2, 'expired', '2026-09-16', '2026-09-19')
ON CONFLICT (id) DO NOTHING;

INSERT INTO public.fines
  (id, student_id, book_id, due_date, return_date, overdue_days, amount, status, paid_date) VALUES
  ('fn1', 'st1', 'bk12', '2026-09-01', '2026-09-03', 2, 10, 'paid', '2026-09-05'),
  ('fn2', 'st1', 'bk14', '2026-10-04', '2026-10-09', 5, 25, 'pending', NULL),
  ('fn3', 'st9', 'bk2', '2026-09-03', '2026-09-10', 7, 35, 'paid', '2026-09-12'),
  ('fn4', 'st13', 'bk28', '2026-08-31', '2026-09-02', 2, 10, 'paid', '2026-09-04'),
  ('fn5', 'st5', 'bk27', '2026-09-29', '2026-09-30', 1, 5, 'pending', NULL),
  ('fn6', 'st2', 'bk4', '2026-08-28', '2026-09-01', 4, 20, 'paid', '2026-09-03'),
  ('fn7', 'st15', 'bk8', '2026-09-20', '2026-09-25', 5, 25, 'pending', NULL)
ON CONFLICT (id) DO NOTHING;

INSERT INTO public.notifications
  (id, type, title, message, time, read, user_id) VALUES
  ('nt1', 'due', 'Due date reminder', '"The Psychology of Money" is due on 10 Oct 2026. Please return it on time to avoid a fine.', '2026-10-07T09:15:00', false, 'st1'),
  ('nt2', 'reservation', 'Reservation ready for pickup', '"Artificial Intelligence: A Modern Approach" is ready for pickup at the Engineering Library until 09 Oct.', '2026-10-06T14:30:00', false, 'st1'),
  ('nt3', 'fine', 'Fine of ₹25 pending', 'A fine of ₹25 was recorded for "Don''t Make Me Think" (5 days overdue). Please clear it at the library counter.', '2026-10-07T08:05:00', false, 'st1'),
  ('nt4', 'due', 'Due date reminder', '"Clean Code" is due on 12 Oct 2026.', '2026-10-06T10:00:00', true, 'st1'),
  ('nt5', 'announcement', 'Library closed on 18 October', 'The Main Library will be closed on 18 Oct 2026 for annual inventory. Please plan your borrows accordingly.', '2026-10-04T16:45:00', true, 'all'),
  ('nt6', 'reservation', 'Reservation expired', 'Your reservation for "Computer Networking: A Top-Down Approach" expired on 25 Sep. You can reserve it again.', '2026-09-25T11:00:00', true, 'st1'),
  ('nt7', 'overdue', 'Overdue books need attention', '5 books are overdue across the library. Reminder SMS have been queued for the borrowers.', '2026-10-07T08:30:00', false, 'ad1'),
  ('nt8', 'announcement', 'New students registered', '24 new student accounts were imported from the college ERP this morning.', '2026-10-07T07:50:00', false, 'ad1'),
  ('nt9', 'announcement', 'Bulk import completed', 'Book import finished: 48 added, 2 failed, 1 skipped. Review the error report in Bulk Import.', '2026-10-05T12:20:00', true, 'ad1'),
  ('nt10', 'announcement', 'Mid-semester reading week', 'Loan period is extended to 21 days during reading week (12–19 Oct).', '2026-10-03T09:00:00', true, 'all')
ON CONFLICT (id) DO NOTHING;

COMMIT;
