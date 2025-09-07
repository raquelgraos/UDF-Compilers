#include <string>
#include "targets/type_checker.h"
#include ".auto/all_nodes.h"  // automatically generated
#include <cdk/types/primitive_type.h>

#include "udf_parser.tab.h"

#define ASSERT_UNSPEC { if (node->type() != nullptr && !node->is_typed(cdk::TYPE_UNSPEC)) return; }

//---------------------------------------------------------------------------

void udf::type_checker::do_sequence_node(cdk::sequence_node *const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl);
  }
}

udf::type_checker::~type_checker() {
  os().flush();
}

void udf::type_checker::do_unless_node(udf::unless_node * const node, int lvl) {
  node->condition()->accept(this, lvl + 2);

  if (node->condition()->is_typed(cdk::TYPE_UNSPEC)) {
    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->condition()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for condition arg");
  }

  node->vector()->accept(this, lvl + 2);

  if (!node->vector()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("wrong type for vector arg");
  }

  auto vec_ref = cdk::reference_type::cast(node->vector()->type());

  node->count()->accept(this, lvl + 2);

  if (node->count()->is_typed(cdk::TYPE_UNSPEC)) {
    node->count()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->count()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for count arg");
  }

  const std::string &id = node->function();
  auto symbol = _symtab.find(id);
  if (symbol == nullptr) throw std::string("symbol '" + id + "' is undeclared.");
  if (!symbol->is_function()) throw std::string("symbol '" + id + "' is not a function.");

  std::shared_ptr<cdk::functional_type> type = cdk::functional_type::cast(symbol->type());

  if (type->input_length() != 1) {
    throw std::string("given function must only take 1 arg");
  }

  if (type->input(0) != vec_ref->referenced()) {
    throw std::string("vector type and function don't match");
  }  

}

void udf::type_checker::do_process_node(udf::process_node *const node, int lvl) {
  node->vector()->accept(this, lvl + 2);

  if (!node->vector()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("wrong type for vector arg");
  }

  auto vec_ref = cdk::reference_type::cast(node->vector()->type());

  node->low()->accept(this, lvl + 2);

  if (node->low()->is_typed(cdk::TYPE_UNSPEC)) {
    node->low()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->low()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for low arg");
  }

  node->high()->accept(this, lvl + 2);

  if (node->high()->is_typed(cdk::TYPE_UNSPEC)) {
    node->high()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->high()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for high arg");
  }

  const std::string &id = node->function();
  auto symbol = _symtab.find(id);
  if (symbol == nullptr) throw std::string("symbol '" + id + "' is undeclared.");
  if (!symbol->is_function()) throw std::string("symbol '" + id + "' is not a function.");

  std::shared_ptr<cdk::functional_type> type = cdk::functional_type::cast(symbol->type());

  if (type->input_length() != 1) {
    throw std::string("function arg must take only 1 arg");
  }

  if (type->input(0) != vec_ref->referenced()) {
    throw std::string("vector type and function don't match");
  }

}

//---------------------------------------------------------------------------

void udf::type_checker::do_double_node(cdk::double_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

void udf::type_checker::do_integer_node(cdk::integer_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_string_node(cdk::string_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
}

void udf::type_checker::do_nullptr_node(udf::nullptr_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::reference_type::create(4, nullptr));
}

void udf::type_checker::do_tensor_node(udf::tensor_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->values()->accept(this, lvl + 2);

  node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));
  
  bool in_tensor = true;
  for (size_t i = 0; i < node->values()->size(); ++i) {
    auto elem = node->values()->node(i); 

    auto inner_tensor = dynamic_cast<udf::tensor_node*>(elem);
    if (!inner_tensor) {
      in_tensor = false;
      auto expr = dynamic_cast<cdk::expression_node*>(elem);
      if (!expr) throw std::string("Invalid tensor element");

      if (!expr->is_typed(cdk::TYPE_INT) && !expr->is_typed(cdk::TYPE_DOUBLE)) {
        throw std::string("Tensor elements must be convertible to doubles");
      }
    } else if (!in_tensor) {
      throw std::string("Bad tensor shape");
    }
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_nil_node(cdk::nil_node *const node, int lvl) {
  // EMPTY
}

void udf::type_checker::do_data_node(cdk::data_node *const node, int lvl) {
  // EMPTY
}

void udf::type_checker::do_not_node(cdk::not_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else {
    throw std::string("wrong type in unary logical expression: not");
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::processUnaryExpression(cdk::unary_operation_node *const node, int lvl) {
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_INT)){
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->argument()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->argument()->is_typed(cdk::TYPE_TENSOR)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));
  } else {
    throw std::string("wrong type in argument of unary expression");
  }
}

