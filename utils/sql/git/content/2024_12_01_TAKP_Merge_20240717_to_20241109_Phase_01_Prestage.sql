-- No longer need kedge_tryout
DELETE FROM spawn2 where zone = 'kedge_tryout'; 

-- Our custom spawn2 is overlapping with TAKP, move it away, far away so it never collides again (can be run multiple times due to id check at end)
UPDATE spawn2 set id = id + 10000000 where (zone in ('myriah','fireice','hate_instanced') or spawngroupID = 4501005) and id < 20000000;

-- Empty entries, like literally empty - no zone, all defaults
DELETE FROM spawn2 where id in (11449250,11449251);

-- skill caps table changed completely, so to allow an alter ahead of our update - we'll just flush it now
DELETE FROM skill_caps;
