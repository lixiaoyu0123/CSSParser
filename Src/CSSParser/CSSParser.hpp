//
//  CSSParser.hpp
//  DDCSSParser
//
//  Created by 1m0nster on 2018/8/7.
//  Copyright © 2018 1m0nster. All rights reserved.
//

#ifndef CSSParser_hpp
#define CSSParser_hpp

#include <stdio.h>
#include <iostream>
#include <stack>
#include "CSSLex.hpp"
#include "CSSParserStatus.h"
#include "Keyword/KeywordItem.hpp"
#include "Selectors/SelectorsHeader.h"

namespace future {
    class Lex;
    class Selector;
    class CSSParser {
    public:
        /**
         * Abstract Syntax Tree node
         */
        struct ASTNode {
            Selector* head;
            ASTNode* left;
            ASTNode* right;
            ASTNode()
            {
                head = 0;
                left = 0;
                right = 0;
            }
        };
    public:
        CSSParser();
        ~CSSParser();
        /**
         * Start parsing a css file
         */
        bool                        parseByFile(const std::string& cssFile);
        
        /**
         * Start parsing css string
         */
        bool                        parseByString (const std::string& cssString);
        
        /**
         * Get the selector models
         */
        std::list<Selector *>       getSelectors();
        
        /**
         * Get the Keyworld models
         */
        std::list<KeywordItem *>    getKeywords();
    private:
        typedef void(*treeTranverseAction)(ASTNode *);
        typedef CSSParser::ASTNode *(*treeTranverseWithUserDataAction)(std::stack<CSSParser::ASTNode *>* stack);
    private:
        friend CSSParser::ASTNode* TreeTranverseCreateExpressionAction(std::stack<CSSParser::ASTNode *>*);
        
        static void             initialASTNode(ASTNode *target, Selector* head, ASTNode* left, ASTNode* right);
        
        static void             pushOperatedElement(std::stack<ASTNode *>&, Selector* head);
        
        bool                    parse();
        
        void                    prepareByFile(const std::string& filePath);
        
        void                    prepareByString(const std::string& cssString);
        
        void                    clean();
        
        bool                    startSelector(CSSTokenType);
        
        bool                    tokenHasInfo(CSSTokenType);
        
        bool                    topHaveSign(std::stack<Selector *>&);
        
        Selector*               getSelector(Lex::CSSToken* token);
        
        std::list<ASTNode *>    createATS(std::stack<Selector *>&);
        
        void                    pushSign(std::stack<Selector *>&, SignSelector::SignType);
        
        void                    buildReversePolishNotation(std::stack<ASTNode*>& operatorStack, std::stack<ASTNode*>& operandStack);
        
        void                    RMLtranverseAST(ASTNode *root, treeTranverseAction action);
        
        void                    LRMtranverseAST(ASTNode *root, treeTranverseAction action);
        
        void                    LMRtranverseAST(ASTNode *root, treeTranverseAction action);
        
        void                    MLRtranverseAST(ASTNode *root, treeTranverseWithUserDataAction action, void *userData);
    private:
        Lex*                        m_lexer;
        CSSParserStatus             m_status;
        std::string                 m_hostCssFile;
        std::list<Lex::CSSToken *>  m_tokenStack;
        std::list<Selector *>       m_selectors;
        std::list<KeywordItem *>    m_keywords;
    };
}

#endif /* CSSParser_hpp */
