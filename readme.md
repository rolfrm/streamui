## Stream UI

a generic format for streaming uis.

Every change to the UI is an event. Everything in the GUI is built up from events.

the format could look like this:

```
/window window 512 512
/window/title "My First Window"
/window/grid grid
/window/grid/comment "This grid is for storing a button and some text"
/window/grid/btn button
/window/grid/btn/text "Click Me!"
/window/grid/txt "Click the button!"
/window/grid/txt/grid-row 1
/window/grid/bnt/grid-row 0
```

This is the code it takes to build a window with a grid with some elements.

