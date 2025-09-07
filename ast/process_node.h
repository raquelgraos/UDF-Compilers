#pragma once

#include <cdk/ast/sequence_node.h>
#include <cdk/ast/basic_node.h>

namespace udf {

  class process_node : public cdk::basic_node {
    cdk::expression_node *_vector;
    cdk::expression_node *_low;
    cdk::expression_node *_high;
    std::string _function;

  public:
    process_node(int lineno, cdk::expression_node *vector, cdk::expression_node *low, 
              cdk::expression_node *high, std::string &function) :
        basic_node(lineno), _vector(vector), _low(low), _high(high), _function(function) {
    }

    cdk::expression_node *vector() { return _vector; }

    cdk::expression_node *low() { return _low; }

    cdk::expression_node *high() { return _high; }
    
    const std::string &function() const { return _function; }

    void accept(basic_ast_visitor *sp, int level) { sp->do_process_node(this, level); }

  };

} // udf
