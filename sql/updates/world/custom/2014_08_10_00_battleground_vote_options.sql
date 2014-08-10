/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;

-- Dumping structure for table world.battleground_vote_options
DROP TABLE IF EXISTS `battleground_vote_options`;
CREATE TABLE IF NOT EXISTS `battleground_vote_options` (
  `id` tinyint(3) unsigned NOT NULL AUTO_INCREMENT,
  `phase` tinyint(3) unsigned DEFAULT '0',
  `mode` tinyint(3) DEFAULT '0',
  `value` tinyint(3) unsigned DEFAULT '0',
  `name` varchar(20) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=20 DEFAULT CHARSET=utf8;

-- Dumping data for table world.battleground_vote_options: ~19 rows (approximately)
DELETE FROM `battleground_vote_options`;
/*!40000 ALTER TABLE `battleground_vote_options` DISABLE KEYS */;
INSERT INTO `battleground_vote_options` (`id`, `phase`, `mode`, `value`, `name`) VALUES
	(1, 1, -1, 0, 'Capture the Flag'),
	(2, 1, -1, 1, 'Team Deathmatch'),
	(3, 1, -1, 2, 'Deathmatch'),
	(4, 2, -1, 10, '10 minutes'),
	(5, 2, -1, 20, '20 minutes'),
	(6, 2, -1, 30, '30 minutes'),
	(7, 2, -1, 60, '60 minutes'),
	(8, 3, 0, 1, '1 flag'),
	(9, 3, 0, 3, '3 flags'),
	(10, 3, 0, 5, '5 flags'),
	(11, 3, 0, 10, '10 flags'),
	(12, 3, 1, 25, '25 kills'),
	(13, 3, 1, 50, '50 kills'),
	(14, 3, 1, 75, '75 kills'),
	(15, 3, 1, 100, '100 kills'),
	(16, 3, 2, 10, '10 kills'),
	(17, 3, 2, 20, '20 kills'),
	(18, 3, 2, 30, '30 kills'),
	(19, 3, 2, 50, '50 kills');
/*!40000 ALTER TABLE `battleground_vote_options` ENABLE KEYS */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IF(@OLD_FOREIGN_KEY_CHECKS IS NULL, 1, @OLD_FOREIGN_KEY_CHECKS) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
