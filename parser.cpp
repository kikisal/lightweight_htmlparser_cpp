#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include <vector>
#include <cstdint>

#define DEBUG_PARSER

enum fs_status_enum {
    FSU_SUCCESS = 0,
    FSU_ERR = 1
};

namespace Utils {
    std::string readFile(std::string filePath, fs_status_enum* state) {

        try {
            std::ifstream ifs{filePath, std::ios_base::binary};
            
            ifs.seekg(0, std::ios::end);
            size_t size = ifs.tellg();
            std::string buffer(size, ' ');

            if (size > 0 && buffer.size() != size) 
            {
                *state = FSU_ERR;
                return std::string{};
            }

            ifs.seekg(0);
            ifs.read(&buffer[0], size);
            
            if (state)
                *state = FSU_SUCCESS;

            return buffer;
        } catch(...) {
            *state = FSU_ERR;
            return std::string{};
        }

    }
}

class HTMLNode {

public:
    static HTMLNode* create() {
        return new HTMLNode();
    }

    friend class HTMLParser;
    
    /// @brief 
    HTMLNode();
    /// @brief 
    /// @param tag 
    HTMLNode(std::string tag);

    /// @brief 
    /// @param child 
    /// @return appended node
    HTMLNode* appendNode(HTMLNode* child);

    /// @brief 
    /// @param child 
    /// @return removed node
    HTMLNode* removeNode(HTMLNode* child);

    /// @brief 
    /// @return 
    inline auto childrenCount() const {
        return children.size();
    }

    /// @brief iterator begin
    /// @return iterator
    auto begin() {
        return children.begin();
    }

    /// @brief iterator end
    /// @return iterator
    auto end() {
        return children.end();
    }

    /// @brief 
    /// @param name 
    /// @return 
    HTMLNode* setTagName(std::string name);

    /// @brief 
    /// @param content 
    /// @return 
    inline HTMLNode* setTextContent(std::string content) {
        textContent = content;
        return this;
    };

    /// @brief 
    /// @return 
    inline std::string getTextContent() const {
        return textContent;
    }

    /// @brief 
    /// @return 
    inline std::string tagName() const {
        return tag;
    }


    
#ifdef DEBUG_PARSER
public:
    void printChildren();
#endif // #ifdef DEBUG_PARSER


private:
    std::string                        tag;
    std::string                        textContent;
    std::map<std::string, std::string> attribs;
    std::vector<HTMLNode*>             children;
    HTMLNode*                          parent = nullptr;
};


/// @brief Lightweight HTML Parser.
class HTMLParser {
public:
    /// @brief 
    /// @param filepath 
    HTMLParser(std::string filepath);
    
    /// @brief 
    void parse();

    /// @brief 
    /// @return 
    inline HTMLNode* document() const { return root; }

protected:

    /// @brief 
    /// @param node 
    /// @return error code
    int parseInternal(HTMLNode* node);
    
    /// @brief 
    /// @param node 
    /// @return 
    int parseChildrenOf(HTMLNode* node);
    
    /// @brief 
    /// @param node 
    /// @return 
    int parseTextContent(HTMLNode* node);
    
    /// @brief 
    /// @param tagName 
    /// @return 
    int parseTagName(std::string & tagName);

    /// @brief 
    /// @param tag 
    /// @return 
    int readClosingTag(std::string& tag);
    
    /// @brief 
    /// @param result 
    /// @return 
    int parseTag(HTMLNode* result);
    
    /// @brief 
    /// @param ch 
    void skipUntil(char ch);

    /// @brief 
    /// @param ch 
    void skipUntilNot(char ch);

    /// @brief 
    /// @param size 
    void putback(size_t size);

    /// @brief 
    /// @return 
    bool available() {
        return readingPos < rawLen();
    }

    /// @brief 
    /// @return 
    inline int currentLine() const {
        return line + 1;
    }
    
    /// @brief 
    /// @return 
    inline int currentCol() const {
        return colmn;
    }
    
    /// @brief 
    /// @param advance 
    /// @return 
    char readChar(bool advance = true);

    void reset() {
        line       = 0;
        colmn      = 0;
        readingPos = 0;
        __pushResetState();
    }

    inline size_t rawLen() const {
        return htmlRawDocument.size();
    }

    void __pushResetState() {
        if (!badState()) {
            oldLine       = line;
            oldColmn      = colmn;
            oldReadingPos = readingPos;
        }
    }

    void pushResetState() {
        __pushResetState();
    }

    void popResetState() {
        line       = oldLine;
        colmn      = oldColmn;
        readingPos = oldReadingPos;
    }

    int parsingError(size_t code) {
        popResetState();
        parserState = code;

        return code;
    }

    /// @brief 
    /// @return 
    bool badState() const;

    int state() const {
        return parserState;
    }

    int LogError() {
        std::cout << "parsing error occured syntax on line " << currentLine() << ":" << currentCol() << std::endl;
        return state();
    }

    /// @brief returns next char without updating reading position cursor.
    /// @return next char in buffer.
    inline char nextChar() const {
        return readingPos + 1 < rawLen() ? htmlRawDocument[readingPos + 1] : -1;
    }

    void skipEmptyChars() {
        if (readChar(false) == ' ')
            skipUntilNot(' ');

        if (readChar(false) == '\n')
            skipUntilNot('\n');
    }

private:
    std::string filepath;
    std::string htmlRawDocument;
    bool sourceLoaded    = false;
    
    HTMLNode* root       = nullptr;

    size_t readingPos    = 0;

    
    // for debugging
    size_t line          = 0;
    size_t colmn         = 0;

    size_t oldLine       = 0;
    size_t oldColmn      = 0;
    size_t oldReadingPos = 0;

    int parserState      = 0;
};

