BSP Viewer
==========
This is an app for viewing Half-Life 1 maps. Expects to find WAD files in *<workdir>* and maps in *<workdir>/maps*.


Console commands
----------------
- `quit` Exits the app
- `map <map name>` Loads a map (only map name, without .bsp)
- `dem_record <demo name>` Records a demo file (saves camera position and angles N times a second).
- `dem_stop` Stops demo recorfing
- `dem_play` Plays back a demo.
- `dem_tickrate <float>` How many times per seconds to write info into demo (def. 60).
- `fps_max <float>` FPS limit (def. 100)
- `m_sens <float>` Mouse sensitivity (degrees/pixel) (def. 0.15)
- `cam_speed <float>` Speed of the camera (def. 1000)
- `fov <float>` Horizontal FOV of the camera (def. 120)
