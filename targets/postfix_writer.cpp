#include <string>
#include <sstream>
#include "targets/type_checker.h"
#include "targets/postfix_writer.h"
#include "targets/frame_size_calculator.h"
#include ".auto/all_nodes.h"  // all_nodes.h is automatically generated

#include "udf_parser.tab.h"

//---------------------------------------------------------------------------

void udf::postfix_writer::do_nil_node(cdk::nil_node * const node, int lvl) {
  // EMPTY
}
void udf::postfix_writer::do_data_node(cdk::data_node * const node, int lvl) {
  // EMPTY
}

void udf::postfix_writer::do_unless_node(udf::unless_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  int lineno = node->lineno();

  int endlbl;

  _symtab.push();

  node->condition()->accept(this, lvl + 2);
  _pf.JNZ(mklbl(endlbl = ++_lbl));

  // iterator: _it = 0
  std::string id = "_it";
  auto it = new udf::variable_declaration_node(lineno, tPRIVATE, cdk::primitive_type::create(4, cdk::TYPE_INT),
            id, new cdk::integer_node(lineno, 0));

  auto it_seq = new cdk::sequence_node(lineno, it);

  auto it_lval = new cdk::variable_node(lineno, id);
  auto it_rval = new cdk::rvalue_node(lineno, it_lval);

  // condition: _it < count
  auto comparison = new cdk::lt_node(lineno, it_rval, node->count());

  auto comp_seq = new cdk::sequence_node(lineno, comparison);

  // step: _it = _it + 1
  auto add = new cdk::add_node(lineno, it_rval, new cdk::integer_node(lineno, 1));
  auto assign = new cdk::assignment_node(lineno, it_lval, add);

  auto step_seq = new cdk::sequence_node(lineno, assign);

  // v[_it]
  auto vec_elem = new udf::index_node(lineno, node->vector(), it_rval); // lvalue
  auto vec_elem_rval = new cdk::rvalue_node(lineno, vec_elem);

  // block: f(v[_it])
  auto block = new udf::function_call_node(lineno, node->function(), new cdk::sequence_node(lineno, vec_elem_rval));

  auto for_loop = new udf::for_node(lineno, it_seq, comp_seq, step_seq, block);

  for_loop->accept(this, lvl + 2);

  _pf.ALIGN();
  _pf.LABEL(mklbl(endlbl));

  _symtab.pop();
}

void udf::postfix_writer::do_process_node(udf::process_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  int lineno = node->lineno();

  _symtab.push();

  // iterator: int _it = low
  std::string id = "_it";
  auto it = new udf::variable_declaration_node(lineno, tPRIVATE, cdk::primitive_type::create(4, cdk::TYPE_INT),
                                               id, node->low());
  
  auto it_seq = new cdk::sequence_node(lineno, it);

  auto it_lval = new cdk::variable_node(lineno, id);
  auto it_rval = new cdk::rvalue_node(lineno, it_lval);

  // condition: _it <= high
  auto comparison = new cdk::le_node(lineno, it_rval, node->high());

  auto comp_seq = new cdk::sequence_node(lineno, comparison);

  // increment: _it = _it + 1
  auto add = new cdk::add_node(lineno, it_rval, new cdk::integer_node(lineno, 1));
  auto assign = new cdk::assignment_node(lineno, it_lval, add);

  auto incr_seq = new cdk::sequence_node(lineno, assign);

  // v[_it]
  auto vec_elem = new udf::index_node(lineno, node->vector(), it_rval); // lval
  auto vec_elem_rval = new cdk::rvalue_node(lineno, vec_elem);

  // block: f(v[_it])
  auto block = new udf::function_call_node(lineno, node->function(), new cdk::sequence_node(lineno, vec_elem_rval));

  // for
  auto for_loop = new udf::for_node(lineno, it_seq, comp_seq, incr_seq, block);

  for_loop->accept(this, lvl + 2);

  _symtab.pop();

}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_sequence_node(cdk::sequence_node * const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl);
  }
}

//---------------------------------------------------------------------------
void udf::postfix_writer::do_double_node(cdk::double_node * const node, int lvl) {
  if (_in_function_body) {
    _pf.DOUBLE(node->value());  // load number to the stack
  } else {
    _pf.SDOUBLE(node->value()); // double is on the DATA segment
  }
}

