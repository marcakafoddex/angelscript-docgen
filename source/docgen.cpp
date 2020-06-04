// C++
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <functional>
#include <algorithm>
#include <regex>
#include <vector>
#include <set>
#include <iomanip>
#include <string>

// C
#include <cstdarg>
#include <cstring>
#include <cassert>

// local
#include "docgen.h"

#if AS_GENERATE_DOCUMENTATION

namespace {

	const char* const HtmlStart = R"^(
<!doctype html>
<html>
<head>
	<title>%%PROJECT_NAME%%</title>
	<script type="text/javascript" src="https://code.jquery.com/jquery-3.5.1.min.js"></script>
	<style>
		* {
			box-sizing: border-box;
		}
		body {
			font-family: %%DEFAULT_FONT%%;
			font-size: %%DEFAULT_FONT_SIZE%%;
			background-color: %%MAIN_BACKGROUND_COLOR%%;
			color: %%MAIN_FOREGROUND_COLOR%%;
			margin: 0;
			padding: 0;
			display: grid;
			grid-template-columns: %%SUMMARY_WIDTH%% 1fr;
			grid-template-rows: auto 1fr;
			height: 100vh;
			overflow: hidden;
		}
		#header {
			font-size: 200%;
			font-weight: bold;
			background-color: %%HEADER_BACKGROUND_COLOR%%;
			color: %%HEADER_FOREGROUND_COLOR%%;
			border-bottom: %%HEADER_BORDER%%;
			padding: 20px;
			display: grid;
			grid-template-columns: auto 1fr;
			grid-area: 1/1/span 1/span 2;
		}
		#header #namecontainer {
			display: grid;
			grid-template-columns: auto 1fr;
			align-items: center;
		}
		#header #namecontainer img {
			padding-right: 10px;
		}
		#header #searchbox {
			align-self: center;
		}
		#header #searchbox > input {
			float: right;
			width: 400px;
		}
		#summary {
			border-right: %%HEADER_BORDER%%;
			padding: 20px;
			overflow: auto;
			grid-area: 2/1/span 1/span 1;
		}
		#content {
			padding: 20px;
			overflow: auto;
			grid-area: 2/2/span 1/span 1;
		}
		h1 {
			font-size: 150%;
			font-weight: bold;
			color: %%SUBHEADER_COLOR%%;
			margin: 0 0 5px 0;
		}
		h2 {
			font-size: 120%;
			text-decoration: underline;
			color: %%SUBHEADER_COLOR%%;
		}
		a, a:hover, a:visited {
			color: %%LINK_COLOR%%;
		}
		.api {
			font-family: %%API_FONT%%;
			font-size: %%API_FONT_SIZE%%;
			white-space: pre;
			tab-size: 20px;
		}
		.AS-keyword {
			font-weight: bold;
			color: %%AS_KEYWORD_COLOR%%;
		}
		.AS-number {
			font-weight: bold;
			color: %%AS_NUMBER_COLOR%%;
		}
		.AS-valuetype {
			font-weight: bold;
			color: %%AS_VALUETYPE_COLOR%%;
		}
		.AS-classtype {
			font-weight: bold;
			color: %%AS_CLASSTYPE_COLOR%%;
		}
		.block {
			margin: 0px 10px 10px 20px;
			padding: 10px;
			border: %%BLOCK_BORDER%%;
			background-color: %%BLOCK_BACKGROUND_COLOR%%;
		}
		.documentation {
			width: 500px;
			padding: 10px 40px;
			font-style: italic;
		}
		.indented {
			padding-left: 20px;
		}
		.hidden {
			display: none;
		}
		.link {
			cursor: pointer;
			color: %%LINK_COLOR%%;
		}
		.link:hover {
			text-decoration: underline;
			color: %%LINK_COLOR%%;
		}
		.generated_on {
			padding-top: 10px;
			font-size: 80%;
			color: %%GENERATED_ON_COLOR%%;
		}

		%%ADDITIONAL_CSS%%
	</style>
	<script type="text/javascript">
		$(document).ready(() => {
			// handle search shortcut
			$(document).keydown((ev) => {
				if (ev.key == '%%SEARCH_HOTKEY%%' || ev.code == '%%SEARCH_HOTKEY%%')
					$('#searchbox input').focus();
			});

			// handle search, need to set same handler to multiple events, so wrap in 
			// anonymous function so we don't litter the global namespace
			(() => {
				
				let handler = (ev) => {
					const 
						v = ev.target.value.toLowerCase(),
						m = '[name' + (v.length ? '*=' + v : '') + ']',
						s = '#content '+m, 
						h = '#content [name]:not('+m+')';
					$(h).hide();
					$(s).show().parents('[name]').show();
				};
				$('#searchbox input').keyup(handler);
				$('#searchbox input').change(handler);
			})();

			// handle collapsing/expanding the summary
			$('#summary div.expandable').click((ev)=>{
				let s = $(ev.target).parents('div.expandable').first(),
					t = s.siblings('div.indented');
				t.toggleClass('hidden');
				s.find('span').first().html(t.hasClass('hidden') ? '+' : '-');
			});

			// handle anchor jumps
			$('span[targetname]').click((ev)=>{
				let targetName = $(ev.target).attr('targetname'),
					target = $('a[name=' + targetName + ']').first();
				$('#content').animate({
					scrollTop: $('#content').scrollTop() + target.position().top - $('#header').innerHeight() - 5,
				}, 250);
			});

			// any additional javascript
			%%ADDITIONAL_JAVASCRIPT%%
		});
	</script>