void udf::type_checker::do_unary_minus_node(cdk::unary_minus_node *const node, int lvl) {
  processUnaryExpression(node, lvl);
}

void udf::type_checker::do_unary_plus_node(cdk::unary_plus_node *const node, int lvl) {
  processUnaryExpression(node, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::processIntOnlyExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_INT)) throw std::string("integer expression expected in binary expression (left)");

  node->right()->accept(this, lvl + 2);
  if (!node->right()->is_typed(cdk::TYPE_INT)) throw std::string("integer expression expected in binary expression (right)");

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::processLogicalExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_INT) && !node->left()->is_typed(cdk::TYPE_DOUBLE)) {
    throw std::string("integer expression expected in binary logical expression (left)");
  }

  node->right()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_INT) && !node->left()->is_typed(cdk::TYPE_DOUBLE)) {
    throw std::string("integer expression expected in binary logical expression (right)");
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::processIDTExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));

  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));

  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));    
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));    
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));    
  
  } else if (node->left()->is_typed(cdk::TYPE_UNSPEC) && node->right()->is_typed(cdk::TYPE_UNSPEC)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));

  } else {
    throw std::string("wrong types in binary expression");
  }
}

void udf::type_checker::processIDPTExpression(cdk::binary_operation_node *const node, int lvl, bool sub) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl + 2);
  node->right()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));

  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));

  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));  
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));
  } else if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));    
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));
  } else if (node->left()->is_typed(cdk::TYPE_TENSOR) && node->right()->is_typed(cdk::TYPE_TENSOR)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));  
    
  } else if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT)) {
    node->type(node->left()->type());
  } else if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_POINTER)) {
    node->type(node->right()->type());
  
  } else if (sub) {
    if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER)) {
      if (!comparePtrTypes(node->left()->type(), node->right()->type()))
        throw std::string("invalid pointer subtraction: mismatched types");
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    }

  } else if (node->left()->is_typed(cdk::TYPE_UNSPEC) && node->right()->is_typed(cdk::TYPE_UNSPEC)) {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));

  } else {
    throw std::string("wrong types in binary expression");
  }
}

bool udf::type_checker::comparePtrTypes(std::shared_ptr<cdk::basic_type> left_type, std::shared_ptr<cdk::basic_type> right_type) {
  auto left_name = left_type->name();
  auto right_name = right_type->name();

  if (left_name == cdk::TYPE_POINTER) {
    if (right_name != cdk::TYPE_POINTER) return false;
    auto left_ref = cdk::reference_type::cast(left_type)->referenced();
    auto right_ref = cdk::reference_type::cast(right_type)->referenced();
    return comparePtrTypes(left_ref, right_ref);
  }
  return (left_name == right_name);
}

void udf::type_checker::processEqualityExpression(cdk::binary_operation_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);

  if ((node->left()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_DOUBLE) ||
    (node->left()->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT)) {
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->left()->type() != node->right()->type()) {
    throw std::string("same type expected on both sides of equality operator");
  } else {
    node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
}