void udf::postfix_writer::do_integer_node(cdk::integer_node * const node, int lvl) {
  if (_in_function_body) {
    _pf.INT(node->value());    // integer literal is on the stack: push an integer
  } else {
    _pf.SINT(node->value());  // integer literal is on the DATA segment
  }
}

void udf::postfix_writer::do_string_node(cdk::string_node * const node, int lvl) {
  int lbl1;

  /* generate the string literal */
  _pf.RODATA(); // strings are DATA readonly
  _pf.ALIGN(); // make sure we are aligned
  _pf.LABEL(mklbl(lbl1 = ++_lbl)); // give the string a name
  _pf.SSTRING(node->value()); // output string characters
  if (_function) {
    // local variable initializer
    _pf.TEXT();
    _pf.ADDR(mklbl(lbl1));
  } else {
    // global variable initializer
    _pf.DATA();
    _pf.SADDR(mklbl(lbl1));
  }
}

void udf::postfix_writer::do_nullptr_node(udf::nullptr_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (_in_function_body) {
    _pf.INT(0);
  } else {
    _pf.SINT(0);
  }
}

void udf::postfix_writer::linearize_tensor_data(cdk::sequence_node *values, std::vector<cdk::expression_node*> &flat_values) {
  for (size_t i = 0; i < values->size(); ++i) {
    auto node = values->node(i);

    if (auto inner_tensor = dynamic_cast<udf::tensor_node*>(node)) {
      linearize_tensor_data(inner_tensor->values(), flat_values);
    } else if (auto expr = dynamic_cast<cdk::expression_node*>(node)) {
      flat_values.push_back(expr);
    } else {
      std::cerr << "Linearize_tensor_data: not an expression!" << std::endl;
    }
  }
}

void udf::postfix_writer::extract_tensor_shape(cdk::sequence_node *values, std::vector<int> &shape) {
  shape.push_back(values->size());

  if (values->size() > 0) {
    auto first = dynamic_cast<udf::tensor_node*>(values->node(0));
    if (first) extract_tensor_shape(first->values(), shape);
  }
}

void udf::postfix_writer::do_tensor_node(udf::tensor_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  std::vector<cdk::expression_node*> flat_values;
  std::vector<int> shape;

  linearize_tensor_data(node->values(), flat_values);
  extract_tensor_shape(node->values(), shape);

  int total_elems = flat_values.size();
  int rank = shape.size();

  for (int i = rank - 1; i >= 0; --i) {
    _pf.INT(shape[i]);
  }
  _pf.INT(rank);
  _functions_to_declare.insert("tensor_create");
  _pf.CALL("tensor_create");
  _pf.TRASH(4 * (rank + 1));
  _pf.LDFVAL32();

  for (ssize_t i = 0; i < total_elems; ++i) {
    _pf.INT(i);
    flat_values[i]->accept(this, lvl + 2);
    if (flat_values[i]->is_typed(cdk::TYPE_INT)) _pf.I2D();
    _functions_to_declare.insert("tensor_put");
    _pf.CALL("tensor_put");
    _pf.TRASH(12);
  }

}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_unary_minus_node(cdk::unary_minus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value
  _pf.NEG(); // 2-complement
}

void udf::postfix_writer::do_unary_plus_node(cdk::unary_plus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value
}

