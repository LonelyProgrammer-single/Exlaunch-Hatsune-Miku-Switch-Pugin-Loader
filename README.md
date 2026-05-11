# exlaunch
A framework for injecting C/C++ code into Nintendo Switch applications/applet/sysmodules.
Forked specifically for Hatsune Miku Project Diva Megamix!
I plan to port most cool hook based pacthes made for Project Diva Megamix+ for PC to Nintendo Switch, starting with ones from DivaModLoader.
^.^

> [!NOTE]
> This project is a work in progress. If you have issues, reach out to `lsmsmx` on Discord.

-------------------------------------------------------------------------------------------------------------------------

# Current Features
- Full support of mod_ prefix! Making it easy to install mods as on PC.
- DEBUG AND FREECAM
- No Songs limit, saving scores and modules and custom items in external savedata
- Song ID limit up to 24576 (can be increased)
- Increased Limit Of Spritesets to be loaded from 4096 to 32768
- Subsurface Scattering restoration
- FXAA and forced toon remove in customization menu.
- No Module, COS limits
- Aet effects limit increased from 83 to 256
- Huge full implementation of str_array code injection to make modules to work properly without crashing the game, since simple patches wasn't enough. Custom hairs work too!
- Increased limit of lyric entries from 150 to 1000 in pv_db
- Mod_str_array.toml support
- Challenge time for all difficulties
- Saturation patch for my friend
- Scripts for parsing and merging multiple pv_db/toml into one; for easy NC->AC
- Trash Self Shadow Remove
- Bone Control through ImGUI
- Record/Play recorded motions through ImGUI
- Input Track Overlay through ImGUI

# Instructions
- External save file is located in sdmc:/DMLSwitchPort/Save/
- mod_str_array.toml can be placed in sdmc:/DMLSwitchPort/lang2/ folder. Apparently you can also use game dlc prefixes, idea by Dandy Bleat. But with new mods support, just don't touch the location of already existing mod specific mod_str_array.toml and enjoy names
- Drop your mods in sdmc:/atmosphere/contents/TitleID/romfs/mods (to be fair, you still have to convert and rename in pv_db usm->mp4)
- You can manage your priority of mods in sdmc:/DMLSwitchPort/config.toml file
- Motion dumps in sdmc:/DMLSwitchPort/Dump/*.txt


-------------------------------------------------------------------------------------------------------------------------

# Debug and Freecam
Introducing fully ported **Debug mode** and **Freecam** mod from PC to Nintendo Switch! Oh, and rendering while paused.
NOTE: Debug and freecam is totally disabled by default. You should go to global config.toml at sdmc:/DMLSwitchPort/ and change `debug = false` to `debug = true`

### Here is a list of debug and freecam features:
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
- (ZR_Hold + ZL_Hold)  for 0.5 seconds — Quick switch between in-game debug and imgui window

  

-------------------------------------------------------------------------------------------------------------------------


### ImGUI API Integration for Bones Control and Animation Recorder Player and Input Tracker Overlay:

- ImGui interface can be toggled by the same way as you did with debug (Tesla Overlay Toggle https://github.com/lsmsmx/MMTeslaNS/releases/tag/v2.0 and txt file 0/1 toggle for people on emulators (delete old emu_states.txt!!!)
- The controls are pretty much the same as in debug, including a mouse movement speed increase by holding Y button
- Made mostly for posing and making shots
- To start recording, reset and freeze motion (for example using "MOTION TEST" window and selecting "STEP"), then click "Arm Recording", then get back to in-game debug motion window and start playing anim set, as it starts recording it. By using the Range, you can record specific part of body, but it sometimes doesn't work the way you think it does, for example its possible to record face motions and fingers motions without issue, but some recordings may curse the model if they're used outside of specific motion (perhaps due to being tied to coords in world). But its not hard to pose body on your own except of face.
- You can load as many recorded anims as you want at the same time but it may break the physics lol so use it with caution. Click "Open Ultimate Motion Player" to select and play motion you recorded (you can rename motions txt files), you can delete the slot or add new one, and of course you can use slider to go between frames.  Don't forget to check and uncheck the check mark.
- Motion Control tab offers you a variety of stuff but mostly it's: select a current bone use sliders to rotate and move, there are no specific ways to tell what each index does, it's basically hooking bone matrix update func, but at least i had enough courage to include some useful information about bones. 
- Toggling Input Overlay will open a new imgui window that is tied to centered bottom of screen, no user interaction, slight transparency. Tracks all your inputs. Useful for people who records videos. Will look like Nintendo Switch Grip for Joycons (yk where in vids of diva gameplay there is ds4 overlay, so it's same but for switch, wont disturb you from gameplay due to small size)
- Mouth Focus Switch by holding ZR + ZL buttons for 0.5 seconds for quick switch between in-game debug and imgui window

-------------------------------------------------------------------------------------------------------------------------

### Other info:
- There will be file created with all debug states for users who can't use Tesla overlays: sdmc:/DMLSwitchPort/emu_states.txt that will contain such format:
`Format: StID SubID Trigger # Name`
 `3 11 0 # 11: DT_STG`
so `3` is StateID
     `11` is SubStateID
     `0` is Trigger
     `# 11: DT_STG` is # Name
Set 'Trigger' to 1 and save to jump. Once you save you will be redirected to the substate you chose. 1 will be set back to 0 after jump.
- For ImGUI there will be:
  `Change 0 to 1 to enable, 1 to 0 to disable."
  ImGUI_Debug_Window = 0
  InputOverlay = 0`
- All that debug stuff is inspired by original deck window from MM+ Debug mod by `nastys`
- Big thanks to https://github.com/Retinalogic/imgui-nvn [subsdk8 by him]
- Bone Control is a part of DivaImGUI by lybxlpsv https://github.com/lybxlpsv/divaimgui that i ported thx to him<3
- You can totally disable debug and freecam by having `debug = false` in global config file. This way, none of hotkeys and state selectors can be used.
- Debug may crash randomly when you select something. From my experience, the same random crashes happen in MM+
- Aet_DT is known to crash, even tho it works fine in MM+. It seems to fail at `strlen and cmp` or something. No solution was found, even after disabling half of plugin features and all mods even LayerFS ones.
- Saving light_param or any other config makes game crash. Tried few times to do something in FsHooks, no luck.
- Aet with high IDs are making game to work horribly unstable. It allocates memory for every id entry, so half of game crash with just couple of high ids. There is actually fix for it that i may port. 
- There seem to be some other limitations, me with tons of mods, using high demanding modules, i can't load all 6 of them at the same time, game crash when i load 3 of them, however for a person without mods at all except of modules one, it crashes when he loads 6th performer in customization menu. Well, not surpising, Switch has to use 3.1 GB for RAM and VRAM... So make sure to not overload the ame with mods.


**Tesla overlay** by me:
https://github.com/lsmsmx/MMDebugSwitcherNS/releases/tag/v2.0

# TODO
- Restore FT shadow (something i probably never will be able to do :< guess it's okay)
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
