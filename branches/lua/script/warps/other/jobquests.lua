--===== Athena Script ========================================
--= Job Quest Warp Script
--===== By: ==================================================
--= Athena (1.0)
--===== Current Version: =====================================
--= 1.3b
--===== Compatible With: =====================================
--= Any Athena Version; RO Version Ep4+
--===== Description: =========================================
--= Warp Points for Job Quest Maps
--===== Additional Comments: =================================
--= Split off npc_warp.txt
--= Commented some warps because new Job quests have correct
--= ones! Added missing warps for Thief/Hunter/Swordman [Lupus]
--= 1.3 Added missing warps Swordman [Lupus]
--= Reorganized.  Added complete rogue quest warps.
--= Removed unused, duplicate warps. [kobra_k88]
--= 1.3b activated Dancer JQ warp [Lupus]
--============================================================


--==============================================================================
--Novice
--==============================================================================
addwarp("new11","new_1-1.gat",148,112,"new_1-2.gat",100,9,2,3)
addwarp("new12","new_1-1.gat",166,112,"new_1-2.gat",100,153,2,2)
addwarp("new12-1","new_1-2.gat",100,150,"new_1-1.gat",162,112,2,1)
addwarp("new13","new_1-1.gat",169,75,"new_1-2.gat",182,163,2,2)
addwarp("new13-1","new_1-2.gat",182,159,"new_1-1.gat",165,75,2,2)
addwarp("new14","new_1-1.gat",169,147,"new_1-2.gat",18,163,2,2)
addwarp("new14-1","new_1-2.gat",18,159,"new_1-1.gat",165,147,2,2)
addwarp("new15","new_1-2.gat",100,122,"new_1-2.gat",100,162,2,1)
addwarp("new15-1","new_1-2.gat",100,165,"new_1-2.gat",100,119,2,1)
addwarp("new16","new_1-2.gat",126,106,"new_1-2.gat",156,171,2,4)
addwarp("new16-1","new_1-2.gat",153,171,"new_1-2.gat",123,106,2,4)
addwarp("new17","new_1-3.gat",96,175,"new_1-4.gat",100,14,3,2)

--==============================================================================
--Swordman
--==============================================================================
addwarp("SwordWarp0","sword_1-1.gat",192,244,"sword_1-1.gat",215,244,2,2)
addwarp("SwordWarp1","sword_1-1.gat",223,243,"sword_1-1.gat",12,206,2,2)
addwarp("SwordWarp2","sword_1-1.gat",192,206,"sword_1-1.gat",215,205,2,2)
addwarp("SwordWarp3","sword_1-1.gat",223,205,"sword_1-1.gat",12,168,2,2)
addwarp("SwordWarp4","sword_1-1.gat",192,168,"sword_1-1.gat",215,167,2,2)
addwarp("swd201","sword_2-1.gat",65,117,"sword_2-1.gat",10,245,1,1)
addwarp("swd202","sword_2-1.gat",98,27,"sword_2-1.gat",11,207,1,1)
addwarp("swd203","sword_2-1.gat",161,27,"sword_2-1.gat",11,207,1,1)
addwarp("swd204","sword_2-1.gat",223,205,"sword_2-1.gat",11,168,1,1)
addwarp("swd205","sword_2-1.gat",223,243,"sword_2-1.gat",11,206,1,1)
addwarp("swd206","sword_2-1.gat",239,117,"sword_2-1.gat",11,169,1,1)
addwarp("swd301","sword_3-1.gat",65,117,"sword_3-1.gat",10,245,1,1)
addwarp("swd302","sword_3-1.gat",98,27,"sword_3-1.gat",11,207,1,1)
addwarp("swd303","sword_3-1.gat",161,27,"sword_3-1.gat",11,207,1,1)
addwarp("swd304","sword_3-1.gat",223,205,"sword_3-1.gat",11,168,1,1)
addwarp("swd305","sword_3-1.gat",223,243,"sword_3-1.gat",11,206,1,1)
addwarp("swd306","sword_3-1.gat",239,117,"sword_3-1.gat",11,169,1,1)

-- Underground cave
addwarp("swd101","sword_1-1.gat",65,117,"sword_1-1.gat",10,245,1,1)
addwarp("swd102","sword_1-1.gat",98,27,"sword_1-1.gat",11,207,1,1)
addwarp("swd103","sword_1-1.gat",161,27,"sword_1-1.gat",11,207,1,1)
addwarp("swd106","sword_1-1.gat",239,117,"sword_1-1.gat",11,169,1,1)

--==============================================================================
--Thief
--==============================================================================
addwarp("jthf","job_thief1.gat",180,15,"moc_ruins.gat",145,117,7,1)

