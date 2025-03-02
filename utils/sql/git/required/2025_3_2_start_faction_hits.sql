CREATE TABLE `start_faction_hits`  (
  `player_choice` int(2) NOT NULL DEFAULT 0,
  `player_class` int(2) NOT NULL DEFAULT 0,
  `player_deity` int(4) NOT NULL DEFAULT 0,
  `player_race` int(4) NOT NULL DEFAULT 0,
  `faction_id` int(11) NOT NULL,
  `faction_amount` int(11) NOT NULL,
  PRIMARY KEY (`player_choice`, `player_race`, `player_class`, `player_deity`) USING BTREE
) ENGINE = MyISAM CHARACTER SET = latin1 COLLATE = latin1_swedish_ci ROW_FORMAT = Fixed;