</head>
<body>
	<div id="header">
		<div id="namecontainer">
			%%LOGO_IMAGE%%
			<span>%%PROJECT_NAME%% - %%DOCUMENTATION_NAME%% v%%INTERFACE_VERSION%%</span>
		</div>
		<div id="searchbox"><input type="text" class="api" placeholder="Search the documentation, hit %%SEARCH_HOTKEY%% to focus..."></div>
	</div>
)^";

	const char* const HtmlEnd = R"^(
</body>
</html>
)^";

	class DocumentationOutput {
	public:
		DocumentationOutput();
		void setVariable(const char* variable, const char* value);
		void setVariable(std::string variable, std::string value);
		void append(const char* text);
		void appendF(const char* format, ...);
		void appendRaw(const char* text);
		void appendRawF(const char* format, ...);
		int writeToFile(const char* outputFile);
	private:
		std::string							output;
		std::map<std::string, std::string>	m_variables;
		int									m_state = 0;
		std::string							m_formatBuffer;
	};

	DocumentationOutput::DocumentationOutput() 
		: m_formatBuffer(65536, '\0')
	{
	}

	void DocumentationOutput::setVariable(const char* variable, const char* value) {
		m_variables.emplace(variable,
							value ? std::string(value) : std::string{});
	}

	void DocumentationOutput::setVariable(std::string variable, std::string value) {
		m_variables.emplace(std::move(variable),
							std::move(value));
	}

	void DocumentationOutput::append(const char* text) {
		std::string varName;
		while (const char* param = strstr(text, "%%")) {
			const char* closeParam = strstr(param + 2, "%%");
			assert(closeParam);
			output.append(text, param);
			varName.assign(param + 2, closeParam);
			auto it = m_variables.find(varName);
			assert(it != m_variables.end());
			output.append(it->second);
			text = closeParam + 2;
		}
		output.append(text);
	}

	void DocumentationOutput::appendF(const char* format, ...) {
		va_list args;
		va_start(args, format);
		vsnprintf(&m_formatBuffer[0], m_formatBuffer.size(), format, args);
		va_end(args);
		append(m_formatBuffer.data());
	}

	void DocumentationOutput::appendRaw(const char* text) {
		output += text;
	}

	void DocumentationOutput::appendRawF(const char* format, ...) {
		va_list args;
		va_start(args, format);
		vsnprintf(&m_formatBuffer[0], m_formatBuffer.size(), format, args);
		va_end(args);
		appendRaw(m_formatBuffer.data());
	}

	int DocumentationOutput::writeToFile(const char* outputFile) {
		if (m_state)
			return m_state;
		try {
			std::ofstream file;
			file.open(outputFile, std::ios_base::binary);
			file << output;
			file.close();
			return asDOCGEN_Success;
		} catch (const std::exception&) {
			return asDOCGEN_FailedToWriteFile;
		}
	}

	// --------------------------------------------------------------------------------------------------------------------------------

	class TextDecorator {
	public:
		TextDecorator(bool syntaxHighlight, bool nl2br, bool htmlSafe, bool linkifyUrls);
		void appendObjectType(const asITypeInfo* typeInfo);

		std::string decorateAngelScript(std::string text);
		std::string decorateDocumentation(std::string text);
	private:
		std::string nl2br(std::string text);
		std::string htmlSafe(std::string text);
		void regexReplace(std::string& str, const std::regex& regex, const std::function<size_t(std::string& str, const std::cmatch&)>& cb);
		size_t wrapSpan(std::string& str, size_t strIndex, const char* text, size_t length, const char* cssClass);
		
		using KeywordMap = std::map<std::string, std::string>;
		KeywordMap		m_asKeywords;
		std::regex		m_identifierRegex;
		std::regex		m_urlRegex;
		std::regex		m_numberRegex;
		bool			m_syntaxHighlight;
		bool			m_nl2br;
		bool			m_htmlSafe;
		bool			m_linkifyUrls;
	};

	TextDecorator::TextDecorator(bool syntaxHighlight, bool nl2br, bool htmlSafe, bool linkifyUrls)
		: m_syntaxHighlight(syntaxHighlight)
		, m_nl2br(nl2br)
		, m_htmlSafe(htmlSafe)
		, m_linkifyUrls(linkifyUrls)
	{
		if (m_syntaxHighlight) {
			const char* const keywords[] = {
				// official keywords
				"and", "abstract", "auto", "bool", "break", "case", "cast", "catch", "class", "const", "continue", "default", "do", "double", "else", "enum", "explicit", "external ", 
				"false", "final", "float", "for", "from", "funcdef", "function", "get", "if", "import", "in", "inout", "int", "interface", "int8", "int16", "int32", "int64", "is", 
				"mixin", "namespace", "not", "null", "or", "out", "override ", "private", "property ", "protected", "return", "set ", "shared ", "super ", "switch", "this ", "true", 
				"try", "typedef", "uint", "uint8", "uint16", "uint32", "uint64", "void", "while", "xor",

				// not official keywords, but should probably be treated as such
				"string",
				"array",
			};
			for (const char* keyword : keywords)
				m_asKeywords[keyword] = "AS-keyword";
			m_identifierRegex = "(^|\\b)(\\w+)(\\b|$)";
			m_urlRegex = "([a-z]+\\://(.*?))(\\s|$)";
			m_numberRegex = "(^|\\b)[0-9]x?[0-9a-fA-F]*(\\b|$)";		// this is not very precise, but probably good enough
		}
	}

	void TextDecorator::appendObjectType(const asITypeInfo* typeInfo) {
		if (m_syntaxHighlight)
			m_asKeywords[typeInfo->GetName()] = typeInfo->GetFlags() & asOBJ_VALUE ? "AS-valuetype" : "AS-classtype";
	}

	std::string TextDecorator::decorateAngelScript(std::string text) {
		text = htmlSafe(text);
		if (m_syntaxHighlight) {
			// do identifiers
			std::string match;
			regexReplace(text, m_identifierRegex, [this, &match](std::string& text, const std::cmatch& matches) {
				const size_t offset = matches[0].first - text.c_str();
				match.assign(matches[0].first, matches[0].first + matches[0].length());
				auto it = m_asKeywords.find(match);
				if (it != m_asKeywords.end())
					return wrapSpan(text, offset, match.c_str(), match.size(), it->second.c_str());
				return offset + matches[0].length();
			});

			// do numbers
			regexReplace(text, m_numberRegex, [this](std::string& text, const std::cmatch& matches) {
				const size_t offset = matches[0].first - text.c_str();
				return wrapSpan(text, offset, matches[0].first, matches[0].length(), "AS-number");
			});
		}
		return std::move(text);
	}
	
	std::string TextDecorator::decorateDocumentation(std::string text) {
		text = htmlSafe(text);
		if (m_linkifyUrls) {
			// do urls
			std::string replacement;
			regexReplace(text, m_urlRegex, [&replacement](std::string& text, const std::cmatch& matches) {
				replacement = "<a href=\"";
				replacement.append(matches[0].first, matches[0].first + matches[0].length());
				replacement += "\">";
				replacement.append(matches[0].first, matches[0].first + matches[0].length());
				replacement += "</a>";
				const size_t offset = matches[0].first - text.c_str();
				text.replace(offset, (size_t)matches[0].length(), replacement.c_str());
				return offset + replacement.size();
			});
		}
		return nl2br(std::move(text));
	}

	size_t TextDecorator::wrapSpan(std::string& str, size_t strIndex, const char* text, size_t length, const char* cssClass) {
		std::string replacement;
		replacement = "<span class=\"";
		replacement += cssClass;
		replacement += "\">";
		replacement.append(text, length);
		replacement += "</span>";
		str.replace(strIndex, length, replacement);
		return strIndex + replacement.size();
	}

	void TextDecorator::regexReplace(std::string& str, const std::regex& regex, const std::function<size_t(std::string&str, const std::cmatch&)>& cb) {
		std::cmatch matches;
		size_t searchOffset = 0;
		while (std::regex_search(str.c_str() + searchOffset, matches, regex))
			searchOffset = cb(str, matches);
	}

	std::string TextDecorator::nl2br(std::string text) {
		if (m_nl2br) {
			size_t searchOffset = 0;
			while (const char* nl = strstr(text.c_str() + searchOffset, "\n")) {
				const size_t replaceOffset = (size_t)(nl - text.c_str());
				text.replace(replaceOffset, 1, "<br/>\n");
				searchOffset = replaceOffset + 6;
			}
		}
		return text;
	}

	std::string TextDecorator::htmlSafe(std::string text) {
		if (m_htmlSafe) {
			size_t searchOffset = 0;
			while (true) {
				const size_t index = text.find_first_of("&<>", searchOffset);
				if (index == std::string::npos)
					break;
				const char* replacement = nullptr;
				switch (text[index]) {
				case '&': replacement = "&amp;"; break;
				case '<': replacement = "&lt;"; break;
				case '>': replacement = "&gt;"; break;
				default: assert(!"Should not happen");
				}
				text.replace(index, 1, replacement);
				searchOffset = index + strlen(replacement) - 1;
			}
		}
		return text;
	}

	// --------------------------------------------------------------------------------------------------------------------------------

	class Base64FileTool {
	public:
		Base64FileTool(const char* filename);
		std::string base64encoded();
	private:
		std::string m_rawData;
	};

	Base64FileTool::Base64FileTool(const char* filename) {
		std::ifstream file;
		file.open(filename, std::ios::binary | std::ios::ate);
		m_rawData.resize(static_cast<size_t>(file.tellg()));
		file.seekg(0).read(&m_rawData[0], m_rawData.size());
		file.close();
	}

	std::string Base64FileTool::base64encoded() {
		// C++ port from top answer here: https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
		static const char encoding_table[] = 
			{	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
				'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
				'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
				'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
				'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
				'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
				'w', 'x', 'y', 'z', '0', '1', '2', '3',
				'4', '5', '6', '7', '8', '9', '+', '/' };
		static const int mod_table[] = { 0, 2, 1 };

		const size_t input_length = m_rawData.size();
		const size_t output_length = 4 * ((input_length + 2) / 3);
		std::string output(output_length, '\0');
		char* encoded_data = &output[0];

		for (size_t i = 0, j = 0; i < input_length;) {

			const uint32_t octet_a = i < input_length ? (unsigned char)m_rawData[i++] : 0;
			const uint32_t octet_b = i < input_length ? (unsigned char)m_rawData[i++] : 0;
			const uint32_t octet_c = i < input_length ? (unsigned char)m_rawData[i++] : 0;
			const uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
			encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
			encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
			encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
			encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
		}
			
		for (int i = 0; i < mod_table[input_length % 3]; i++)
			encoded_data[output_length - 1 - i] = '=';

		return output;
	}

}

