#pragma once

#include <cdk/ast/lvalue_node.h>
#include <cdk/ast/expression_node.h>

namespace udf {

    /**
     * Class for describing tensor index nodes.
     */
    class tensor_index_node : public cdk::lvalue_node {
        cdk::expression_node *_tensor;
        cdk::sequence_node *_indexes;

    public:
        tensor_index_node(int lineno, cdk::expression_node *tensor, cdk::sequence_node *indexes) :
            cdk::lvalue_node(lineno), _tensor(tensor), _indexes(indexes) {}

        cdk::expression_node *tensor() { return _tensor; }

        cdk::sequence_node *indexes() { return _indexes; }
  
        void accept(basic_ast_visitor *sp, int level) { sp -> do_tensor_index_node(this, level); }
  
    };
  
} // udf