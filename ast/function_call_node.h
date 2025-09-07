#pragma once

#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>
#include <string>

namespace udf {

  /**
   * Class for describing function call nodes.
   */
  class function_call_node : public cdk::expression_node {
    std::string _identifier;  
    cdk::sequence_node *_args;

  public:
    /**
     * Constructor for a function call without arguments.
     * An empty sequence is automatically inserted to represent
     * the missing arguments.
     */
    function_call_node(int lineno, const std::string &identifier) :
        cdk::expression_node(lineno), _identifier(identifier), _args(new cdk::sequence_node(lineno)) {}

    /**
     * Constructor for a function call with arguments.
     */
    function_call_node(int lineno, const std::string &identifier, cdk::sequence_node *args) :
            cdk::expression_node(lineno), _identifier(identifier), _args(args) {}

    const std::string &identifier() const { return _identifier; }

    cdk::sequence_node *args() { return _args; }

    cdk::typed_node *arg(size_t ax) { return dynamic_cast<cdk::typed_node*>(_args->node(ax)); }

    void accept(basic_ast_visitor *sp, int level) { sp -> do_function_call_node(this, level); }

  };

} // udf