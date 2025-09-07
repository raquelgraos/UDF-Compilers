#pragma once

#include <cdk/ast/typed_node.h>
#include <cdk/ast/sequence_node.h>
#include "ast/block_node.h"
#include <string>

namespace udf {

  /**
   * Class for describing function definition nodes.
   */
  class function_definition_node : public cdk::typed_node {
    int _qualifier;
    std::string _identifier;
    cdk::sequence_node *_args;
    udf::block_node *_block;

  public:
    function_definition_node(int lineno, int qualifier, std::shared_ptr<cdk::basic_type> retType,
                                const std::string &identifier, cdk::sequence_node *args, udf::block_node *block) :
            cdk::typed_node(lineno), _qualifier(qualifier), _identifier(identifier), _args(args), _block(block) {
        std::vector<std::shared_ptr<cdk::basic_type>> arguments;
        for (size_t i = 0; i < args->size(); i++) {
          arguments.push_back(dynamic_cast<cdk::typed_node*>(args->node(i))->type());
        }
        type(cdk::functional_type::create(arguments, retType));
    }

    int qualifier() { return _qualifier; }

    const std::string &identifier() const { return _identifier; }

    cdk::sequence_node *args() { return _args; }

    cdk::typed_node *arg(size_t ax) { return dynamic_cast<cdk::typed_node*>(_args->node(ax)); }

    udf::block_node *block() { return _block; }

    void accept(basic_ast_visitor *sp, int level) { sp -> do_function_definition_node(this, level); }

  };

} // udf