void udf::type_checker::do_add_node(cdk::add_node *const node, int lvl) {
  processIDPTExpression(node, lvl, false);
}
void udf::type_checker::do_sub_node(cdk::sub_node *const node, int lvl) {
  processIDPTExpression(node, lvl, true);
}
void udf::type_checker::do_mul_node(cdk::mul_node *const node, int lvl) {
  processIDTExpression(node, lvl);
}
void udf::type_checker::do_div_node(cdk::div_node *const node, int lvl) {
  processIDTExpression(node, lvl);
}
void udf::type_checker::do_mod_node(cdk::mod_node *const node, int lvl) {
  processIntOnlyExpression(node, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_lt_node(cdk::lt_node *const node, int lvl) {
  processLogicalExpression(node, lvl);
}
void udf::type_checker::do_le_node(cdk::le_node *const node, int lvl) {
  processLogicalExpression(node, lvl);
}
void udf::type_checker::do_ge_node(cdk::ge_node *const node, int lvl) {
  processLogicalExpression(node, lvl);
}
void udf::type_checker::do_gt_node(cdk::gt_node *const node, int lvl) {
  processLogicalExpression(node, lvl);
}
void udf::type_checker::do_ne_node(cdk::ne_node *const node, int lvl) {
  processEqualityExpression(node, lvl);
}
void udf::type_checker::do_eq_node(cdk::eq_node *const node, int lvl) {
  processEqualityExpression(node, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_and_node(cdk::and_node *const node, int lvl) {
  processIntOnlyExpression(node, lvl);
}
void udf::type_checker::do_or_node(cdk::or_node *const node, int lvl) {
  processIntOnlyExpression(node, lvl);
}

//---------------------------------------------------------------------------

void udf::type_checker::do_variable_node(cdk::variable_node *const node, int lvl) {
  ASSERT_UNSPEC;
  const std::string &id = node->name();
  auto symbol = _symtab.find(id);

  if (symbol) {
    node->type(symbol->type());
  } else {
    throw std::string("undeclared variable '" + id + "'");
  }
}

void udf::type_checker::do_index_node(udf::index_node * const node, int lvl) {
  ASSERT_UNSPEC;
  std::shared_ptr < cdk::reference_type > btype;

  if (node->base()) {
    node->base()->accept(this, lvl + 2);
    btype = cdk::reference_type::cast(node->base()->type());
    if (!node->base()->is_typed(cdk::TYPE_POINTER)) throw std::string("pointer expression expected in index left-value");
  } else {
    btype = cdk::reference_type::cast(_function->type());
    if (!_function->is_typed(cdk::TYPE_POINTER)) throw std::string("return pointer expression expected in index left-value");
  }

  node->index()->accept(this, lvl + 2);
  if (!node->index()->is_typed(cdk::TYPE_INT)) throw std::string("integer expression expected in left-value index");

  node->type(btype->referenced());
}

void udf::type_checker::do_rvalue_node(cdk::rvalue_node *const node, int lvl) {
  ASSERT_UNSPEC;
  try {
    node->lvalue()->accept(this, lvl);
    node->type(node->lvalue()->type());
  } catch (const std::string &id) {
    throw "undeclared variable '" + id + "'";
  }
}

void udf::type_checker::do_assignment_node(cdk::assignment_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl + 4);
  node->rvalue()->accept(this, lvl + 4);

  if (node->lvalue()->is_typed(cdk::TYPE_INT)) {

    if (node->rvalue()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else {
      throw std::string("wrong assignment to integer");
    }
    
  } else if (node->lvalue()->is_typed(cdk::TYPE_POINTER)) {
    if (node->rvalue()->is_typed(cdk::TYPE_POINTER)) {
      auto left_name = cdk::reference_type::cast(node->lvalue()->type())->referenced()->name(); 
      auto right_name = cdk::reference_type::cast(node->rvalue()->type())->referenced()->name();
      if (left_name == cdk::TYPE_VOID || right_name == cdk::TYPE_VOID)
        node->rvalue()->type(node->lvalue()->type());
      if (right_name == cdk::TYPE_UNSPEC)
        node->rvalue()->type(node->lvalue()->type()); // it is now the ptr type of the right side
      node->type(node->rvalue()->type());
    } else if (node->rvalue()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_POINTER));
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_ERROR));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_ERROR));
    } else {
      throw std::string("wrong assignment to pointer");
    }

  } else if (node->lvalue()->is_typed(cdk::TYPE_DOUBLE)) {

    if (node->rvalue()->is_typed(cdk::TYPE_DOUBLE) || node->rvalue()->is_typed(cdk::TYPE_INT)) {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
      node->rvalue()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    } else {
      throw std::string("wrong assignment to real");
    }

  } else if (node->lvalue()->is_typed(cdk::TYPE_STRING)) {

    if (node->rvalue()->is_typed(cdk::TYPE_STRING)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
    } else if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
      node->rvalue()->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
    } else {
      throw std::string("wrong assignment to string");
    }
  
  } else if (node->lvalue()->is_typed(cdk::TYPE_TENSOR)) {
    if (node->rvalue()->is_typed(cdk::TYPE_TENSOR)) {
      node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));
    } else {
      throw std::string("wrong assignment to tensor");
    }
  
  } else {
    throw std::string("wrong types in assignment");
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_evaluation_node(udf::evaluation_node *const node, int lvl) {
  node->argument()->accept(this, lvl + 2);
}

void udf::type_checker::do_write_node(udf::write_node *const node, int lvl) {
  for (size_t i = 0; i < node->args()->size(); i++) {
    auto arg = dynamic_cast<cdk::expression_node*>(node->args()->node(i));

    arg->accept(this, lvl);
    if (arg->is_typed(cdk::TYPE_UNSPEC)) {
      arg->type(cdk::primitive_type::create(4, cdk::TYPE_INT));

    }

    if (!arg->is_typed(cdk::TYPE_INT) && !arg->is_typed(cdk::TYPE_DOUBLE) &&
        !arg->is_typed(cdk::TYPE_STRING) && !arg->is_typed(cdk::TYPE_TENSOR))
        throw std::string("wrong type for write");
  }
}

void udf::type_checker::do_input_node(udf::input_node *const node, int lvl) {
  node->type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_address_node(udf::address_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->value()->accept(this, lvl + 2);
  node->type(cdk::reference_type::create(4, node->value()->type()));
}

void udf::type_checker::do_objects_node(udf::objects_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->argument()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type in argument");
  }

  node->type(cdk::reference_type::create(4, cdk::primitive_type::create(0, cdk::TYPE_UNSPEC)));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_for_node(udf::for_node *const node, int lvl) {
  _symtab.push();
  node->init()->accept(this, lvl + 4);
  node->condition()->accept(this, lvl + 4);
  node->increment()->accept(this, lvl + 4);
  node->block()->accept(this, lvl + 4);
  _symtab.pop();
}

//---------------------------------------------------------------------------

void udf::type_checker::do_if_node(udf::if_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (!node->condition()->is_typed(cdk::TYPE_INT)) throw std::string("expected integer condition");
}

void udf::type_checker::do_if_else_node(udf::if_else_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (!node->condition()->is_typed(cdk::TYPE_INT)) throw std::string("expected integer condition");
}

//---------------------------------------------------------------------------

void udf::type_checker::do_block_node(udf::block_node * const node, int lvl) {
  /* EMPTY */
}


void udf::type_checker::do_break_node(udf::break_node * const node, int lvl) {
  /* EMPTY */
}


void udf::type_checker::do_continue_node(udf::continue_node * const node, int lvl) {
  /* EMPTY */
}

//---------------------------------------------------------------------------

void udf::type_checker::do_return_node(udf::return_node * const node, int lvl) {
  if (node->retval()) {
    if (_function->type() != nullptr) {
      auto ftype = std::static_pointer_cast<cdk::functional_type>(_function->type())->output(0);
      if (ftype->name() == cdk::TYPE_VOID)
        throw std::string("return value specified for void function");
    }
    
    node->retval()->accept(this, lvl + 2);

    // function is auto: copy type of first return expression
    if (_function->type() == nullptr) {
      _function->set_type(node->retval()->type());
      return; // simply set the type
    }

    if (_in_block_ret_type == nullptr) {
      _in_block_ret_type = node->retval()->type();
    } else {
      if (_in_block_ret_type != node->retval()->type()) {
        _function->set_type(cdk::primitive_type::create(0, cdk::TYPE_ERROR));
        throw std::string("all return statements in a function must return the same type.");
      }
    }

    auto ftype = std::static_pointer_cast<cdk::functional_type>(_function->type())->output(0);

    if (ftype->name() == cdk::TYPE_INT) {
      if (!node->retval()->is_typed(cdk::TYPE_INT)) 
        throw std::string("wrong type for initializer (integer expected).");

    } else if (ftype->name() == cdk::TYPE_DOUBLE) {
      if (!node->retval()->is_typed(cdk::TYPE_INT) && !node->retval()->is_typed(cdk::TYPE_DOUBLE))
        throw std::string("wrong type for initializer (integer or double expected).");

    } else if (ftype->name() == cdk::TYPE_STRING) {
      if (!node->retval()->is_typed(cdk::TYPE_STRING)) 
        throw std::string("wrong type for initializer (string expected).");
      
    } else if (ftype->name() == cdk::TYPE_POINTER) {
      //DAVID: FIXME: trouble!!!
      int ft = 0, rt = 0;
      while (ftype->name() == cdk::TYPE_POINTER) {
        ft++;
        ftype = cdk::reference_type::cast(ftype)->referenced();
      }
      auto rtype = node->retval()->type();
      while (rtype != nullptr && rtype->name() == cdk::TYPE_POINTER) {
        rt++;
        rtype = cdk::reference_type::cast(rtype)->referenced();
      }

      bool compatible = (ft == rt) && (rtype == nullptr || (rtype != nullptr && ftype->name() == rtype->name()));
      if (!compatible) throw std::string("wrong type for return expression (pointer expected).");

    } else if (ftype->name() == cdk::TYPE_TENSOR) {
      if (!node->retval()->is_typed(cdk::TYPE_TENSOR))
        throw std::string("wrong type for initializer (tensor expected).");

    } else {
      throw std::string("unknown type for initializer.");
    }
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_function_declaration_node(udf::function_declaration_node * const node, int lvl) {
  std::string id;

  if (node->identifier() == "udf")
    id = "_main";
  else if (node->identifier() == "_main")
    id = "._main";
  else
    id = node->identifier();

  auto symbol = udf::make_symbol(node->qualifier(), node->type(), id, false, true, true);

  std::vector < std::shared_ptr < cdk::basic_type >> argtypes;
  for (size_t ax = 0; ax < node->args()->size(); ax++)
    argtypes.push_back(node->arg(ax)->type());
  symbol->set_argument_types(argtypes);

  std::shared_ptr<udf::symbol> previous = _symtab.find(symbol->name());
  if (previous) {
    if (previous->number_of_arguments() != symbol->number_of_arguments())
      throw std::string("conflicting declaration for '" + symbol->name() + "'");

    for (size_t ax = 0; ax < symbol->number_of_arguments(); ax++) {
      if (previous->argument_type(ax) != symbol->argument_type(ax))
        throw std::string("conflicting declaration for '" + symbol->name() + "'");
    }
  } else {
    _symtab.insert(symbol->name(), symbol);
    _parent->set_new_symbol(symbol);
  }
}

void udf::type_checker::do_function_definition_node(udf::function_definition_node * const node, int lvl) {
  std::string id;

  if (node->identifier() == "udf")
    id = "_main";
  else if (node->identifier() == "_main")
    id = "._main";
  else
    id = node->identifier();

  _in_block_ret_type = nullptr;

  auto symbol = udf::make_symbol(node->qualifier(), node->type(), id, false, true);

  std::vector < std::shared_ptr < cdk::basic_type >> argtypes;
  for (size_t ax = 0; ax < node->args()->size(); ax++)
    argtypes.push_back(node->arg(ax)->type());
  symbol->set_argument_types(argtypes);

  std::shared_ptr<udf::symbol> previous = _symtab.find(symbol->name());
  if (previous) {
    if (previous->qualifier() == tFORWARD || 
       (previous->is_declaration() && 
          ((previous->qualifier() == tPUBLIC && symbol->qualifier() == tPUBLIC) ||
           (previous->qualifier() == tPRIVATE && symbol->qualifier() == tPRIVATE) ))) {
      _symtab.replace(symbol->name(), symbol);
      _parent->set_new_symbol(symbol);
    } else {
      throw std::string("conflicting definition for '" + symbol->name() + "'");
    }
  } else {
    _symtab.insert(symbol->name(), symbol);
    _parent->set_new_symbol(symbol);
  }
}

void udf::type_checker::do_function_call_node(udf::function_call_node * const node, int lvl) {
  ASSERT_UNSPEC;

  const std::string &id = node->identifier();
  auto symbol = _symtab.find(id);
  if (symbol == nullptr) throw std::string("symbol '" + id + "' is undeclared.");
  if (!symbol->is_function()) throw std::string("symbol '" + id + "' is not a function.");

  std::shared_ptr<cdk::functional_type> type = cdk::functional_type::cast(symbol->type());
  node->type(type->output(0));

  if (node->args()->size() == symbol->number_of_arguments()) {
    node->args()->accept(this, lvl + 4);
    for (size_t ax = 0; ax < node->args()->size(); ax++) {
      if (node->arg(ax)->type() == symbol->argument_type(ax)) continue;
      if (symbol->argument_is_typed(ax, cdk::TYPE_DOUBLE) && node->arg(ax)->is_typed(cdk::TYPE_INT)) continue;
      throw std::string("type mismatch for argument " + std::to_string(ax + 1) + " of '" + id + "'.");
    }
  } else {
    throw std::string(
        "number of arguments in call (" + std::to_string(node->args()->size()) + ") must match declaration ("
            + std::to_string(symbol->number_of_arguments()) + ").");
  }
}

//---------------------------------------------------------------------------


void udf::type_checker::do_variable_declaration_node(udf::variable_declaration_node * const node, int lvl) {
  if (node->initializer() != nullptr) {

    node->initializer()->accept(this, lvl + 2);

    // variable is auto: copy type of initializer
    if (node->type() == nullptr) {
      node->type(node->initializer()->type()); // simply set the type
    } else {
      if (node->is_typed(cdk::TYPE_INT)) {
        if (!node->initializer()->is_typed(cdk::TYPE_INT)) 
          throw std::string("wrong type for initializer (integer expected)");

      } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
        if (!node->initializer()->is_typed(cdk::TYPE_DOUBLE) && !node->initializer()->is_typed(cdk::TYPE_INT)) 
          throw std::string("wrong type for initializer (double or integer expected)");

      } else if (node->is_typed(cdk::TYPE_STRING)) {
        if (!node->initializer()->is_typed(cdk::TYPE_STRING)) 
          throw std::string("wrong type for initializer (string expected)");

      } else if (node->is_typed(cdk::TYPE_POINTER)) {
        if (!node->initializer()->is_typed(cdk::TYPE_POINTER)) {
          auto in = (cdk::literal_node<int>*)node->initializer();
          if (in == nullptr || in->value() != 0) throw std::string("wrong type for initializer (pointer expected).");
        }
      } else if (node->is_typed(cdk::TYPE_TENSOR)) {
        if (!node->initializer()->is_typed(cdk::TYPE_TENSOR)) 
          throw std::string("wrong type for initializer (tensor expected)");
      } else {
        throw std::string("unknowkn type for initializer");
      }
    }
  }


  const std::string &id = node->identifier();
  auto symbol = udf::make_symbol(node->qualifier(), node->type(), id, (bool)node->initializer(), false);
  if (_symtab.insert(id, symbol)) {
    _parent->set_new_symbol(symbol);
  } else {
    auto previous = _symtab.find(id); // retry: forward declaration
    if (previous->qualifier() == tFORWARD) {
      _symtab.replace(id, symbol);
      _parent->set_new_symbol(symbol);
    }
    throw std::string("variable '" + id + "' already declared");
  }
}

//---------------------------------------------------------------------------

void udf::type_checker::do_sizeof_node(udf::sizeof_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->arg()->accept(this, lvl + 2);
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

//---------------------------------------------------------------------------

void udf::type_checker::do_tensor_capacity_node(udf::tensor_capacity_node * const node, int lvl) {
  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("reshape: must operate on a tensor");
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));

}

void udf::type_checker::do_tensor_contraction_node(udf::tensor_contraction_node * const node, int lvl) {
  node->left()->accept(this, lvl + 2);
  if (!node->left()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("Left value of tensor contraction node must be a tensor");
  node->right()->accept(this, lvl + 2);
  if (!node->right()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("Right value of tensor contraction node must be a tensor");
  node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));

}

void udf::type_checker::do_tensor_dim_node(udf::tensor_dim_node * const node, int lvl) {
  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("dim: must operate on a tensor");
  node->argument()->accept(this, lvl + 2);
  if (!node->argument()->is_typed(cdk::TYPE_INT))
    throw std::string("dim: argument must be an integer");
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_tensor_dims_node(udf::tensor_dims_node * const node, int lvl) {
  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("dims: must operate on a tensor");
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_tensor_index_node(udf::tensor_index_node * const node, int lvl) {
  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("index: must operate on a tensor");
  for (size_t i = 0; i < node->indexes()->size(); ++i) {
    auto elem = node->indexes()->node(i);
    if (auto expr = dynamic_cast<cdk::expression_node*>(elem)) {
      expr->accept(this, lvl + 2);
      if (!expr->is_typed(cdk::TYPE_INT))
        throw std::string("index: indexes must be integers");
    }
    else throw std::string("index: indexes must be integers");
  }
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

void udf::type_checker::do_tensor_rank_node(udf::tensor_rank_node * const node, int lvl) {
  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("rank: must operate on a tensor");
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void udf::type_checker::do_tensor_reshape_node(udf::tensor_reshape_node * const node, int lvl) {
  node->tensor()->accept(this, lvl + 2);
  if (!node->tensor()->is_typed(cdk::TYPE_TENSOR))
    throw std::string("reshape: must operate on a tensor");
  for (size_t i = 0; i < node->args()->size(); ++i) {
    auto elem = node->args()->node(i);
    if (auto expr = dynamic_cast<cdk::expression_node*>(elem)) {
      expr->accept(this, lvl + 2);
      if (!expr->is_typed(cdk::TYPE_INT))
        throw std::string("index: args must be integers");
    }
    else throw std::string("index: args must be integers");
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_TENSOR));
}