BEGIN_AS_NAMESPACE

class DocumentationGenerator::Impl {
public:
	Impl(asIScriptEngine* engine, const ScriptDocumentationOptions& options);
	int DocumentFunction(int funcId, const char* string);
	int DocumentType(int typeId, const char* string);
	int generate();

private:
	void GenerateSubHeader(int level, const char* title, const char* name, const std::function<void(void)>& cb);
	void GenerateContentBlock(const char* title, const char* name, const std::function<void(void)>& cb);
	void GenerateClasses();
	void GenerateGlobalFunctions();
private:
	struct SummaryNode;
	using SummaryNodeVector = std::vector<SummaryNode>;
	struct SummaryNode {
		std::string text;
		std::string anchor;
		SummaryNodeVector children;
	};

	struct ObjectTypeComparator {
		bool operator()(const asITypeInfo* a, const asITypeInfo* b) const {
			int i = strcmp(a->GetName(), b->GetName());
			if (i < 0)
				return true;
			if (i > 0)
				return false;
			return a < b;
		}
	};
	using ObjectTypeSet = std::set<const asITypeInfo*, ObjectTypeComparator>;

	SummaryNodeVector CreateSummary();
	void OutputSummary(const SummaryNodeVector& nodes);
	const char* LowerCaseTempBuf(const char* text);
	int Prepare();
	const char* GetDocumentationForType(const asITypeInfo* type) const;
	const char* GetDocumentationForFunction(const asIScriptFunction* function) const;

