#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>

namespace udf {

    /**
     * Class for describing tensor nodes.
     */
    class tensor_node : public cdk::expression_node {
        cdk::sequence_node *_values;
  
    public:

        tensor_node(int lineno, cdk::sequence_node *values) :
            cdk::expression_node(lineno), _values(values) {}

        cdk::sequence_node *values() { return _values; }
  
        void accept(basic_ast_visitor *sp, int level) { sp -> do_tensor_node(this, level); }
  
    };
  
} // udf