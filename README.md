# exlaunch
A framework for injecting C/C++ code into Nintendo Switch applications/applet/sysmodules.
Forked specifically for Hatsune Miku Project Diva Megamix!
I plan to port most cool hook based pacthes made for Project Diva Megamix+ for PC to Nintendo Switch, starting with ones from DivaModLoader.
^.^

> [!NOTE]
> This project is a work in progress. If you have issues, reach out to `lsmsmx` on Discord.

# Current Features
- Full support of mod_ prefix! Making it easy to install mods as on PC.
- DEBUG AND FREECAM
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


-------------------------------------------------------------------------------------------------------------------------

# Debug and Freecam
Introducing fully ported **Debug mode** and **Freecam** mod from PC to Nintendo Switch! Oh, and rendering while paused.
NOTE: Debug and freecam is totally disabled by default. You should go to global config.toml at sdmc:/DMLSwitchPort/ and change `debug = false` to `debug = true`

### Here is a list of debug and freecam of features:
- Access _most_ of debug game substates
- Advanced freecam in Rhythm game, PV modes; In customization menu as well; Pausing will make game render stuff
- Use mouse to open a real time dw gui for other debug windows by right click (Ctrl + RMC alternative)
- Full usability with joycons/controller (A LOT of hotkeys, listed below)
- Physical mouse support
- Changing game substates two different ways: through Tesla overlay; through txt file (a template where you change desired substate trigger from 0 to 1);
- Generate **osage play data** in corresponding substate! Move files from `sdmc:/atmosphere/contents/TID/romfs/ram/osage_play_data_tmp/0/` to `osage_play_data` directory (for example create a new mod, or put in any dlc folder)
- Quickly jump to Main Menu
- Included removed limits (SWITCH EXCLUSIVE) for stages and modules and a3d (cos limit, three digits truncations; these are specific to debug)
- Excluding any (ig?) conflicts with game and between each other making it possible to use freecam in debug


### Hotkeys:

**Freecam**
- (L + R + Minus) — Enable/Disable Freecam
- (D-Pad + Up-Down-Left-Right) — Move Forward-Backward-Left-Right
- (Right stick) — Look in a desired direction judged by stick movement
- (B/X) — Move Down/Up
- (L/R) — Zoom Out/In
- (Hold_Y + L/R) — Rotate (lean) Left/Right
- (Hold_R3) — Speed boost of any camera movement 
 _note_: you can still use A button to pause without menu and capture beauty

**Debug Mode**
- (L + R + Plus) — Enable/Disable Debug Mode
- (L3) — Toggle Mouse
- (Left stick) — Mouse control
- (ZL) — Left Mouse Click
- (ZR) — Right Mouse Click
- (Y_Hold) — Mouse movement speed boost
- (L + R + D-Pad_Up) — Fast travel to Test Mode State
- (L + R + D-Pad_Down) — Fast travel to Data Test State
- (L + R + D-Pad_Left) — Fast travel to CS Menu (umm, instant crash tho)
- (L + R + D-Pad_Right) — Fast travel to Data Menu Switch (very useful to quickly jump to main menu)


### Other info:
- There will be file created with all debug states for users who can't use Tesla overlays: sdmc:/DMLSwitchPort/emu_states.txt that will contain such format:
`Format: StID SubID Trigger # Name`
 `3 11 0 # 11: DT_STG`
so `3` is StateID
     `11` is SubStateID
     `0` is Trigger
     `# 11: DT_STG` is # Name
Set 'Trigger' to 1 and save to jump. Once you save you will be redirected to the substate you chose. 1 will be set back to 0 after jump.
- All that debug stuff is inspired by original deck window from MM+ Debug mod by `nastys`
- You can totally disable debug and freecam by having `debug = false` in global config file. This way, none of hotkeys and state selectors can be used.
- Debug may crash randomly when you select something. From my experience, the same random crashes happen in MM+
- Aet_DT is known to crash, even tho it works fine in MM+. It seems to fail at `strlen and cmp` or something. No solution was found, even after disabling half of plugin features and all mods even LayerFS ones.
- Saving light_param or any other config makes game crash. Tried few times to do something in FsHooks, no luck.


**Tesla overlay** by me:
https://github.com/lsmsmx/MMDebugSwitcherNS/releases/tag/v1.0

# TODO
- Fixing some hairs don't appear as separate entry
- Proper pv_parser to not make game iterate every possible pv_id (i keep failing to make it not crash the game)
- Fixing issue where pv_db info about singers doesn't apply and it always sets to default miku (prob have to rewrite saving logic, but the issue itself isn't that big of a deal, so not going to fix soon)

# TitleID to be used
const char *possible_tids[] = {

       - "0100F3100DA46000", // JP (Mega39s)
       - "01001CC00FA1A000", // EN (MegaMix)
       - "0100BE300FF62000"  // KR (Mega39s)
};    

# Credit
- Atmosphère: A great reference and guide.
- oss-rtld: Included for (pending) interop with rtld in applications (License [here](https://github.com/shadowninja108/exlaunch/blob/main/source/lib/reloc/rtld/LICENSE.txt)).
- DML https://github.com/blueskythlikesclouds/DivaModLoader/tree/master/Source/DivaModLoader