void udf::postfix_writer::do_not_node(cdk::not_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
  _pf.INT(0);
  _pf.EQ();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_operation(cdk::binary_operation_node * const node, int lvl, int operation) {
    ASSERT_SAFE_EXPRESSIONS;

    if (node->type()->name() == cdk::TYPE_TENSOR) {
        if ((node->left()->type()->name() == cdk::TYPE_TENSOR && node->right()->type()->name() == cdk::TYPE_INT) ||
            (node->left()->type()->name() == cdk::TYPE_TENSOR && node->right()->type()->name() == cdk::TYPE_DOUBLE)) {
            node->right()->accept(this, lvl + 2);

            if (node->right()->type()->name() == cdk::TYPE_INT) _pf.I2D();

            node->left()->accept(this, lvl + 2);

            switch (operation) {
                case 0:
                  _functions_to_declare.insert("tensor_add_scalar");
                  _pf.CALL("tensor_add_scalar");
                  break;
                case 1:
                  _functions_to_declare.insert("tensor_sub_scalar");
                  _pf.CALL("tensor_sub_scalar");
                  break;
                case 2:
                  _functions_to_declare.insert("tensor_mul_scalar");
                  _pf.CALL("tensor_mul_scalar");
                  break;
                case 3:
                  _functions_to_declare.insert("tensor_div_scalar");
                  _pf.CALL("tensor_div_scalar");
            }
            _pf.TRASH(12);
            _pf.LDFVAL32();

        } else if ((node->left()->type()->name() == cdk::TYPE_INT && node->right()->type()->name() == cdk::TYPE_TENSOR) ||
                   (node->left()->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_TENSOR)) {
            auto type = cdk::tensor_type::cast(node->right()->type());    
            switch (operation) {
              case 0:
                //std::cout << "NUM, TENSOR: ADD" << std::endl;
                node->left()->accept(this, lvl + 2);
                if (node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();
                node->right()->accept(this, lvl + 2);
                _functions_to_declare.insert("tensor_add_scalar");
                _pf.CALL("tensor_add_scalar");
                _pf.TRASH(12);
                break;
              case 1:
                // std::cout << "NUM, TENSOR: SUB" << std::endl;
                node->right()->accept(this, lvl + 2);
                node->left()->accept(this, lvl + 2); 
                if (node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();
                for (int i = type->dims().size() - 1; i >= 0; --i) {
                  _pf.INT(type->dims()[i]);
                }
                _pf.INT(type->dims().size());
                _functions_to_declare.insert("tensor_ones");
                _pf.CALL("tensor_ones");
                _pf.TRASH(4 * (type->dims().size() + 1));
                _pf.LDFVAL32();
                _functions_to_declare.insert("tensor_mul_scalar");
                _pf.CALL("tensor_mul_scalar");
                _pf.TRASH(12); // double + tensor ones pointer
                _pf.LDFVAL32();

                _functions_to_declare.insert("tensor_sub");
                _pf.CALL("tensor_sub");
                _pf.TRASH(8);
                break;
              case 2:
                //std::cout << "TENSOR, NUM: MUL" << std::endl;
                node->left()->accept(this, lvl + 2);
                if (node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();
                node->right()->accept(this, lvl + 2);
                _functions_to_declare.insert("tensor_mul_scalar");
                _pf.CALL("tensor_mul_scalar");
                _pf.TRASH(12);
                break;
              case 3:
                // std::cout << "NUM, TENSOR: DIV" << std::endl;
                node->right()->accept(this, lvl + 2);

                node->left()->accept(this, lvl + 2);
                if (node->left()->type()->name() == cdk::TYPE_INT) _pf.I2D();
                for (int i = type->dims().size() - 1; i >= 0; --i) {
                  _pf.INT(type->dims()[i]);
                }
                _pf.INT(type->dims().size());
                _functions_to_declare.insert("tensor_ones");
                _pf.CALL("tensor_ones");
                _pf.TRASH(4 * (type->dims().size() + 1));
                _pf.LDFVAL32();
                _functions_to_declare.insert("tensor_mul_scalar");
                _pf.CALL("tensor_mul_scalar");
                _pf.TRASH(12); // double + tensor ones pointer
                _pf.LDFVAL32();

                _functions_to_declare.insert("tensor_div");
                _pf.CALL("tensor_div");
                _pf.TRASH(8);

            }
            _pf.LDFVAL32();

        } else if ((node->left()->type()->name() == cdk::TYPE_TENSOR && node->right()->type()->name() == cdk::TYPE_TENSOR)) {
            node->right()->accept(this, lvl + 2);
            node->left()->accept(this, lvl + 2);

            switch (operation) {
              case 0:
                _functions_to_declare.insert("tensor_add");
                _pf.CALL("tensor_add");
                break;
              case 1:
                _functions_to_declare.insert("tensor_sub");
                _pf.CALL("tensor_sub");
                break;
              case 2:
                _functions_to_declare.insert("tensor_mul");
                _pf.CALL("tensor_mul");
                break;
              case 3:
                _functions_to_declare.insert("tensor_div");
                _pf.CALL("tensor_div");
            }
            _pf.TRASH(8);
            _pf.LDFVAL32();

        }
    } else {
        node->left()->accept(this, lvl + 2);
        if (node->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) {
            _pf.I2D();
        } else if (node->type()->name() == cdk::TYPE_POINTER && node->left()->type()->name() == cdk::TYPE_INT) {
            auto ref = cdk::reference_type::cast(node->type())->referenced();
            _pf.INT(ref->size());
            _pf.MUL();
        }

        node->right()->accept(this, lvl + 2);
        if (node->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) {
            _pf.I2D();
        } else if (node->type()->name() == cdk::TYPE_POINTER && node->right()->type()->name() == cdk::TYPE_INT) {
            auto ref = cdk::reference_type::cast(node->type())->referenced();
            _pf.INT(ref->size());
            _pf.MUL();
        }

        if (node->type()->name() == cdk::TYPE_DOUBLE)
            switch (operation) {
                case 0:
                    _pf.DADD();
                    break;
                case 1:
                    _pf.DSUB();
                    
                    break;
                case 2:
                    _pf.DMUL();
                    break;
                case 3:
                    _pf.DDIV();
        } else {
            switch (operation) {
                case 0:
                    _pf.ADD();
                    break;
                case 1:
                    _pf.SUB();
                    // number of objects of the referenced type
                    if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER)) {
                      auto lref = cdk::reference_type::cast(node->left()->type());
                      if (lref->referenced()->name() == cdk::TYPE_VOID) _pf.INT(1);
                      else _pf.INT(lref->referenced()->size());
                      _pf.DIV();
                    }
                    break;
                case 2:
                    _pf.MUL();
                    break;
                case 3:
                    _pf.DIV();
            } 
        }
    }
}

void udf::postfix_writer::do_add_node(cdk::add_node * const node, int lvl) {
  do_operation(node, lvl, 0);   
}

void udf::postfix_writer::do_sub_node(cdk::sub_node * const node, int lvl) {
  do_operation(node, lvl, 1);
}

void udf::postfix_writer::do_mul_node(cdk::mul_node * const node, int lvl) {
  do_operation(node, lvl, 2);
}

void udf::postfix_writer::do_div_node(cdk::div_node * const node, int lvl) {
  do_operation(node, lvl, 3);
}

void udf::postfix_writer::do_mod_node(cdk::mod_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.MOD();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_comparative_operation(cdk::binary_operation_node *node, int lvl, int operation) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl + 2);
  if (node->right()->type()->name() == cdk::TYPE_DOUBLE && node->left()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  node->right()->accept(this, lvl + 2);
  if (node->left()->type()->name() == cdk::TYPE_DOUBLE && node->right()->type()->name() == cdk::TYPE_INT) {
    _pf.I2D();
  }

  if (node->left()->type()->name() == cdk::TYPE_TENSOR){
    switch(operation){
      case 4: // !=
        _functions_to_declare.insert("tensor_equals");
        _pf.CALL("tensor_equals");
        _pf.TRASH(8);
        _pf.LDFVAL32();
        _pf.INT(1);
        _pf.NE();
        break;
      case 5: // ==
        _functions_to_declare.insert("tensor_equals");
        _pf.CALL("tensor_equals");
        _pf.TRASH(8);
        _pf.LDFVAL32();
        _pf.INT(1);
        _pf.EQ();
        break;
    }
    return;
  } else if (node->left()->type()->name() == cdk::TYPE_DOUBLE || node->right()->type()->name() == cdk::TYPE_DOUBLE){
    _pf.DCMP();
    _pf.INT(0);
  }

  switch(operation){
    case 0: // <
      _pf.LT();
      break;
    case 1: // <=
      _pf.LE();
      break;
    case 2: // >
      _pf.GT();
      break;
    case 3: // >=
      _pf.GE();
      break;
    case 4: // !=
      _pf.NE();
      break;
    case 5: // ==
      _pf.EQ();
      break;
  }
}

void udf::postfix_writer::do_lt_node(cdk::lt_node * const node, int lvl) {
  do_comparative_operation(node, lvl, 0);
}

void udf::postfix_writer::do_le_node(cdk::le_node * const node, int lvl) {
  do_comparative_operation(node, lvl, 1);
}

void udf::postfix_writer::do_gt_node(cdk::gt_node * const node, int lvl) {
  do_comparative_operation(node, lvl, 2);
}

void udf::postfix_writer::do_ge_node(cdk::ge_node * const node, int lvl) {
  do_comparative_operation(node, lvl, 3);
}

void udf::postfix_writer::do_ne_node(cdk::ne_node * const node, int lvl) {
  do_comparative_operation(node, lvl, 4);
}

void udf::postfix_writer::do_eq_node(cdk::eq_node * const node, int lvl) {
  do_comparative_operation(node, lvl, 5);
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_and_node(cdk::and_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JZ(mklbl(lbl));   // 2nd arg is only evaluated if 1st is not false
  node->right()->accept(this, lvl);
  _pf.AND();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}

void udf::postfix_writer::do_or_node(cdk::or_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JNZ(mklbl(lbl));   // 2nd arg is only evaluated if 1st is not true
  node->right()->accept(this, lvl);
  _pf.OR();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_variable_node(cdk::variable_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  const std::string &id = node->name();
  auto symbol = _symtab.find(id);
  if (symbol->global()) {
    _pf.ADDR(symbol->name());
  } else {
    _pf.LOCAL(symbol->offset());
  }
}

void udf::postfix_writer::do_index_node(udf::index_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->base()->accept(this, lvl + 2);
  node->index()->accept(this, lvl + 2);
  _pf.INT(node->type()->size());
  _pf.MUL();
  _pf.ADD();
}

void udf::postfix_writer::do_rvalue_node(cdk::rvalue_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl);
  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.LDDOUBLE();
  } else {
    // integers, pointers, and strings
    _pf.LDINT();
  }
}

void udf::postfix_writer::do_assignment_node(cdk::assignment_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->rvalue()->accept(this, lvl + 2);
  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    if (node->rvalue()->type()->name() == cdk::TYPE_INT) _pf.I2D();
    _pf.DUP64();
  } else {
    _pf.DUP32();
  }

  node->lvalue()->accept(this, lvl);
  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.STDOUBLE();
  } else {
    _pf.STINT();
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_evaluation_node(udf::evaluation_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl);
  auto typeSize = node->argument()->type()->size();
  if (typeSize > 0) {
    _pf.TRASH(typeSize); // deletes evaluated value
  }
}

void udf::postfix_writer::do_write_node(udf::write_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  for (size_t ax = 0; ax < node->args()->size(); ax++) {
    auto arg = dynamic_cast<cdk::expression_node*>(node->args()->node(ax));

    arg->accept(this, lvl); // expression to write

    if (arg->is_typed(cdk::TYPE_INT)) {
      _functions_to_declare.insert("printi");
      _pf.CALL("printi");
      _pf.TRASH(4); // delete the written int
    } else if (arg->is_typed(cdk::TYPE_DOUBLE)) {
      _functions_to_declare.insert("printd");
      _pf.CALL("printd");
      _pf.TRASH(8); // delete the written double
    } else if (arg->is_typed(cdk::TYPE_STRING)) {
      _functions_to_declare.insert("prints");
      _pf.CALL("prints");
      _pf.TRASH(4); // delete the written value's address
    } else if (arg->is_typed(cdk::TYPE_TENSOR)) {
      _functions_to_declare.insert("tensor_print");
      _pf.CALL("tensor_print");
      _pf.TRASH(4); // delete the written tensor
    }
     else {
      std::cerr << "cannot happen!" << std::endl;
    }
    
  }
  if (node->newline()) {
    _functions_to_declare.insert("println");
    _pf.CALL("println");
  }
}

void udf::postfix_writer::do_input_node(udf::input_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _functions_to_declare.insert("inputd");
    _pf.CALL("inputd");
    _pf.LDFVAL64();
  } else {
    _functions_to_declare.insert("inputi");
    _pf.CALL("inputi");
    _pf.LDFVAL32();
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_address_node(udf::address_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->value()->accept(this, lvl + 2);
}

void udf::postfix_writer::do_objects_node(udf::objects_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto ref = cdk::reference_type::cast(node->type())->referenced();
  node->argument()->accept(this, lvl);

  if (ref->name() == cdk::TYPE_VOID) _pf.INT(1);
  else _pf.INT(ref->size());

  _pf.MUL();
  _pf.ALLOC();
  _pf.SP();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_for_node(udf::for_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  
  _for_init.push(++_lbl); // after init, before body
  _for_incr.push(++_lbl); // after intruction
  _for_end.push(++_lbl);  // after for

  _symtab.push();
  _in_for_init = true;

  node->init()->accept(this, lvl + 2);

  _pf.ALIGN();
  _pf.LABEL(mklbl(_for_init.top()));
  
  node->condition()->accept(this, lvl + 2);
  _pf.JZ(mklbl(_for_end.top()));

  node->block()->accept(this, lvl + 2);

  _pf.ALIGN();

  _pf.LABEL(mklbl(_for_incr.top()));
  node->increment()->accept(this, lvl + 2);
  _pf.JMP(mklbl(_for_init.top()));

  _pf.ALIGN();
  _pf.LABEL(mklbl(_for_end.top()));

  _in_for_init = false;
  _symtab.pop();

  _for_init.pop();
  _for_incr.pop();
  _for_end.pop();
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_if_node(udf::if_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl_end;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl_end = ++_lbl));
  node->block()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl_end));
}