--==============================================================================
-- Assassin
--==============================================================================
addwarp("guild_to_16","in_moc_16.gat",18,8,"moc_fild16.gat",205,291,2,2)
addwarp("16_to_guild","moc_fild16.gat",205,296,"in_moc_16.gat",18,11,2,2)

--==============================================================================
-- Hunter
--==============================================================================
addwarp("jhun01","in_hunter.gat",100,15,"pay_fild10.gat",143,250,0,0)

--==============================================================================
-- Monk (St. Capitolina Abbey)
--==============================================================================
addwarp("monk15","prt_monk.gat",192,172,"monk_test.gat",329,50,1,1)
addwarp("monk16","monk_test.gat",329,47,"prt_monk.gat",193,166,1,1)
addwarp("monk17","monk_test.gat",329,76,"monk_test.gat",259,118,1,1)
addwarp("monk18","monk_test.gat",259,115,"monk_test.gat",329,71,1,1)
addwarp("monk19","monk_test.gat",272,125,"monk_test.gat",301,127,1,1)
addwarp("monk20","monk_test.gat",298,127,"monk_test.gat",268,125,1,1)

--==============================================================================
--Alchemist
--==============================================================================
--Town -- Alchemist , Alchemist -- Town
addwarp("alche01","aldebaran.gat",54,66,"alde_alche.gat",41,174,1,1)
addwarp("alche02","alde_alche.gat",41,171,"aldebaran.gat",56,68,1,1)

--Alchemist Hall 1st Floor Top Rooms
addwarp("alche03","alde_alche.gat",129,104,"alde_alche.gat",163,163,1,1)
addwarp("alche04","alde_alche.gat",159,163,"alde_alche.gat",126,104,1,1)
addwarp("alche05","alde_alche.gat",131,77,"alde_alche.gat",162,107,1,1)
addwarp("alche06","alde_alche.gat",159,107,"alde_alche.gat",128,77,1,1)

--Alchemist Hall 1st Floor Bottom Rooms
addwarp("alche07","alde_alche.gat",47,103,"alde_alche.gat",88,18,1,1)
addwarp("alche08","alde_alche.gat",92,18,"alde_alche.gat",50,103,1,1)
addwarp("alche09","alde_alche.gat",47,77,"alde_alche.gat",155,18,1,1)
addwarp("alche10","alde_alche.gat",159,18,"alde_alche.gat",50,77,1,1)

--Alchemist Hall 2nd Floor -- 1st Floor Link
addwarp("alche11","alde_alche.gat",14,184,"alde_alche.gat",88,113,1,1)
addwarp("alche12","alde_alche.gat",88,117,"alde_alche.gat",14,180,1,1)

--Alchemist Hall 2nd Floor Rooms
addwarp("alche13","alde_alche.gat",19,171,"aldebaran.gat",68,56,1,1)
addwarp("alche14","aldebaran.gat",66,54,"alde_alche.gat",19,175,1,1)
addwarp("alche15","alde_alche.gat",18,28,"alde_alche.gat",89,66,1,1)
addwarp("alche16","alde_alche.gat",89,63,"alde_alche.gat",18,24,1,1)

--Alchemist Hall 3rd Floor
addwarp("alche17","alde_alche.gat",41,187,"alde_alche.gat",114,178,1,1)
addwarp("alche18","alde_alche.gat",114,182,"alde_alche.gat",41,183,1,1)

--==============================================================================
--Rogue
--==============================================================================
addwarp("fild07-rogue00","cmd_fild07.gat",193,117,"in_rogue.gat",378,46,1,1)
addwarp("rogue00-fild07","in_rogue.gat",375,46,"cmd_fild07.gat",195,116,1,1)
addwarp("rogue01-02","in_rogue.gat",376,34,"in_rogue.gat",378,125,1,1)
addwarp("rogue02-01","in_rogue.gat",375,125,"in_rogue.gat",379,34,1,1)
addwarp("fild07-rogue03","cmd_fild07.gat",352,275,"in_rogue.gat",265,122,1,1)
addwarp("rogue03-fild07","in_rogue.gat",265,118,"cmd_fild07.gat",349,275,1,1)
addwarp("rogue04-fild09","in_rogue.gat",244,20,"cmd_fild09.gat",106,192,1,1)
addwarp("rogue05-fild09","in_rogue.gat",172,34,"cmd_fild09.gat",339,143,1,1)
addwarp("rogue06-fild04","in_rogue.gat",160,103,"cmd_fild04.gat",301,177,1,1)

--==============================================================================
--Dancer
--==============================================================================
addwarp("duncer1","job_duncer.gat",70,45,"comodo.gat",193,149,2,2)