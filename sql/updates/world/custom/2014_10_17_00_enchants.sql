-- --------------------------------------------------------
-- Host:                         127.0.0.1
-- Server version:               5.6.19 - MySQL Community Server (GPL)
-- Server OS:                    Win64
-- HeidiSQL Version:             8.3.0.4694
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

-- Dumping structure for table world.hcl4_enchantids
DROP TABLE IF EXISTS `hcl4_enchantids`;
CREATE TABLE IF NOT EXISTS `hcl4_enchantids` (
  `InventoryType` int(11) NOT NULL,
  `id` int(11) NOT NULL,
  PRIMARY KEY (`InventoryType`,`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Dumping data for table world.hcl4_enchantids: ~48 rows (approximately)
/*!40000 ALTER TABLE `hcl4_enchantids` DISABLE KEYS */;
INSERT INTO `hcl4_enchantids` (`InventoryType`, `id`) VALUES
	(5, 20025),
	(5, 20026),
	(5, 20028),
	(5, 27957),
	(5, 27960),
	(5, 33991),
	(7, 2833),
	(8, 13890),
	(8, 20020),
	(8, 20023),
	(8, 63746),
	(9, 20008),
	(9, 20010),
	(9, 20011),
	(9, 23801),
	(9, 23802),
	(10, 13939),
	(10, 13948),
	(10, 25073),
	(10, 25074),
	(10, 25078),
	(10, 25079),
	(10, 25080),
	(13, 13898),
	(13, 20031),
	(13, 20032),
	(13, 20034),
	(13, 22749),
	(13, 23799),
	(13, 23800),
	(13, 23803),
	(13, 23804),
	(14, 13689),
	(14, 20016),
	(14, 20017),
	(14, 29454),
	(16, 13522),
	(16, 13882),
	(16, 20015),
	(16, 25081),
	(16, 25082),
	(16, 25083),
	(16, 27947),
	(17, 7218),
	(17, 20030),
	(17, 27837);
/*!40000 ALTER TABLE `hcl4_enchantids` ENABLE KEYS */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IF(@OLD_FOREIGN_KEY_CHECKS IS NULL, 1, @OLD_FOREIGN_KEY_CHECKS) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
