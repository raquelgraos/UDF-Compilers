#pragma once

#include "targets/basic_ast_visitor.h"

#include <set>
#include <stack>
#include <sstream>
#include <cdk/emitters/basic_postfix_emitter.h>

namespace udf {

  //!
  //! Traverse syntax tree and generate the corresponding assembly code.
  //!
  class postfix_writer: public basic_ast_visitor {
    cdk::symbol_table<udf::symbol> &_symtab;

    std::set<std::string> _functions_to_declare;

    bool _errors;
    bool _in_function_args, _in_function_body;
    bool _return_seen;
    std::shared_ptr<udf::symbol> _function; // current function
    int _offset;
    bool _in_for_init;
    std::stack<int> _for_init, _for_incr, _for_end;

    std::string _current_body_ret_lbl; // where to jump when a return occurs

    cdk::basic_postfix_emitter &_pf;
    int _lbl;

  public:
    postfix_writer(std::shared_ptr<cdk::compiler> compiler, cdk::symbol_table<udf::symbol> &symtab, cdk::basic_postfix_emitter &pf) :
        basic_ast_visitor(compiler), _symtab(symtab), _errors(false), _in_function_args(false), _in_function_body(false), _return_seen(false), _function(nullptr), _offset(0),
        _current_body_ret_lbl(""), _pf(pf), _lbl(0) {
    }

  public:
    ~postfix_writer() {
      os().flush();
    }

  private:
    /** Method used to generate sequential labels. */
    inline std::string mklbl(int lbl) {
      std::ostringstream oss;
      if (lbl < 0)
        oss << ".L" << -lbl;
      else
        oss << "_L" << lbl;
      return oss.str();
    }

    void error(int lineno, std::string s) {
      std::cerr << "error: " << lineno << ": " << s << std::endl;
    }

    void linearize_tensor_data(cdk::sequence_node *values, std::vector<cdk::expression_node*> &flat_values);

    void extract_tensor_shape(cdk::sequence_node *values, std::vector<int> &shape);

    void do_operation(cdk::binary_operation_node *node, int lvl, int operation);

    void do_comparative_operation(cdk::binary_operation_node *node, int lvl, int operation);

  public:
  // do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h"       // automatically generated
#undef __IN_VISITOR_HEADER__
  // do not edit these lines: end

  };

} // udf