	// our input
	asIScriptEngine*						engine;
	ScriptDocumentationOptions				options;

	// documentation strings by pointer
	std::map<const asIScriptFunction*, std::string>	functionDocumentation;
	std::map<const asITypeInfo*, std::string>		objectTypeDocumentation;

	// our helper objects
	DocumentationOutput						output;
	TextDecorator							decorator;
	std::string								lowercase;

	// our state
	std::vector<const asIScriptFunction*>	globalFunctions;
	ObjectTypeSet							objectTypes;
};

DocumentationGenerator::Impl::Impl(asIScriptEngine* engine, const ScriptDocumentationOptions& options)
	: engine(engine)
	, options(options)
	, decorator(options.syntaxHighlight, options.nl2br, options.htmlSafe, options.linkifyUrls)
{
}

int DocumentationGenerator::Impl::Prepare() {
	// load logo file if required
	std::string logoImageTag;
	if (options.logoImageFile) {
		try {
			// this may throw, it uses std file stream
			Base64FileTool filetool(options.logoImageFile);
			std::string base64Img = filetool.base64encoded();

			// now build img tag
			logoImageTag = "<img ";
			const char* const names[] = {"width", "height"};
			for (int i = 0; i < 2; ++i) {
				if (options.logoImageSize[0]) {
					std::stringstream html;
					html << names[i] << "=\"" << options.logoImageSize[0] << "\"";
					logoImageTag += html.str();
				}
			}
			logoImageTag += R"^(src="data:image/png;base64,)^";
			logoImageTag += base64Img;
			logoImageTag += R"(" />)";
		} catch (...) {
			return asDOCGEN_CouldNotLoadLogoFile;
		}
	}

	// set variables
	output.setVariable("PROJECT_NAME", options.projectName);
	output.setVariable("DOCUMENTATION_NAME", options.documentationName);
	output.setVariable("INTERFACE_VERSION", options.interfaceVersion);
	output.setVariable("DEFAULT_FONT", options.defaultFont);
	output.setVariable("DEFAULT_FONT_SIZE", options.defaultFontSize);
	output.setVariable("API_FONT", options.apiFont);
	output.setVariable("API_FONT_SIZE", options.apiFontSize);
	output.setVariable("MAIN_BACKGROUND_COLOR", options.mainBackgroundColor);
	output.setVariable("MAIN_FOREGROUND_COLOR", options.mainForegroundColor);
	output.setVariable("HEADER_BACKGROUND_COLOR", options.headerBackgroundColor);
	output.setVariable("HEADER_FOREGROUND_COLOR", options.headerForegroundColor);
	output.setVariable("HEADER_BORDER", options.headerBorder);
	output.setVariable("SUBHEADER_COLOR", options.textHeaderColor);
	output.setVariable("BLOCK_BORDER", options.blockBorder);
	output.setVariable("BLOCK_BACKGROUND_COLOR", options.blockBackgroundColor);
	output.setVariable("LINK_COLOR", options.linkColor);
	output.setVariable("GENERATED_ON_COLOR", options.generatedDateTimeColor);
	output.setVariable("AS_KEYWORD_COLOR", options.asKeywordColor);
	output.setVariable("AS_NUMBER_COLOR", options.asNumberColor);
	output.setVariable("AS_VALUETYPE_COLOR", options.asValueTypeColor);
	output.setVariable("AS_CLASSTYPE_COLOR", options.asClassTypeColor);
	output.setVariable("ADDITIONAL_CSS", options.additionalCss);
	output.setVariable("ADDITIONAL_JAVASCRIPT", options.additionalJavascript);
	output.setVariable("SEARCH_HOTKEY", options.searchHotKey);
	output.setVariable("SUMMARY_WIDTH", options.summaryWidth);
	output.setVariable("LOGO_IMAGE", std::move(logoImageTag));

	// get and sort all global functions
	const int funcCount = engine->GetGlobalFunctionCount();
	if (funcCount) {
		globalFunctions.reserve(funcCount);
		for (int i = 0; i < funcCount; ++i)
			globalFunctions.push_back(engine->GetGlobalFunctionByIndex(i));
		std::sort(globalFunctions.begin(), globalFunctions.end(), [](const asIScriptFunction* a, const asIScriptFunction* b) {
			return strcmp(a->GetName(), b->GetName()) < 0;
		});
	}

	// get and sort all object types
	for (int t = 0, tcount = engine->GetObjectTypeCount(); t < tcount; ++t) {
		const asITypeInfo* const typeInfo = engine->GetObjectTypeByIndex(t);
		if (!strcmp(typeInfo->GetName(), "array") && !options.includeArrayInterface)
			continue;
		if (!strcmp(typeInfo->GetName(), "string") && !options.includeStringInterface)
			continue;
		if (!strcmp(typeInfo->GetName(), "ref") && !options.includeRefInterface)
			continue;
		if ((!strcmp(typeInfo->GetName(), "weakref") || !strcmp(typeInfo->GetName(), "const_weakref")) && !options.includeWeakRefInterface)
			continue;
		objectTypes.insert(typeInfo);
	}

	// append all types to the decorator
	for (const auto type : objectTypes)
		decorator.appendObjectType(type);
	return asDOCGEN_Success;
}

