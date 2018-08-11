//
//  CSSLex.cpp
//  DDCSSParser
//
//  Created by 1m0nster on 2018/8/7.
//  Copyright © 2018 1m0nster. All rights reserved.
//

#include "CSSLex.hpp"

#define NextChar(buffer) *(buffer + m_forwardPos++)
#define ErrorInLoop STATUS = LexError;stopLoop = true;
#define WS_CASE ' ': case '\r': case '\n': case '\t': case '\f'
namespace future {
    Lex::Lex()
    {
        m_buffer = 0;
        m_firstPos = 0;
        m_forwardPos = 0;
    }
    
    Lex::~Lex()
    {
        CleanResource();
    }
    Lex* Lex::GetLex()
    {
        static Lex* lexer = 0;
        if (!lexer) {
            lexer = new Lex;
        }
        return lexer;
    }
    
    void Lex::SetBufferSource(const std::string fileName)
    {
        if (fileName.empty()) {
            return;
        }
        FILE* fileHandler = fopen(fileName.c_str(), "r");
        if ( !fileHandler ) {
            return;
        }
        size_t bufferSize = 0;
        fseek(fileHandler, 0, SEEK_END);
        bufferSize = ftell(fileHandler);
        if ( !bufferSize ) {
            return;
        }
        fseek(fileHandler, 0, SEEK_SET);
        if (m_buffer) {
            delete [] m_buffer;
            m_buffer = 0;
        }
        m_buffer = new char[bufferSize];
        memset((void *)m_buffer, 0, bufferSize);
        m_bufferSize = fread((void *)m_buffer, 1, bufferSize, fileHandler);
        fclose(fileHandler);
    }
    
