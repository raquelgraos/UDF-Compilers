#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/lvalue_node.h>

namespace udf {

    /**
     * Class for describing address nodes.
     */
    class address_node : public cdk::expression_node {
        cdk::lvalue_node *_value;

    public:
        address_node(int lineno, cdk::lvalue_node *value) : 
            cdk::expression_node(lineno), _value(value) {}

        cdk::lvalue_node *value() { return _value; }
  
        void accept(basic_ast_visitor *sp, int level) { sp -> do_address_node(this, level); }
  
    };
  
} // udf