int DocumentationGenerator::Impl::generate() {
	int r = Prepare();
	if (r != asDOCGEN_Success)
		return r;

	// start HTML
	output.append(HtmlStart);

	// output summary
	output.appendRaw(R"^(<div id="summary">)^");
	OutputSummary(CreateSummary());
	if (options.addTimestamp) {
		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);
		std::stringstream dt;
		dt << std::put_time(&tm, "%d-%m-%Y %H:%M:%S");
		output.appendRawF(R"^(<div class="generated_on">Generated on %s</div>)^", dt.str().c_str());
	}
	output.appendRaw(R"^(</div>)^");

	// output content
	output.appendRaw(R"^(<div id="content">)^");
	GenerateClasses();
	GenerateGlobalFunctions();
	output.appendRaw(R"^(</div>)^");

	// finish HTML and output
	output.append(HtmlEnd);
	return output.writeToFile(options.outputFile);
}

void DocumentationGenerator::Impl::GenerateSubHeader(int level, const char* title, const char* name, const std::function<void(void)>& cb) {
	if (name) {
		output.appendRawF(R"^(<a name="%s"></a>)^", LowerCaseTempBuf(name));
		output.appendRawF(R"^(<h%d name="%s">%s</h%d>)^" "\n", level, LowerCaseTempBuf(title), title, level);
		output.appendRawF(R"^(<div name="%s">)^", LowerCaseTempBuf(name));
	} else {
		output.appendRawF(R"^(<h%d>%s</h%d>)^" "\n", level, title, level);
		output.appendRaw(R"^(<div>)^");
	}
	cb();
	output.appendRaw("</div>");
}