HTMLNode::HTMLNode(std::string tag) : tag{tag} {}

HTMLNode::HTMLNode() {}

HTMLNode* HTMLNode::setTagName(std::string name) {
    tag = name;
    return this;
}

HTMLNode* HTMLNode::appendNode(HTMLNode* child) {
    if (!child)
        return nullptr;
    
    if (child->parent != this && child->parent != nullptr)
        child->parent->removeNode(child);

    children.push_back(child);

    child->parent = this;

    return child;
}

HTMLNode* HTMLNode::removeNode(HTMLNode* child) {
    if (!child)
        return nullptr;

    child->parent = nullptr;
    
    auto it = children.begin();
    while (it != children.end()) {
        if ((*it) == child)
        {
            children.erase(it);
            return child;    
        }

        it = std::next(it);
    }


    return child;
}

// PARSER IMPLEMENTATION

HTMLParser::HTMLParser(std::string fp) : filepath {fp} {}

#ifdef DEBUG_PARSER
void HTMLNode::printChildren() {
    std::cout << "Children of (" << tagName() << ", #" << reinterpret_cast<uint64_t>(this) << ")" << std::endl;

    for (auto c : *this)
        std::cout << "<" << c->tagName() << ">" << std::endl;

    std::cout << "--------------" << std::endl;
}
#endif // #ifdef DEBUG_PARSER


void HTMLParser::skipUntil(char ch) {
    while (available() && readChar() != ch);
}

void HTMLParser::skipUntilNot(char ch) {
    while (available() && readChar() == ch);
    --readingPos;
}

int HTMLParser::parseChildrenOf(HTMLNode* node) {
    if (badState())
        return parserState;

    return 0;
}

// either in: [HERE...<%TAG%></%TAG>]
// or in: [<T1></T1>, ..., <Tn></Tn>]HERE...
int HTMLParser::parseTextContent(HTMLNode* node) {
    if (badState())
        return parserState;

    if (!node)
        return parsingError(-1);

    std::string content = node->getTextContent();

    while(available()) {
        char ch = readChar();
        if (ch == '<')
        {
            putback(1);
            break;
        } else if (ch == '\n') 
            continue;
        else
            content.push_back(ch);
    }

    node->setTextContent(content);

    return 0;
}

void HTMLParser::putback(size_t size) {
    readingPos -= size;
    if (readingPos < 0)
        readingPos = 0;
}

bool HTMLParser::badState() const {
    return parserState != 0;
}

int HTMLParser::parseInternal(HTMLNode* node) {
    if (badState())
        return parserState;
    // look for either token < or an alpha char

    skipEmptyChars();
    
    bool firstCharFound = false;

    while(available()) {

        auto ch = readChar();

        if (ch == '<') {

            skipEmptyChars();

            if (!available())
                return 0;

            if (readChar() == '/')
            {
                
                std::string closingTag;

                readClosingTag(closingTag);
                
                if (closingTag != node->tagName())
                    return parsingError(-2);

                return 0;
            }

            putback(1);

            HTMLNode* n = HTMLNode::create();
            n->parent   = node;

            node->appendNode(n);

            parseTag(n);
            
            // <%TAG%>...[<T1>, <T2>, ..., <Tn>]</%TAG%>
            //parseChildrenOf(n); // this can recurse.


            parseInternal(n);


            if (badState())
                return LogError();
        } else {
            
            putback(1);

            if (!firstCharFound) {
                firstCharFound = true;
                // HERE...<%TAG%>
                parseTextContent(node);
            } else {
                skipEmptyChars();
                        
                // </%TAG%>HERE...
                parseTextContent(node);
            }
        }

    }

    return 0;
}

int HTMLParser::parseTagName(std::string & tagName) {
    std::string result;

    bool tagNameParsed = false;

    pushResetState();

    while (available()) {
        char ch = readChar();

        if (ch == '>' || ch == ' ' || ch == '\n') {
            tagName       = result;
            tagNameParsed = true;
        }

        if ((ch == ' '  || ch == '\n') && tagNameParsed)
            skipUntil('>');

        if (tagNameParsed)
            return 0;

        result.push_back(ch);
    }

    tagName = result;
    return 0;
}


int HTMLParser::readClosingTag(std::string& tag) {
    return parseTagName(tag);
}

int HTMLParser::parseTag(HTMLNode* result) {

    if (badState())
        return parserState;

    pushResetState();

    if (!result)
        return parsingError(-1);

    std::string tagName;

    int ts = parseTagName(tagName);
    if (ts != 0)
        return parsingError(ts);

    result->setTagName(tagName);

    return 0;
}

void HTMLParser::parse() {
    fs_status_enum state = FSU_ERR;
    
    if (!sourceLoaded) {
        htmlRawDocument = Utils::readFile(filepath, &state);
        if (state != FSU_SUCCESS)
            return;

        sourceLoaded = true;
    }
    // initialize parser state (only one positional state for now).
    reset();

    root = HTMLNode::create();
    parseInternal(root);
}

char HTMLParser::readChar(bool advance) {
    if (!available())
        return -1;

    char c = htmlRawDocument[readingPos];

    if (advance) {
        ++readingPos;

        // for debugging.
        if (c == '\n') {
            ++line;
            colmn = 0;
        } else 
            ++colmn;
    }
        
    return c;
}

void printChildren(HTMLNode* node) {

    std::cout << "text content of " << node->tagName() << ": " << std::endl;
    std::cout << node->getTextContent() << std::endl;    


    if (node->childrenCount() > 0) {
        for (auto c : *node) {
            printChildren(c);
        }
    }

}

int main() {
    HTMLParser p{"./test.html"};

    p.parse();

    printChildren(p.document()->setTagName("document"));
        
    return 0;
}