void udf::postfix_writer::do_if_else_node(udf::if_else_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl_else, lbl_end;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl_else = lbl_end = ++_lbl));
  node->thenblock()->accept(this, lvl + 2);
  if (node->elseblock()) {
    _pf.JMP(mklbl(lbl_end = ++_lbl));
    _pf.LABEL(mklbl(lbl_else));
    node->elseblock()->accept(this, lvl + 2);
  }
  _pf.LABEL(mklbl(lbl_end));
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_block_node(udf::block_node * const node, int lvl) {
  _symtab.push(); // for block-local vars
  if (node->declarations()) node->declarations()->accept(this, lvl + 2);
  if (node->instructions()) node->instructions()->accept(this, lvl + 2);
  _symtab.pop();
}


void udf::postfix_writer::do_break_node(udf::break_node * const node, int lvl) {
  if (_for_init.size() != 0) {
    _pf.JMP(mklbl(_for_end.top())); // jump to for end
  } else
    error(node->lineno(), "'break' outside 'for'");
}

void udf::postfix_writer::do_continue_node(udf::continue_node * const node, int lvl) {
  if (_for_init.size() != 0) {
    _pf.JMP(mklbl(_for_incr.top())); // jump to next cycle
  } else
    error(node->lineno(), "'break' outside 'for'");
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_return_node(udf::return_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto ftype = std::static_pointer_cast<cdk::functional_type>(_function->type())->output(0);

  if (ftype->name() != cdk::TYPE_VOID) {
    node->retval()->accept(this, lvl + 2);

    if (ftype->name() == cdk::TYPE_INT || ftype->name() == cdk::TYPE_STRING ||
        ftype->name() == cdk::TYPE_POINTER || ftype->name() == cdk::TYPE_TENSOR) {
      _pf.STFVAL32();
    } else if (ftype->name() == cdk::TYPE_DOUBLE) {
      if (node->retval()->type()->name() == cdk::TYPE_INT) _pf.I2D();
      _pf.STFVAL64();
    }
    else {
      std::cerr << node->lineno() << ": should not happen: unknown return type" << std::endl;
    }
  }

  _pf.JMP(_current_body_ret_lbl);
  _return_seen = true;
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_function_declaration_node(udf::function_declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (_in_function_body || _in_function_args) {
    error(node->lineno(), "cannot declare function in body or in args");
    return;
  }

  if (!new_symbol()) return;

  auto function = new_symbol();
  _functions_to_declare.insert(function->name());
  reset_new_symbol();
}

void udf::postfix_writer::do_function_definition_node(udf::function_definition_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (_in_function_body || _in_function_args) {
    error(node->lineno(), "cannot define function in body or in args");
    return;
  }

  _function = new_symbol();
  _functions_to_declare.erase(_function->name());
  reset_new_symbol();
  
  _current_body_ret_lbl = mklbl(++_lbl);

  _offset = 8;
  _symtab.push();

  if (node->args()->size() > 0) {
    _in_function_args = true;
    for (size_t ax = 0; ax < node->args()->size(); ax++) {
      cdk::basic_node *arg = node->args()->node(ax);
      if (arg == nullptr) break; // empty sequence of args
      arg->accept(this, 0);
    }
    _in_function_args = false;
  }

  _pf.TEXT();
  _pf.ALIGN();
  if (node->qualifier() == tPUBLIC)
    _pf.GLOBAL(_function->name(), _pf.FUNC());
  _pf.LABEL(_function->name());

  if (_function->name() == "_main") {
    _functions_to_declare.insert("mem_init");
    _pf.CALL("mem_init");
  }

  // compute stack size to be reserved for local variables
  frame_size_calculator lsc(_compiler, _symtab, _function);
  node->accept(&lsc, lvl);
  _pf.ENTER(lsc.localsize()); // total stack size reserved for local variables


  _offset = 0;

  _in_function_body = true;

  node->block()->accept(this, lvl + 4);

  _in_function_body = false;
  _return_seen = false;

  _pf.LABEL(_current_body_ret_lbl);
  _pf.LEAVE();
  _pf.RET();

  _symtab.pop();

  if (node->identifier() == "udf") {
    for (std::string s : _functions_to_declare)
      _pf.EXTERN(s);
  }
    
}

void udf::postfix_writer::do_function_call_node(udf::function_call_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  auto symbol = _symtab.find(node->identifier());

  size_t argsSize = 0;
  if (node->args()->size() > 0) {
    for (int ax = node->args()->size() - 1; ax >= 0; ax--) {
      cdk::expression_node *arg = dynamic_cast<cdk::expression_node*>(node->args()->node(ax));
      arg->accept(this, lvl + 2);
      if (symbol->argument_is_typed(ax, cdk::TYPE_DOUBLE) && arg->is_typed(cdk::TYPE_INT)) {
        _pf.I2D();
      }
      argsSize += symbol->argument_size(ax);
    }
  }

  _pf.CALL(node->identifier());
  if (argsSize != 0)
    _pf.TRASH(argsSize);  

  if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_POINTER) 
      || node->is_typed(cdk::TYPE_STRING) || node->is_typed(cdk::TYPE_TENSOR)) {
    _pf.LDFVAL32();
  } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.LDFVAL64();
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_variable_declaration_node(udf::variable_declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto id = node->identifier();

  int offset = 0;
  int typesize = node->type()->size(); // in bytes

  if (_in_function_body) {
    _offset -= typesize;
    offset = _offset;
  } else if (_in_function_args) {
    offset = _offset;
    _offset += typesize;
  } else {
    offset = 0; // global
  }

  auto symbol = new_symbol();
  if (symbol) {
    symbol->set_offset(offset);
    reset_new_symbol();
  }

  if (_in_function_body) {
    if (node->initializer()) {
      node->initializer()->accept(this, lvl);
      if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_STRING) || 
          node->is_typed(cdk::TYPE_POINTER) || node->is_typed(cdk::TYPE_TENSOR)) {
        _pf.LOCAL(symbol->offset());
        _pf.STINT();
      } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
        if (node->initializer()->is_typed(cdk::TYPE_INT)) _pf.I2D();
        _pf.LOCAL(symbol->offset());
        _pf.STDOUBLE();
      }
    } else if (node->initializer() == nullptr) {
      if (node->is_typed(cdk::TYPE_TENSOR)){
          auto type = cdk::tensor_type::cast(node->type());    
          for (int i = type->dims().size() - 1; i >= 0; --i) {
            _pf.INT(type->dims()[i]);
          }
          _pf.INT(type->dims().size());
          _functions_to_declare.insert("tensor_zeros");
          _pf.CALL("tensor_zeros");
          _pf.TRASH(4 * (type->dims().size() + 1));
          _pf.LDFVAL32();
          _pf.LOCAL(symbol->offset());
          _pf.STINT();
      }
    }
  } else {
    if (!_function) {
      if (node->initializer() == nullptr) {
        if (node->is_typed(cdk::TYPE_TENSOR)){
          _pf.DATA();
          _pf.ALIGN();
          if (node->qualifier() == tPUBLIC) _pf.GLOBAL(symbol->name(), _pf.OBJ());
          _pf.LABEL(id);
          auto type = cdk::tensor_type::cast(node->type());    
          for (int i = type->dims().size() - 1; i >= 0; --i) {
            _pf.INT(type->dims()[i]);
          }
          _pf.INT(type->dims().size());
          _functions_to_declare.insert("tensor_zeros");
          _pf.CALL("tensor_zeros");
          _pf.TRASH(4 * (type->dims().size() + 1));
          _pf.LDFVAL32();
          _pf.ADDR(symbol->name());
          _pf.STINT();
        } else {
          _pf.BSS();
          _pf.ALIGN();
          if (node->qualifier() == tPUBLIC) _pf.GLOBAL(symbol->name(), _pf.OBJ());
          _pf.LABEL(id);
          _pf.SALLOC(typesize);
        }

      } else {
        _pf.DATA();
        _pf.ALIGN();
        if (node->qualifier() == tPUBLIC) _pf.GLOBAL(symbol->name(), _pf.OBJ());
        _pf.LABEL(id);
        
        if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_POINTER) 
            || node->is_typed(cdk::TYPE_STRING) || node->is_typed(cdk::TYPE_TENSOR)) {
              node->initializer()->accept(this, lvl);

        } else if (node->is_typed(cdk::TYPE_DOUBLE)) {

          if (node->initializer()->is_typed(cdk::TYPE_DOUBLE)) {
            node->initializer()->accept(this, lvl);

          } else if (node->initializer()->is_typed(cdk::TYPE_INT)) {
            cdk::integer_node *dclini = dynamic_cast<cdk::integer_node*>(node->initializer());
            cdk::double_node ddi(dclini->lineno(), dclini->value());
            ddi.accept(this, lvl);

          } else {
            std::cerr << node->lineno() << ": '" << id << "' has bad initializer for real value\n";
            _errors = true;
          }
        }
      }
    }
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_sizeof_node(udf::sizeof_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->arg()->accept(this, lvl + 2);
  if (node->arg()->is_typed(cdk::TYPE_TENSOR)) {
    _functions_to_declare.insert("tensor_size");
    _pf.CALL("tensor_size");
    _pf.LDFVAL32();
    _pf.INT(8); // each tensor cell is a double
    _pf.MUL();
  } else {
    _pf.TRASH(node->arg()->type()->size());
    _pf.INT(node->arg()->type()->size());
  }
}