void DocumentationGenerator::Impl::GenerateContentBlock(const char* title, const char* name, const std::function<void(void)>& cb) {
	output.appendRawF(R"^(<a name="%s"></a>)^", LowerCaseTempBuf(name));
	output.appendRawF(R"^(<div class="block" name="%s">)^" "\n", LowerCaseTempBuf(title));
	cb();
	output.appendRaw("</div>\n");
}

void DocumentationGenerator::Impl::GenerateGlobalFunctions() {
	// bail if nothing to do
	if (globalFunctions.empty())
		return;

	// create output
	GenerateSubHeader(1, "Global Functions", "___globalfunctions", [&]() {
		for (auto func : globalFunctions)
			GenerateContentBlock(func->GetName(), func->GetName(), [&]() {
			output.appendRawF(R"^(<div class="api">%s</div>)^", decorator.decorateAngelScript(func->GetDeclaration(true, true, true)).c_str());
			const char* documentation = GetDocumentationForFunction(func);
			if (documentation && *documentation)
				output.appendRawF(R"^(<div class="documentation">%s</div>)^", decorator.decorateDocumentation(documentation).c_str());
		});
	});
}

DocumentationGenerator::Impl::SummaryNodeVector DocumentationGenerator::Impl::CreateSummary() {
	SummaryNodeVector functionNodes;
	std::set<std::string> seenFunctions;
	for (auto func : globalFunctions) {
		if (seenFunctions.insert(func->GetName()).second)
			functionNodes.emplace_back(SummaryNode{
				func->GetName(),
				LowerCaseTempBuf(func->GetName()),
			});
	}

	SummaryNodeVector valueTypes, classes;
	for (auto typeInfo : objectTypes) {
		SummaryNodeVector& v = (typeInfo->GetFlags() & asOBJ_VALUE) ? valueTypes : classes;
		v.emplace_back(SummaryNode{
			typeInfo->GetName(),
			LowerCaseTempBuf(typeInfo->GetName()),
		});
	}

	return {
		{
			"Value Types",
			"___valuetypes",
			std::move(valueTypes),
		},			{
			"Classes",
			"___classes",
			std::move(classes),
		},
		{
			"Global Functions",
			"___globalfunctions",
			std::move(functionNodes)
		},
	};
}

