# Wasted 🍺

Drinking simulator, written in modern OpenGL.

![animated gif of the game](/res/preview.gif)

This was a final project for the computer graphics & visualization course at my university.
It came out pretty well, so I'm publishing it in a dedicated repo.

The general goal was to correctly load models and textures, then draw and shade the scene in the OpenGL core profile.
Also, for this project a first person camera, movement, collision detection and object picking had to be implemented.
As you get more drunk, the camera starts to wobble, pulsate, and movement becomes harder.
Scenes are editable and persistent, so you can move objects around and save the changes to a JSON file.

The things that didn't make the cut are post-processing effects (bloom, hdr, motion blur), text overlays with true-type fonts, debug console, a crosshair, "real" collision detection and physics.

The `engine.cc` and `engine.hh` have nice functions for resource loading, which may be useful for future OpenGL adventures.
The code is quite terse, and written in ~~pretty~~ acceptably modern C++, so you're free to reuse it, if you wish.

Overall, it was a great learning experience, and one of the most fun university projects,
especially considering we've never used OpenGL, or played around with 3D graphics before.

## Controls

- Mouse: look around
- W/A/S/D: movement
- Left click: pick up/put down
- Right click: drink up
- M: toggle mode 
- ESC: deactivate game window

Additionally, in edit mode:

- Mouse wheel: scale the held object up/down
- SPACE/LSHIFT: fly up/down
- R: reload shaders
- O: print object coordinates
- J: save scene to JSON

## How to run

These libraries must be installed on your system: GLFW, GLEW, GLM, Assimp, DevIL

Run this command in the root of the project:

```
make run
```

## Credits

### Authors

- Max Adamski
- Anita Zielińska

### Attribution

- [Bar] and other models by Google / [CC BY]
- [Beer Flight] model by Bruno Oliveira / [CC BY]
- [Lantern] model by Karl Channell / [CC BY]
- [Gramophone] model by Don Carson / [CC BY]

[Lantern]: https://poly.google.com/view/9Ug3Sq6PCR3
[Gramophone]: https://poly.google.com/view/9MZ0sCt1REv
[Beer Flight]: https://poly.google.com/view/bK5lwl7gvA7
[Bar]: https://poly.google.com/view/cEbi0acnY23
[CC BY]: https://creativecommons.org/licenses/by/2.0
