<h1 align="center">angelscript-docgen</h1>
<p align="center">
AngelScript Engine Interface Documentation Generator

![alt text](https://github.com/marcakafoddex/angelscript-docgen/blob/master/docs/screenshot.png?raw=true)
</p>

## Introduction

angelscript-docgen is a 3rdparty add on for <a href="http://www.angelcode.com/angelscript/">AngelScript</a>.

It allows you to generate the documentation for the script writers in your project. The documentation is generated as a self contained single HTML file, so it's easy to distribute. It's easy to use as it queries your AngelScript engine object for all available types and outputs it to HTML.

## AngelScript version

The add-on has been tested with AngelScript 2.34 only.

## Features

- Lists all registered classes and global functions automatically
- Lists all constructors and methods for all classes
- Syntax highlighting
  - Also highlights your custom types
- Visually configurable, colors, backgrounds, fonts etc. can all be tweaked if desired
- Can embed your company logo in the generated HTML file
- Has a search function built in, so script writers can quickly search for keywords
- Automatically differentiates between value object types and reference object types, and highlights them differently.

## Dependencies

- The generated HTML file depends on jQuery. This is used through a CDN, so there is no need to ship it with the generated file.

## License

This plugin is released under the MIT license. 

## How to use

- Copy the source/docgen.* files to any location in your project that you want
- Add the docgen.cpp file in your build. 
- Make sure the directory where docgen.h is located is in your include paths
- Add ```#include "docgen.h"``` to your code
- Setup a ```ScriptDocumentationOptions``` object to your liking (see the source for documentation)
- Setup a ```DocumentationGenerator``` object passing it your ```asIScriptEngine*``` pointer and the options object
- Register documentation by calling the proper methods on the ```DocumentationGenerator``` object (see example below)
- Call ```Generate()``` on the ```DocumentationGenerator``` object

## Example Usage

The following is an exerpt of a modified version of ```scriptmgr.cpp``` from the example game that comes with AngelScript. The basic idea is that you register a type, and use the result value (usually r in AngelScript example code) to register the documentation for the object type, behavior, method. Then when you are done you call generate on the documentation generator to actually create the output file.

```C++
/* ...regular includes... */

#include "../../../add_on/docgen/docgen.h"

int CScriptMgr::Init()
{
    int r;

    // setup the script engine
    engine = asCreateScriptEngine();

    // setup the documentation generator
    ScriptDocumentationOptions docgenOptions;
    docgenOptions.projectName = "Example Game";
    docgenOptions.includeArrayInterface = false;      // keeps the example clean
    docgenOptions.includeStringInterface = false;     // keeps the example clean
    docgenOptions.includeWeakRefInterface = false;    // keeps the example clean
    docgenOptions.htmlSafe = false;                                // we want to be able to style ourselves with tags, e.g. <b></b>
    DocumentationGenerator docGen(engine, docgenOptions);

    /* ... continued ... */

    // Register the game object. The scripts cannot create these directly, so there is no factory function.
    r = engine->RegisterObjectType("CGameObj", 0, asOBJ_REF); assert( r >= 0 );
    r = docGen.DocumentObjectType(r, "Represents a game object."); assert( r >= 0 );
    r = engine->RegisterObjectBehaviour("CGameObj", asBEHAVE_ADDREF, "void f()", asMETHOD(CGameObj, AddRef), asCALL_THISCALL); assert( r >= 0 );
    r = docGen.DocumentObjectMethod(r, "Constructs a empty game object."); assert( r >= 0 );

    /* ... continued ... */

    r = engine->RegisterObjectMethod("CGameObj", "int get_x() const", asMETHOD(CGameObj, GetX), asCALL_THISCALL); assert( r >= 0 );
    r = docGen.DocumentObjectMethod(r, "Returns current horizontal position for object in-game"); assert( r >= 0 );
    r = engine->RegisterObjectMethod("CGameObj", "int get_y() const", asMETHOD(CGameObj, GetY), asCALL_THISCALL); assert( r >= 0 );
    r = docGen.DocumentObjectMethod(r, "Returns current vertical position for object in-game"); assert( r >= 0 );
    r = engine->RegisterObjectMethod("CGameObj", "bool Move(int dx, int dy)", asMETHOD(CGameObj, Move), asCALL_THISCALL); assert( r >= 0 );
    r = docGen.DocumentObjectMethod(r, "Will attempt to move the object by the given amount, returns whether or not the move was legal."); assert( r >= 0 );

    /* ... continued ... */

    r = docGen.Generate(); assert(r >= 0);
    return 0;
}

```

## Removing documentation strings from the final build

By default, if nothing else is specified, the ```AS_GENERATE_DOCUMENTATION``` macro is defined as 1 in the docgen.h file. This will cause all documentation strings to be build into your executable or (shared) library, so that the documentation generator can be used. However, in the final build of your game you most likely do not want to include these strings. To disable them, define ```AS_GENERATE_DOCUMENTATION=0``` in your build system. This causes the header to declare the generate as a stub that ignores all incoming variables, so that the compiler will optimize everything away. This way your use of this add on doesn't need to be commented out or removed, everything is handled automatically through (proper) use of the ```AS_GENERATE_DOCUMENTATION``` macro.
If for some reason the compiler does not optimize the strings away, you can use the ```asDOC``` macro. This will turn strings into 0 when ```AS_GENERATE_DOCUMENTATION=0```, so that the compiler won't even see them anymore.