void DocumentationGenerator::Impl::OutputSummary(const SummaryNodeVector& nodes) {
	for (const auto& node : nodes) {
		output.appendRaw(R"^(<div class="summary">)^");
		if (node.children.empty()) {
			output.appendRawF(R"^(<span class="api link" targetname="%s">%s</span>)^", node.anchor.c_str(), node.text.c_str());
		} else {
			output.appendRawF(R"^(<div class="api expandable link" targetname="%s"><span>+</span> <span>%s</span></div>)^", node.anchor.c_str(), node.text.c_str());
			output.appendRawF(R"^(<div class="indented hidden">)^");
			OutputSummary(node.children);
			output.appendRawF("</div>");
		}
		output.appendRaw("</div>");
	}
}

const char* DocumentationGenerator::Impl::LowerCaseTempBuf(const char* text) {
	lowercase = text;
	for (auto& c : lowercase)
		c = (char)std::tolower(c);
	return lowercase.c_str();
}

void DocumentationGenerator::Impl::GenerateClasses() {
	const asEBehaviours behaviors[] = { asBEHAVE_CONSTRUCT, asBEHAVE_FACTORY, asBEHAVE_LIST_FACTORY, asBEHAVE_DESTRUCT };
	std::vector<const asIScriptFunction*> funcs;

	auto generate = [&](bool valueTypes) {
		GenerateSubHeader(1, valueTypes ? "Value Types" : "Classes", valueTypes ? "___valuetypes" : "___classes", [&]() {
			for (const asITypeInfo* typeInfo : objectTypes) {
				if (!!(typeInfo->GetFlags() & asOBJ_VALUE) != valueTypes)
					continue;

				GenerateSubHeader(2, typeInfo->GetName(), typeInfo->GetName(), [&]() {
					// list overview (fake class)
					std::string classOverview = std::string("class ") + typeInfo->GetNamespace() + typeInfo->GetName() + " {\n";
					funcs.clear();
					for (auto beh : behaviors)
						for (int i = 0, c = typeInfo->GetBehaviourCount(); i < c; ++i) {
							asEBehaviours b;
							auto behFunc = typeInfo->GetBehaviourByIndex(i, &b);
							if (beh == b) {
								classOverview += std::string("\t") + behFunc->GetDeclaration(false, false, true) + ";\n";
								funcs.push_back(behFunc);
							}
						}
					for (int i = 0, c = typeInfo->GetMethodCount(); i < c; ++i) {
						auto func = typeInfo->GetMethodByIndex(i);
						funcs.push_back(func);
						classOverview += std::string("\t") + func->GetDeclaration(false, false, true) + ";\n";
					}
					for (int i = 0, c = typeInfo->GetPropertyCount(); i < c; ++i)
						classOverview += std::string("\t") + typeInfo->GetPropertyDeclaration(i, true) + ";\n";
					classOverview += "}";
					GenerateContentBlock(typeInfo->GetName(), typeInfo->GetName(), [&]() {
						output.appendRawF(R"^(<div class="api">%s</div>)^", decorator.decorateAngelScript(classOverview).c_str());
						const char* documentation = GetDocumentationForType(typeInfo);
						if (documentation && *documentation)
							output.appendRawF(R"^(<div class="documentation">%s</div>)^", decorator.decorateDocumentation(documentation).c_str());
					});

					// list behaviors + methods
					for (auto func : funcs) {
						std::string fullName = (std::string(typeInfo->GetName()) + "::" + func->GetName());
						GenerateContentBlock(fullName.c_str(), fullName.c_str(), [&]() {
							output.appendRawF(R"^(<div class="api">%s</div>)^", decorator.decorateAngelScript(func->GetDeclaration(true, false, true)).c_str());
							const char* documentation = GetDocumentationForFunction(func);
							if (documentation && *documentation)
								output.appendRawF(R"^(<div class="documentation">%s</div>)^", decorator.decorateDocumentation(documentation).c_str());
						});
					}

					// list classes
				});
			}
		});
	};
	generate(true);
	generate(false);
}

