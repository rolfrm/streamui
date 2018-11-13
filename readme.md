## TO DO
 - icy_eval/icy_parse_id2 has some bad behavior when generating temporary object trees, since parents might be used for data that is not available.
 - read only access to value objects. Testing for this behavior currently.
 - Consider if bytecode is a good approach for driving events. Consider how the datalog event should be propagated to the listeners
 - Consider that it should be possible to efficiently store the entire scope as a file and load it later.


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




## 1 Nov 2018
After trying to implement the system I have found some issues and some new ideas.

An object is an object.
so /a/b creates two objects a and b
/c /a/b creates one object c and assigns b to it. so that a/b and c/b is the same object.


### More abstractions

It could be more efficient with another level of abstraction that maps properties onto an object, so that the code behind the objects could use a more optimized datastructure. For example if there is an object /a and it has an /a/x. X could map to an index in an array. So if there is a list of particles
[x,y],[x,y],[x,y].

and a list of objects /v1,/v2,/v3. then they can be addressed individually as such:
/v1/x and /v1/y. Without having to allocate /v1/x, /v1/y, /v2/x, /v2/y. 

How could this be achieved? Simple!
Lets say they all exist in an array For example:
/array/0/x

/array is an array of points, thats the only real object in the system.
there is a special type for custom object types.

'/' (root) is a regular object, with normal lookup semantics
/array/ (array) is a special array type that overloads the indexer for '\0'

Anyway, every type can make an indexer that gives access to its child objects. I'll generalize this for the normal object case.


### More more abstractions

The abstractions thing works, but it brought along a new problem. Now there are virtual objects, so objects dont always have an ID anymore. Previously if there was an object of type Double, then a table could map from object id to doubles. And this generally works, although its a bit slow. However, with the new abstractions it is also possible to have an array of doubles and those values does not need to be stored in that table. I would still like the type to be the same. This gives an essential problem.

/array/5 512  // double array
/MyWindow/Width /array/5

This kind of copy operations must be possible, but they have to be complicated in the new version.

Maybe the problem is just that /MyWindow/Width is an object that holds a value, but thats different from being a value by one level of indirection. So the point is to store that indirection instead of storing the value.

/MyWindow/Width is an object with reference to some data
/array/5 is an index with a reference to a value

The parse method for /array/5 uses the parser for 'ArrayIndex' which only looks at type, while the parse method for /MyWindow/Width uses the parser for any object.

The value is always a virtual object. A virtual can ask its parent for space for storage.


### Bytecode format.

To efficiently represent the events in a binary format, its better to have a bytecode format than the string based format.

Everything inside '//' represent a scope. Each scope has a unique ID within its parent. And the parent scope decides on its own, depending on type how that is implemented. In the initial version, every scope has 4 bytes of storage and ends with a '0'

The final scope is callable with a set of arguments, for example

/MyWindow/Width 512

if the type of '/MyWindow/Width' is not known, it must make a guess. It does this by running through all the known parsers.
It is possible to let the type of objects be known by setting
/MyWindow/Width/value_type /types/int

Once the type of the argument is known the type with be written into the bytecode stream and the type will encode the value into the bytecode stream.

So /MyWindow/Width 512 would turn into
[[Id of MyWindow][Id of Width inside MyWindow]0[type of double][8 bytes of double value]
So this one line is encoded into
4 + 4 + 4 + 4 + 8 = 24 bytes of data.

The first part is also generally useful as a method to look up elements from the byte code.

The bytecode format has a flaw in the sense that refering to the name of a thing can be done at evaluation time, whereas the ids themselves might change.

This could be a problem when an object changes. For example:
/MyWindow/Width 512   // Object #1/#2: 512
del /MyWindow/Width   // delete #2
/MyWindow/Width 256   // Object #1/#3: 256
/MyWindow/Width 512   // Object #1/#3: 512

If the bytecode '#1/#2: 512' is stored and later executed, an error will occur because #2 no longer exists. However, one could question why that would be needed.

### Object copies

It is up to the object itself to handle references to other objects. 

An object can be created using another object as a basis:
/MyWindowCopy /Default/Window

This creates a new object that is linked to the base. If any changes happens on the base object those will propagate to the copy. If any changes happens on the copy specifically, they will override changes happening on the base object.

This behavior can be changed by specifying the value handler type  to be different:
/MyWindowCopy/type /type/NodeReference
/MyWindowCopy /Default/Window

In this case since '/type' was specified, the behavior can change.


### Callable nodes

cd is a callable node. cd is defined in the global scope.

'cd /home/..'.

Likewise '..' and '.' could be things defined in the global scope, so that
cd './...'. actually makes sense without anything having to be hardcoded.

### Actions on the bytecode stream
Since everything that can be executed has a bytecode representation, the names of nodes also has to be registered here.
So when something is evaluated, strings are being interned and the internation has to be executed before the action itself.

