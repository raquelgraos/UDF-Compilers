#pragma once

#include <cdk/ast/sequence_node.h>
#include <cdk/ast/basic_node.h>

namespace udf {

  /**
   * Class for describing for-cycle nodes.
   */
  class unless_node : public cdk::basic_node {
    cdk::expression_node *_condition;
    cdk::expression_node *_vector;
    cdk::expression_node *_count;
    std::string _function;

  public:
    unless_node(int lineno, cdk::expression_node *condition, cdk::expression_node *vector, 
              cdk::expression_node *count, std::string &function) :
        basic_node(lineno), _condition(condition), _vector(vector), _count(count), _function(function) {
    }

    cdk::expression_node *condition() { return _condition; }

    cdk::expression_node *vector() { return _vector; }

    cdk::expression_node *count() { return _count; }
    
    const std::string &function() const { return _function; }

    void accept(basic_ast_visitor *sp, int level) { sp->do_unless_node(this, level); }

  };

} // udf