int DocumentationGenerator::Impl::DocumentFunction(int funcId, const char* string) {
	if (funcId >= 0) {
		const asIScriptFunction* function = engine->GetFunctionById(funcId);
		if (!function)
			return asDOCGEN_CouldNotFindFunctionById;
		if (!functionDocumentation.insert({function, {string}}).second)
			return asDOCGEN_AlreadyDocumented;
	}
	return funcId;
}

int DocumentationGenerator::Impl::DocumentType(int typeId, const char* string) {
	if (typeId >= 0) {
		const asITypeInfo* type = engine->GetTypeInfoById(typeId);
		if (!type)
			return asDOCGEN_CouldNotFindTypeById;
		if (!objectTypeDocumentation.insert({type, {string}}).second)
			return asDOCGEN_AlreadyDocumented;
	}
	return typeId;
}

const char* DocumentationGenerator::Impl::GetDocumentationForType(const asITypeInfo* type) const {
	auto it = objectTypeDocumentation.find(type);
	if (it != objectTypeDocumentation.end())
		return it->second.c_str();
	return "";
}

const char* DocumentationGenerator::Impl::GetDocumentationForFunction(const asIScriptFunction* function) const {
	auto it = functionDocumentation.find(function);
	if (it != functionDocumentation.end())
		return it->second.c_str();
	return "";
}

DocumentationGenerator::DocumentationGenerator(asIScriptEngine* engine, const ScriptDocumentationOptions& options)
	: impl(new Impl(engine, options))
{	
}

DocumentationGenerator::~DocumentationGenerator() {
	delete impl;
}

int DocumentationGenerator::DocumentGlobalFunction(int r, const char* string) {
	return impl->DocumentFunction(r, string);
}

int DocumentationGenerator::DocumentObjectType(int r, const char* string) {
	return impl->DocumentType(r, string);
}

int DocumentationGenerator::DocumentObjectMethod(int r, const char* string) {
	return impl->DocumentFunction(r, string);
}

int DocumentationGenerator::DocumentInterface(int r, const char* string) { 
	return impl->DocumentFunction(r, string);
}

int DocumentationGenerator::DocumentInterfaceMethod(int r, const char* string) { 
	return impl->DocumentFunction(r, string); 
}

int DocumentationGenerator::Generate() {
	return impl->generate();
}

END_AS_NAMESPACE

#endif
