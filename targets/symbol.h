#pragma once

#include <string>
#include <memory>
#include <cdk/types/basic_type.h>

namespace udf {

  class symbol {
    std::string _name; // identifier
    int _value;

    int _qualifier; // qualifiers: public, forward, "private" (i.e., none)
    std::shared_ptr<cdk::basic_type> _type; // type (type id + type size)
    bool _initialized; // initialized?
    int _offset = 0; // 0 (zero) means global variable/function
    bool _function; // false for variables
    std::vector<std::shared_ptr<cdk::basic_type>> _argument_types;
    bool _declaration = 0; // true for function declarations

  public:
    symbol(int qualifier, std::shared_ptr<cdk::basic_type> type, const std::string &name, bool initialized,
           bool function, bool declaration = false) :
        _name(name), _value(0), _qualifier(qualifier), _type(type), _initialized(initialized), _function(
            function), _declaration(declaration) {
    }

    ~symbol() {
    }

    const std::string& name() const {
      return _name;
    }
    int value() const {
      return _value;
    }
    int value(int v) {
      return _value = v;
    }

    int qualifier() const {
      return _qualifier;
    }

    std::shared_ptr<cdk::basic_type> type() const {
      return _type;
    }
    void set_type(std::shared_ptr<cdk::basic_type> t) {
      _type = t;
    }
    bool is_typed(cdk::typename_type name) const {
      return _type->name() == name;
    }

    const std::string& identifier() const {
      return name();
    }
    bool initialized() const {
      return _initialized;
    }
    int offset() const {
      return _offset;
    }
    void set_offset(int offset) {
      _offset = offset;
    }
    bool is_function() const {
      return _function;
    }

    bool global() const {
      return _offset == 0;
    }
    bool is_variable() const {
      return !_function;
    }
    bool is_declaration() const {
      return _declaration;
    }

    void set_argument_types(const std::vector<std::shared_ptr<cdk::basic_type>> &types) {
      _argument_types = types;
    }

    bool argument_is_typed(size_t ax, cdk::typename_type name) const {
      return _argument_types[ax]->name() == name;
    }
    std::shared_ptr<cdk::basic_type> argument_type(size_t ax) const {
      return _argument_types[ax];
    }

    size_t argument_size(size_t ax) const {
      return _argument_types[ax]->size();
    }

    size_t number_of_arguments() const {
      return _argument_types.size();
    }

  };

  inline auto make_symbol(int qualifier, std::shared_ptr<cdk::basic_type> type, const std::string &name,
                          bool initialized, bool function, bool declaration = false) {
    return std::make_shared<symbol>(qualifier, type, name, initialized, function, declaration);
  }

} // udf
