#pragma once

#include <cdk/ast/unary_operation_node.h>
#include <cdk/ast/expression_node.h>

namespace udf {

    /**
     * Class for describing objects (stack allocation) nodes.
     */
    class objects_node : public cdk::unary_operation_node {

        public:
            objects_node(int lineno, cdk::expression_node *arg) : 
                cdk::unary_operation_node(lineno, arg) {}

            void accept(basic_ast_visitor *sp, int level) { sp -> do_objects_node(this, level); }
    };

} // udf