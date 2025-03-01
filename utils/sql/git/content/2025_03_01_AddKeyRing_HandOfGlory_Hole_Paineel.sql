-- Hand of Glory (only the door behind Drusella, at the exit pad)
UPDATE doors SET nokeyring = 0 WHERE keyitem = 17274 and altkeyitem = 0;

-- Hole Key
UPDATE doors SET nokeyring = 0 WHERE keyitem = 6379 AND altkeyitem = 0;

-- Paineel Key
UPDATE doors SET nokeyring = 0 WHERE keyitem = 6378 AND altkeyitem = 0;
