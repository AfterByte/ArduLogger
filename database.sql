CREATE DATABASE ardulogger;
USE ardulogger;

CREATE TABLE `users` (
  `id` int AUTO_INCREMENT PRIMARY KEY,
  `name` varchar(40),
  `email` varchar(40) UNIQUE,
  `password` varchar(40),
  `fingerprint` varchar(40) UNIQUE
);
