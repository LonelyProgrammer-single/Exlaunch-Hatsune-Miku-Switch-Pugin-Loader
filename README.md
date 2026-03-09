# exlaunch
A framework for injecting C/C++ code into Nintendo Switch applications/applet/sysmodules.
Forked specifically for Hatsune Miku Project Diva Megamix!
I plan to port most cool hook based pacthes made for Project Diva Megamix+ for PC to Nintendo Switch, starting with ones from DivaModLoader.
^.^

> [!NOTE]
> This project is a work in progress. If you have issues, reach out to `lsmsmx` on Discord.

# Current Features
- Full support of mod_ prefix! Making it easy to install mods as on PC.
- No Songs limit, saving scores and modules and custom items in external savedata
- Song ID limit up to 24576 (can be increased)
- Increased Limit Of Spritesets to be loaded from 4096 to 32768
- No Module, COS limits
- Aet effects limit increased from 83 to 256
- Huge full implementation of str_array code injection to make modules to work properly without crashing the game, since simple patches wasn't enough. Custom hairs work too!
- Increased limit of lyric entries from 150 to 1000 in pv_db
- Mod_str_array.toml support
- Challenge time for all difficulties
- Saturation patch for my friend
- Scripts for parsing and merging multiple pv_db/toml into one; for easy NC->AC

# Instructions
- External save file is located in sdmc:/DMLSwitchPort/Save/
- mod_str_array.toml can be placed in sdmc:/DMLSwitchPort/lang2/ folder. Apparently you can also use game dlc prefixes, idea by Dandy Bleat. But with new mods support, just don't touch the location of already existing mod specific mod_str_array.toml and enjoy names
- Drop your mods in sdmc:/atmosphere/contents/TitleID/romfs/mods (to be fair, you still have to convert and rename in pv_db usm->mp4)
- You can manage your priority of mods in sdmc:/DMLSwitchPort/config.toml file

# TODO
- Debug mode and test menu port from pc (not huge implementations)
- Freecam (same as debug)
- Fixing some hairs don't appear as separate entry
- Proper pv_parser to not make game iterate every possible pv_id (i keep failing to make it not crash the game)
- Fixing issue where pv_db info about singers doesn't apply and it always sets to default miku (prob have to rewrite saving logic, but the issue itself isn't that big of a deal, so not going to fix soon)

# Credit
- Atmosphère: A great reference and guide.
- oss-rtld: Included for (pending) interop with rtld in applications (License [here](https://github.com/shadowninja108/exlaunch/blob/main/source/lib/reloc/rtld/LICENSE.txt)).
- DML https://github.com/blueskythlikesclouds/DivaModLoader/tree/master/Source/DivaModLoader
