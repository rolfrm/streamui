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

The stream generates some lower level primitives based on the objects/classes used in the more high level stream

```
/window window  // window is a primitive
/window/height 512
/window/width 512
/window/btn button
/window/btn/rectangle rectangle //primitive
/window/btn/click click-area //primitive
/window/btn/content content-presenter //primitive?
/window/btn/text "Click me"
/window/btn/text text // primitive
/window/btn/text/text "Click me"
````

The style for button defines some style, but the style can also declare some things to be controlled by code. This is essential for the gui to interact with the underlying business code.

the default style for buttons might be defined like this:

```
/style/button button // a style is the instance of the thing
/style/button/rectangle rectangle
/style/button/rectangle/fill /colors/blue
style/button/rectangle/fill click click-area
```

the order of rendering is the same as the order of decleration. if something in the style is changed, the controls should be updated dynamically. e.g

```
/style/button/rectangle/fill /colors/red
/window/btn/rectangle/fill /colors/red
```

But only if the color is not already overwritten.

some basic compononts are needed:

- A: node->subnode mapping
- B: style->inheritor mapping
- C: node customization mapping.

Each node has an id. The node->subnode mapping is a multi-lookup table mapping each node to its sub nodes.

/           ID: 0
/a object   ID: 1
/a/b object ID: 2
/a/b/c object ID:3
/b /a/b       ID:4       // create a copy of /a/b.

// lets say c is the value five

/a/b/c color 255 255 255 255 //white

This is similar to doing
/a/b/c /colors/white


in this situation color is a function that can create a color value. In this case, there should be a color table, that maps ids to colors. Or something like that..

It should generally be avoided to generate new ids when old ones can be re-used. This will reduce the amount of data generated when a new object is created and make it easier to propagate updates. However, the changed events should still propagate. Example:

// initialization
/a object
/a/b 5
/c /a

- /c object
- /c/b 5

// change value
/a/b 6
- /c/b 6

// change value back
/a/b 5
- /c/b/5

b still has only one parent, a. /c/b is not a new object.

An object can have one value. For example, /a has the value 'a/b/', while /a/n has the value '5'. There is a table for storing numbers, a table for colors, etc. Each type has its own table where the lookup can be done.


