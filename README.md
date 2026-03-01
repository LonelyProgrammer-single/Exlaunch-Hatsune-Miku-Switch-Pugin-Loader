# exlaunch
A framework for injecting C/C++ code into Nintendo Switch applications/applet/sysmodules.
Forked specifically for Hatsune Miku Project Diva Megamix!
I plan to port most cool hook based pacthes made for Project Diva Megamix+ for PC to Nintendo Switch, starting with ones from DivaModLoader.
^.^

> [!NOTE]
> This project is a work in progress. If you have issues, reach out to `lsmsmx` on Discord.

# Current Features

- No Songs limit, saving scores and nodules and custom items in external savedata
- Song ID limit up to 24576
- No Module, COS limits
- Aet effects limit increased from 83 to 256
- Huge full implementation of str_array code injection to make modules to work properly without crashing the game, since simple patches wasn't enough. Custom hairs work too!
- Increased limit of lyric entries from 150 to 1000 in pv_db
- Mod_str_array.toml support
- Saturation patch for my friend
- Scripts for parsing and merging multiple pv_db/toml into one.

# Credit
- Atmosphère: A great reference and guide.
- oss-rtld: Included for (pending) interop with rtld in applications (License [here](https://github.com/shadowninja108/exlaunch/blob/main/source/lib/reloc/rtld/LICENSE.txt)).
- DML https://github.com/blueskythlikesclouds/DivaModLoader/tree/master/Source/DivaModLoader
