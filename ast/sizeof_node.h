#pragma once

#include <cdk/ast/expression_node.h>

namespace udf {

    /**
     * Class for describing sizeof nodes.
     */
    class sizeof_node : public cdk::expression_node {
        cdk::expression_node *_arg;
  
    public:
        sizeof_node(int lineno, cdk::expression_node *arg) : 
            cdk::expression_node(lineno), _arg(arg) {}

        cdk::expression_node *arg() { return _arg; }

        void accept(basic_ast_visitor *sp, int level) { sp -> do_sizeof_node(this, level); }
  
    };
  
} // udf