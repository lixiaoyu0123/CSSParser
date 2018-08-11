//
//  main.cpp
//  DDCSSParser
//
//  Created by 1m0nster on 2018/8/1.
//  Copyright © 2018 1m0nster. All rights reserved.
//

#include <iostream>
#include "CSSParser.hpp"
#include "KeywordItem.hpp"
#include "gumbo.h"
#include "GumboInterface.h"
#include "HTMLCSSRefAdaptor.h"

void tranversTree(GumboNode *root, future::Selector* selector)
{
    if (root->type == GUMBO_NODE_ELEMENT) {
        GumboVector children = root->v.element.children;
        for (unsigned int i = 0; i < children.length; i++) {
            GumboNode*child = (GumboNode *)(children.data[i]);
            tranversTree(child, selector);
        }
        const char* name = HTMLTagNames[root->v.element.tag];
        printf("%s match selector %d\n", name, future::HTMLCSSRefAdaptor::nodeAdaptToSelector(&root, selector));
    }
}

int main(int argc, const char * argv[]) {
    using namespace future;
    FILE *f = fopen("/Users/lmonster/Desktop/test.html", "r");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* ptr = new char[size];
    fread((void *)ptr, size, 1, f);
    fclose(f);
    
    GumboInterface gi(ptr, "2");
    CSSParser::getParser()->parser("/Users/lmonster/Desktop/test.css");
    std::list<Selector *>selectors = CSSParser::getParser()->getSelectors();
    for (Selector *s : selectors) {
        tranversTree(gi.get_root_node(), s);
        printf("\n");
    }
    return 0;
}