    Lex::CSSToken* Lex::GetTextToken(char stringType)
    {
        const char  _START = 0;
        const char  _ESCAPESTART = 1;
        const char  _STRING = 2;
        char status = _START;
        CSSToken* token = new CSSToken;
        m_tokenCache.push_back(token);
        m_forwardPos = m_firstPos;
        if (m_forwardPos >= m_bufferSize) {
            token->type = END;
            return token;
        }
        char stringBoundary = stringType == 1 ? '"' : '\'';
        while (1) {
            char c = NextChar(m_buffer);
            switch (status) {
                case _START: {
                    if ((c != '\r' && c != '\n' && c != '\f' && c != stringBoundary) || c & 0x80) {
                        status = _STRING;
                        break;
                    } else if (c == '\\') {
                        status = _ESCAPESTART;
                        break;
                    }
                    break;
                }
                case _ESCAPESTART: {
                    if (isDigitalCharacter(c) || isLetter(c)) {
                        // [0-9a-z]{1,6}
                        for(int i = 0; i < 5; i ++) {
                            c = NextChar(m_buffer);
                            if (isDigitalCharacter(c) || isLetter(c)) {
                                continue;
                            } else {
                                break;
                            }
                        }
                    } else if (c == '\r' || c == '\n' || c == '\f') {
                        // \\{nl}
                        // nl \n|\r\n|\r|\f
                    } else {
                        // escape's \\[^\n\r\f0-9a-f]
                    }
                    status = _STRING;
                    break;
                }
                case _STRING: {
                    if ((c != '\r' && c != '\n' && c != '\f' && c != stringBoundary) || c & 0x80 || c == '\\') {
                        --m_forwardPos;
                        status = _START;
                    } else {
                        token->type = STRING;
                        size_t size = m_forwardPos - m_firstPos + 1;
                        if (size - 1) {
                            char* data = new char[size];
                            data[size - 1] = '\0';
                            memcpy(data, m_buffer + m_firstPos, size - 1);
                            token->data = data;
                        }
                        return token;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        return token;
    }
    
    Lex::CSSToken* Lex::GetIdentToken()
    {
        CSSToken* token = new CSSToken;
        m_tokenCache.push_back(token);
        token->type = IDENT;
        CSSDFAStatus STATUS = Start;
        bool stopLoop = false;
        while(1) {
            if (m_forwardPos >= m_bufferSize) {
                STATUS = end;
                break;
            }
            unsigned char c = NextChar(m_buffer);
            switch (STATUS) {
                case Start: {
                    if (c == IDENT_START_SIGN) {
                        STATUS = iDentStart;
                    } else if (isLetter(c) || c == UNDER_LINE_SIGN || c & 0x80) {
                        STATUS = NMStart;
                    } else if (c == BACK_SPLASH) {
                        STATUS = EscapeStartInNMStart;
                    } else if (isWs(c)) {
                        STATUS = Ws;
                    } else if (c == HASH_SIGN) {
                        STATUS = HashStart;
                    } else if (c == KEYWORD_SIGN) {
                        STATUS = AtKeyWordStart;
                    } else {
                        STATUS = LexError;
                        stopLoop = true;
                    }
                    break;
                }
                case iDentStart: {
                    if (isLetter(c) || c == UNDER_LINE_SIGN || c & 0x80) {
                        STATUS = NMStart;
                    } else if (c == BACK_SPLASH) {
                        STATUS = EscapeStartInNMStart;
                    } else {
                        STATUS = LexError;
                        stopLoop = true;
                    }
                    break;
                }
                case NMStart: {
                    if (isLetter(c) || isDigitalCharacter(c) || c == UNDER_LINE_SIGN || c == IDENT_START_SIGN || c &0x80) {
                        STATUS = NMChar;
                    } else if (c == BACK_SPLASH) {
                        STATUS = EscapeStartInNMChar;
                    } else {
                        STATUS = iDent;
                        stopLoop = true;
                    }
                    break;
                }
                case NMChar: {
                    if (isLetter(c) || isDigitalCharacter(c) || c == UNDER_LINE_SIGN || c == IDENT_START_SIGN || c &0x80) {
                        STATUS = NMChar;
                    } else if (c == BACK_SPLASH) {
                        STATUS = EscapeStartInNMChar;
                    } else {
                        STATUS = iDent;
                        stopLoop = true;
                    }
                    break;
                }
                case EscapeStartInNMStart: {
                    if (isHexCharacter(c)) {
                        for (int i = 0; i < 5; i++) {
                            c = NextChar(m_buffer);
                            if (isHexCharacter(c)) {
                                continue;
                            } else {
                                --m_forwardPos;
                                break;
                            }
                        }
                        STATUS = NMStart;
                    }
                    break;
                }
                case EscapeStartInNMChar: {
                    if (isHexCharacter(c)) {
                        for (int i = 0; i < 5; i++) {
                            c = NextChar(m_buffer);
                            if (isHexCharacter(c)) {
                                continue;
                            } else {
                                --m_forwardPos;
                                break;
                            }
                        }
                        STATUS = NMChar;
                    }
                    break;
                }
                default: {
                    STATUS = LexError;
                    break;
                }
            }
            if (stopLoop) {
                --m_forwardPos;
                break;
            }
        }
        size_t size = m_forwardPos - m_firstPos + 1;
        if (size - 1) {
            char *tokenData = new char[size];
            tokenData[size - 1] = '\0';
            memcpy(tokenData, (m_buffer + m_firstPos), size - 1);
            token->data = tokenData;
        }
        if (STATUS == iDent) {
            token->type = IDENT;
        } else if (STATUS == end) {
            token->type = END;
        } else {
            token->type = ERROR;
        }
        m_firstPos = m_forwardPos;
        return token;
    }
    
    Lex::CSSToken* Lex::GetToken()
    {
        CSSToken* token = new CSSToken;
        m_tokenCache.push_back(token);
        token->type = IDENT;
        m_firstPos = m_forwardPos;
        bool stopLoop = false;
        const char* data = 0;
        static CSSDFAStatus STATUS;
        if (m_firstPos >= m_bufferSize || !m_buffer) {
            token->type = END;
            return token;
        }
        while(1) {
            char c = NextChar(m_buffer);
            switch (STATUS) {
                case Start: {
                    switch (c) {
                        case '@': {
                            CSSToken* identToken = GetIdentToken();
                            stopLoop = true;
                            if (identToken->type == IDENT) {
                                STATUS = AtKeyWord;
                                data = copyData(identToken);
                            } else {
                                STATUS = LexError;
                            }
                            break;
                        }
                        case '#': {
                            CSSToken* identToken = GetIdentToken();
                            stopLoop = true;
                            if (identToken->type == IDENT) {
                                STATUS = Hash;
                                data = copyData(identToken);
                            } else {
                                STATUS = LexError;
                            }
                            break;
                        }
                        case '~': {
                            if (NextChar(m_buffer) == EQUAL_SIGN) {
                                STATUS = include;
                            } else {
                                STATUS = tidle;
                                --m_forwardPos;
                            }
                            break;
                        }
                        case '|': {
                            if (NextChar(m_buffer) == EQUAL_SIGN) {
                                STATUS = dashMatch;
                            } else {
                                STATUS = LexError;
                            }
                            break;
                        }
                        case '^': {
                            if (NextChar(m_buffer) == EQUAL_SIGN) {
                                STATUS = prefixMatch;
                            } else {
                                STATUS = LexError;
                            }
                            break;
                        }
                        case '$': {
                            if (NextChar(m_buffer) == EQUAL_SIGN) {
                                STATUS = suffixMatch;
                            } else {
                                STATUS = LexError;
                            }
                            break;
                        }
                        case '*': {
                            if (NextChar(m_buffer) == EQUAL_SIGN) {
                                STATUS = subStringMatch;
                            } else {
                                --m_forwardPos;
                                STATUS = star;
                            }
                            break;
                        }
                        case '"': {
                            STATUS = string1Start;
                            break;
                        }
                        case '\'': {
                            STATUS = string2Start;
                            break;
                        }
                        case WS_CASE: {
                            STATUS = Ws;
                            break;
                        }
                        case '.': {
                            STATUS = dot;
                            stopLoop = true;
                            break;
                        }
                        case '{': {
                            STATUS = blockStart;
                            stopLoop = true;
                            break;
                        }
                        case '}': {
                            STATUS = blockEnd;
                            stopLoop = true;
                            break;
                        }
                        case ',': {
                            STATUS = comma;
                            stopLoop = true;
                            break;
                        }
                        case '>': {
                            STATUS = greater;
                            stopLoop = true;
                            break;
                        }
                        case '+': {
                            STATUS = plus;
                            stopLoop = true;
                            break;
                        }
                        case '[': {
                            STATUS = leftSqureBracket;
                            break;
                        }
                        case ']': {
                            STATUS = rightSqureBracket;
                            break;
                        }
                        case ':': {
                            STATUS = colon;
                            break;
                        }
                        case '=': {
                            STATUS = equal;
                            break;
                        }
                        case '/': {
                            char cn = NextChar(m_buffer);
                            if (cn == '*') {
                                STATUS = annotationStart;
                            } else {
                                --m_forwardPos;
                                goto identLabel;
                            }
                            break;
                        }
                            
                        default: {
                        identLabel:
                            m_forwardPos = m_firstPos;
                            CSSToken* idToken = GetIdentToken();
                            if (idToken->type == IDENT) {
                                STATUS = iDent;
                                data = copyData(idToken);
                            } else if (idToken->type == END) {
                                STATUS = end;
                            } else {
                                STATUS = LexError;
                            }
                            stopLoop = true;
                            break;
                        }
                    }
                    break;
                }
                case Ws: {
                    stopLoop = true;
                    if (isWs(c)) {
                        STATUS = Ws;
                        stopLoop = false;
                    } else if (c == PLUS_SIGN) {
                        STATUS = plus;
                    } else if (c == COMMA_SIGN) {
                        STATUS = comma;
                    } else if (c == GREATER_SIGN) {
                        STATUS = greater;
                    } else  if (c == TIDLE_SIGN) {
                        STATUS = tidle;
                    } else {
                        --m_forwardPos;
                        STATUS = Ws;
                    }
                    break;
                }
                case include: {
                    stopLoop = true;
                }
                case blockStart: {
                    if (c == BLOCK_END_SIGN) {
                        STATUS = blockEnd;
                    }
                    while (c != BLOCK_END_SIGN) {
                        if (m_forwardPos >= m_bufferSize) {
                            STATUS = LexError;
                            break;
                        } else {
                            c = NextChar(m_buffer);
                            STATUS = blockEnd;
                        }
                    }
                    size_t size = m_forwardPos - m_firstPos + 1;
                    if (size - 1) {
                        char *ptr = new char[size];
                        ptr[size] = '\0';
                        memcpy((void *)ptr, m_buffer + m_firstPos, size - 1);
                        data = ptr;
                    }
                    stopLoop = true;
                    break;
                }
                case string1Start: {
                    CSSToken* stringToken = GetTextToken(1);
                    if (stringToken->type == STRING) {
                        STATUS = string1End;
                        data = copyData(stringToken);
                    } else {
                        STATUS = LexError;
                    }
                    break;
                }
                case string2Start: {
                    CSSToken* stringToken = GetTextToken(2);
                    if (stringToken->type == STRING) {
                        STATUS = string2End;
                        data = copyData(stringToken);
                    } else {
                        STATUS = LexError;
                    }
                    break;
                }
                case annotationStart: {
                    char cn = NextChar(m_buffer);
                    if (c == '*' && cn == '/') {
                        STATUS = annotationEnd;
                    } else {
                        --m_forwardPos;
                        continue;
                    }
                    break;
                }
                    
                default:
                    break;
            }
            bool resetStatus = true;
            switch (STATUS) {
                case AtKeyWord: {
                    token->type = ATKEYWORD;
                    token->data = data;
                    break;
                }
                case LexError: {
                    token->type = ERROR;
                    token->data = 0;
                    break;
                }
                case Hash: {
                    token->type = HASH;
                    token->data = data;
                    break;
                }
                case Ws: {
                    token->type = WS;
                    token->data = 0;
                    break;
                }
                case include: {
                    token->type = INCLUDES;
                    token->data = 0;
                    stopLoop = true;
                    break;
                }
                case iDent: {
                    token->type = IDENT;
                    token->data = data;
                    break;
                }
                case dot: {
                    token->type = DOT;
                    token->data = 0;
                    break;
                }
                case blockStart: {
                    token->type = BLOCKSTART;
                    token->data = 0;
                    resetStatus = false;
                    break;
                }
                case blockEnd:{
                    token->type = BLOCKEND;
                    token->data = data;
                    STATUS = Start;
                    break;
                }
                case end: {
                    token->type = END;
                    resetStatus = false;
                    break;
                }
                case comma: {
                    token->type = COMMA;
                    break;
                }
                case plus:{
                    token->type = PLUS;
                    break;
                }
                case tidle: {
                    token->type = TIDLE;
                    stopLoop = true;
                    break;
                }
                case greater: {
                    token->type = GREATER;
                    token->data = 0;
                    stopLoop = true;
                    break;
                }
                case star:{
                    token->type = STAR;
                    stopLoop = true;
                    break;
                }
                case dashMatch:{
                    token->type = DASHMATCH;
                    stopLoop = true;
                    break;
                }
                case prefixMatch:{
                    token->type = PREFIXMATCH;
                    stopLoop = true;
                    break;
                }
                case suffixMatch:{
                    token->type = SUFFIXMATCH;
                    stopLoop = true;
                    break;
                }
                case includes:{
                    token->type = INCLUDES;
                    stopLoop = true;
                    break;
                }
                case subStringMatch:{
                    token->type = SUBSTRINGMATCH;
                    stopLoop = true;
                    break;
                }
                case leftSqureBracket: {
                    token->type = LEFTSQUREBRACKET;
                    stopLoop = true;
                    break;
                }
                case rightSqureBracket: {
                    token->type = RIGHTSQUREBRACKET;
                    stopLoop = true;
                    break;
                }
                case colon: {
                    token->type = COLON;
                    stopLoop = true;
                    break;
                }
                case equal: {
                    token->type = EQUAL;
                    stopLoop = true;
                    break;
                }
                case string1End: case string2End: {
                    token->type = STRING;
                    token->data = data;
                    stopLoop = true;
                    break;
                }
                case annotationEnd: {
                    token->type = ANNOTATION;
                    token->data = 0;
                    stopLoop = true;
                    break;
                }
                default:
                    break;
            }
            if (stopLoop) {
                if (resetStatus) {
                    STATUS = Start;
                }
                break;
            }
        }
        return token;
    }
    
    const char* Lex::copyData(future::Lex::CSSToken *from)
    {
        size_t size = strlen(from->data);
        if (!size) {
            return 0;
        }
        char* ptr = new char[size + 1];
        ptr[size] = '\0';
        memcpy(ptr, from->data, size);
        return ptr;
    }
    
    void Lex::CleanResource()
    {
        delete [] m_buffer;
        m_buffer = 0;
        auto it = m_tokenCache.begin();
        auto end = m_tokenCache.end();
        while (it != end) {
            delete *it++;
        }
        m_tokenCache.clear();
    }
    
    inline bool Lex::isDigitalCharacter(char c) {
        return (c > 0x2F && c < 0x3A);
    }
    
    inline bool Lex::isLetter(char c) {
        return (c > 0x60 && c < 0x7B) || (c > 0x40 && c < 0x5B);
    }
    
    inline bool Lex::isHexCharacter(char c) {
        return isDigitalCharacter(c) || (c > 0x60 && c < 0x67) || (c > 0x40 && c < 0x47);
    }
    
    inline bool Lex::isWs(char c) {
        return (c == ' ' || c == '\r' || c == '\n' || c == '\f' || c == '\t');
    }
}