//---------------------------------------------------------------------------

void udf::postfix_writer::do_tensor_capacity_node(udf::tensor_capacity_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_size");
  _pf.CALL("tensor_size");
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_contraction_node(udf::tensor_contraction_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->right()->accept(this, lvl + 2);
  node->left()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_matmul");
  _pf.CALL("tensor_matmul");
  _pf.TRASH(8);
  _pf.LDFVAL32(); 
}

void udf::postfix_writer::do_tensor_dim_node(udf::tensor_dim_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->argument()->accept(this, lvl + 2);
  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_get_dim_size");
  _pf.CALL("tensor_get_dim_size");
  _pf.TRASH(8);
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_dims_node(udf::tensor_dims_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_get_dims");
  _pf.CALL("tensor_get_dims");
  _pf.TRASH(4);
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_index_node(udf::tensor_index_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->indexes()->accept(this, lvl + 2);
  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_getptr");
  _pf.CALL("tensor_getptr");
  _pf.TRASH(4 + 4 * node->indexes()->size());
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_rank_node(udf::tensor_rank_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_get_n_dims");
  _pf.CALL("tensor_get_n_dims");
  _pf.TRASH(4);
  _pf.LDFVAL32();
}

void udf::postfix_writer::do_tensor_reshape_node(udf::tensor_reshape_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  for (int i = node->args()->size() - 1; i >= 0; --i) {
    node->args()->node(i)->accept(this, lvl + 2);
  }
  _pf.INT(node->args()->size());
  node->tensor()->accept(this, lvl + 2);
  _functions_to_declare.insert("tensor_reshape");
  _pf.CALL("tensor_reshape");
  _pf.TRASH(4 + 4 + 4 * node->args()->size());
  _pf.LDFVAL32();
}
