#pragma once

#include <cdk/ast/unary_operation_node.h>
#include <cdk/ast/expression_node.h>

namespace udf {

    /**
     * Class for describing tensor dim nodes.
     */
    class tensor_dim_node : public cdk::unary_operation_node {
        cdk::expression_node *_tensor;

    public:
        tensor_dim_node(int lineno, cdk::expression_node *tensor, cdk::expression_node *arg) :
            cdk::unary_operation_node(lineno, arg), _tensor(tensor) {}

        cdk::expression_node *tensor() { return _tensor; }
  
        void accept(basic_ast_visitor *sp, int level) { sp -> do_tensor_dim_node(this, level); }
  
    };
  
} // udf