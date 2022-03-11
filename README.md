# ReplayMap Fixer
On August 25 the official 1.32.10.17734 Warcraft III patch aired, turning thousands of Netease (.nwg) replays unwatchable.

patch | Battle-net replays | W3c replays | NetEase replays
:---- | :----------------: | :---------: | :-------------:
1.32.10.17380 | yes | yes | yes
1.32.10.17734 | yes | yes | **no**

This is a simple tool for fixing those replays.

## How to use
1. Place the replays you'd want to convert to /Replays/ folder
2. Run the .exe file
3. You are about to find the converted replays in the /Converted/ folder

In case you get into any kind of trouble, this might mean the script was not able to copy the maps to your local folder.  
You should try doing it manually then. The structure shall be as following:  


***Documents/Warcraft III/1.32.10.17380/ \*the maps (.w3x/.w3m files) are here***


## A problem and a fix explained
The patch did **not** have a full patch-note covering the changes. But quite soon after it's deployment it was clear that
a huge number of replays were no longer accessible *via the game client*.  

Those affected seemed to be ***Netease ones exclusively***. NetEase replays, played on *standard maps*, precisely speaking. The W3C maps **Autumn Leaves**
and the **Tidehunters** were **OK**.

The issue to cause this behaviour was the fact the Blizzard decided do update the standard maps themselves.  
Each replay stores a ***full path to a map*** which it was played on.  
It stores a ***hash*** of that map as well in order to make sure the map is exactly the same.

The problem with the changed maps was that the game **could find** the map by the path as maps were the standard ones. But right after that,
when it was checking if the hash is the same, a discrepancy occurred.

#### An obvious solution
An easy solution would've been to create your own ***documents/warcraft III/maps/frozenthrone/*** folder and fill it with the "old" maps. 
This way it would have caused the eclipsis of the in-game maps as it usually happens with \_retail\_ files.

Unfortunately, this causes a bug and the game might even crush (as the two maps are exact by path, whilst their hashes are not). It is
not even possible to use such map to host a custom game. 

#### Solution
The problem might be solved by intrusively changing the map path inside of the replay to another folder where our "old" maps are.
- First, we decompress the replay
- Then we check if the map used belongs to the list of those changed, and if it does, then we change a path to Maps/1.32.10.17380/MapName.w3x
- After that we compress the replay once again

### A few notes on implementation
This tool uses a legacy code of [my old replay-parser](https://github.com/l-Fingon-l/ReplayParser) as it's core. 
That code is quite old a pretty ugly (and I have even got into some trouble while trying to recall what was my year-old code designed to do)
but it is incredibly fast and it does its job.  
Some edits were made and a couple of optimizations were added, though.

---
A fixer for Warcraft III patch 1.32.10 Netease (.nwg) replays, broken by official 1.32.10.17734 patch.
