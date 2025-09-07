#pragma once

#include <cdk/ast/binary_operation_node.h>
#include <cdk/ast/expression_node.h>

namespace udf {

    /**
     * Class for describing tensor contraction nodes.
     */
    class tensor_contraction_node : public cdk::binary_operation_node {

    public:
        tensor_contraction_node(int lineno, cdk::expression_node *tensor1, cdk::expression_node *tensor2) :
            cdk::binary_operation_node(lineno, tensor1, tensor2) {}
  
        void accept(basic_ast_visitor *sp, int level) { sp -> do_tensor_contraction_node(this, level); }
  
    };
  
} // udf