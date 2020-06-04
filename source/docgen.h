#ifndef ANGELSCRIPT_DOCGEN_H
#define ANGELSCRIPT_DOCGEN_H

#ifndef ANGELSCRIPT_H 
// Avoid having to inform include path if header is already include before
#include <angelscript.h>
#endif

// If not otherwise defined beforehand, this define will be set to 1, so that documentation
// strings end up in the build executable/so/dll. 
// For a final release build, set this define to 0 in the build to remove all documentation 
// strings from the final executable/so/dll.
#ifndef AS_GENERATE_DOCUMENTATION
#define AS_GENERATE_DOCUMENTATION 1
#endif

BEGIN_AS_NAMESPACE

enum : int {
	// generate results
	asDOCGEN_DocumentationNotBuiltIn	= 0x7fffffff,
	asDOCGEN_Success					= 0,
	asDOCGEN_FailedToWriteFile			= -1001,
	asDOCGEN_InvalidInput				= -1002,
	asDOCGEN_CouldNotLoadLogoFile		= -1003,

	// register documentation results
	asDOCGEN_CouldNotFindFunctionById	= -1004,
	asDOCGEN_CouldNotFindTypeById		= -1005,
	asDOCGEN_AlreadyDocumented			= -1006,
};

struct ScriptDocumentationOptions {
	// output options
	const char*		outputFile = "documentation.html";					// local file where output will be written to
	bool			includeStringInterface = false;						// should - if registered - the string object type be included in the documentation?
	bool			includeArrayInterface = false;						// should - if registered - the array object type be included in the documentation?
	bool			includeRefInterface = false;						// should - if registered - the ref object type be included in the documentation?
	bool			includeWeakRefInterface = false;					// should - if registered - the weakref object type be included in the documentation?
	bool			addTimestamp = true;								// outputs date + time underneath summary 
	bool			syntaxHighlight = true;								// performs syntax highlighting on the function signatures
	bool			nl2br = true;										// if true, makes "\n" in documentation strings automatically convert to <br/>
	bool			htmlSafe = true;									// if true, makes "&, "<" and ">" in documentation strings automatically convert to "&amp;", "&lt;" and "&gt;" resp.
	bool			linkifyUrls = true;									// if true, turns urls automatically into <a href=""></a> texts.

	// interface options
	const char*		interfaceVersion = "1";								// the version of the interface, and thus the documentation, should be increased with every change to the engine interface
	const char*		projectName = "MyGame";								// name of your project, can include a separate version if required
	const char*		documentationName = "AngelScript Engine Interface";	// name of documentation shown in the headerbar, alongside the project name
	const char*		searchHotKey = "F4";								// the keycode to jump to search bar, can only be a single charcode, no ctrl/alt/shift or anything, see http://keycode.info/

	// visual options
	const char*		logoImageFile = nullptr;							// logo file shown in top left can point to any local image file that browsers support (png, jpg, etc.) will be embedded into the HTML
	unsigned int	logoImageSize[2] = { 0,0 };							// force size of logo to this, you can set either 0 for width, 1 for height, or both, if not 0 the value is used
	const char*		defaultFont = "Helvetica";							// CSS font spec to use for all but API names
	const char*		defaultFontSize = "12pt";							// CSS font size spec for default font
	const char*		apiFont = "Consolas, Courier New, Courier";			// CSS font spec to use on API names
	const char*		apiFontSize = "11pt";								// CSS font size spec for api font
	const char*		mainBackgroundColor = "#1e1e1e";					// CSS color spec for page background color
	const char*		mainForegroundColor = "#f1f1f1";					// CSS color spec for page text color
	const char*		headerBackgroundColor = "#2d2d30";					// CSS color spec for header background color
	const char*		headerForegroundColor = "#f1f1f1";					// CSS color spec for header text color
	const char*		headerBorder = "2px solid #007acc";					// CSS border spec for header bottom border
	const char*		textHeaderColor = "#007acc";						// CSS color spec for <h1>, <h2> etc headers 
	const char*		linkColor = "#dcdcaa";								// CSS color spec for <a href> links
	const char*		blockBackgroundColor = "#222222";					// CSS color spec for content block background color
	const char*		blockBorder = "1px solid #b1b1b1";					// CSS border spec for content blocks
	const char*		generatedDateTimeColor = "#666666";					// CSS color spec for generated date/time, if enabled
	const char*		asKeywordColor = "#007acc";							// CSS color spec for keywords in displayed AngelScript code
	const char*		asNumberColor = "#d8a0df";							// CSS color spec for numbers in displayed AngelScript code
	const char*		asValueTypeColor = "#ff8000";						// CSS color spec for value (class) types in displayed AngelScript code
	const char*		asClassTypeColor = "#4ec9b0";						// CSS color spec for non-value reference class types in displayed AngelScript code
	const char*		summaryWidth = "300px";								// CSS width spec for width of summary on the left
	
	// raw CSS/JS that will be appended after the main CSS and JS
	const char*		additionalCss = nullptr;							// additional CSS that will be added at the bottom of the style sheet, so can be used to override anything
	const char*		additionalJavascript = nullptr;						// additional JavaScript that will be added at the bottom of the document.ready handler, so can be used to override anything
};

#if AS_GENERATE_DOCUMENTATION

#define asDOC(text) text

/* When a build with documentation is requested, use the code as defined in the cpp file.
 * It will maintain a copy of the documentation strings in the generator class, and use
 * them when Generate() is called.
 */
class DocumentationGenerator {
public:
	DocumentationGenerator(asIScriptEngine* engine, const ScriptDocumentationOptions& options);
	~DocumentationGenerator();
	int DocumentGlobalFunction(int r, const char* string);
	int DocumentObjectType(int r, const char* string);
	int DocumentObjectMethod(int r, const char* string);
	int DocumentInterface(int r, const char* string);
	int DocumentInterfaceMethod(int r, const char* string);
	int Generate();
private:
	class Impl;
	Impl* impl;
};

#else

#define asDOC(text) nullptr

/* WHen a build without documentation is requested, use the inlined code below that will
 * ignore the documentation strings completely. In an optimized (release) build this means
 * that all the strings will be omitted.
 * If for some reason the documentation strings still end up in the final build, wrap all
 * strings in the asDOC macro, so that the compiler will never ever see them.
 */
class DocumentationGenerator {
public:
	inline DocumentationGenerator(asIScriptEngine* /*engine*/, const ScriptDocumentationOptions& /*options*/) {}
	inline int DocumentGlobalFunction(int r, const char* /*string*/) { return r; }
	inline int DocumentObjectType(int r, const char* /*string*/) { return r; }
	inline int DocumentObjectMethod(int r, const char* /*string*/) { return r; }
	inline int DocumentInterface(int r, const char* string) { return r; }
	inline int DocumentInterfaceMethod(int r, const char* string) { return r; }
	inline int Generate() { return asDOCGEN_DocumentationNotBuiltIn; }
};

#endif

END_AS_NAMESPACE

#endif
