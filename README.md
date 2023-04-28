# Mouse Aiming Data Collector
## Intro
This is an Unreal Engine 5 game which collects information on how the user moves their mouse while aiming at various types of targets. It's similar to aim training software for first-person shooter games.

## Gamemodes
The game contains the following modes:

- Stationary mode: The user aims at stationary targets which appear only in front of them. The goal is to aim to and click on the target. A new target will spawn every time the player clicks and the old target will be destroyed.
- Moving mode: Moving targets instead of stationary targets. The goal is the same as the Stationary mode.
- Tracking mode: The goal is to keep the mouse on top of a moving target, tracking it as much as possible.
- Sphere mode (aka Large Angle mode): Stationary targets can appear anywhere in a sphere around the player. The goal is the same as Stationary mode.

## How to play
To play, download the latest release and extract the folder anywhere. To play a specific gamemode, run the file called "Start {gamemode name}.bat". This will start the game in that gamemode. Running the "MTDataCollector.exe" file will only run the game in Moving mode.To use the data collector, download this release and extract the folder anywhere. To play a specific gamemode, run the file called "Start {gamemode name}.bat". This will start the game in that gamemode. Running the "MTDataCollector.exe" file will only run the game in Moving mode.

The "escape" key will bring up a menu where you can edit your mouse sensitivity. Do this *before* left clicking to start the game. Once you're ready, left-click to start and targets will appear. Every game goes for 5 minutes. Once the 5 minutes are over, the game will automatically close. 

## Data
All data files will be located in the "...\MTDataCollector\Config\DATA" folder. It is subdivided into folders for every mode, containing one csv file for every game played.
