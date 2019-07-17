#include <iostream>

#include "jsast/jsast.hpp"

int main() {
  jsast::generator gen;

  gen.write(jsast::ast::variable_declaration{
      jsast::ast::variable_declarator{
          jsast::ast::identifier{"value"},
          jsast::ast::new_expression{
              jsast::ast::call_expression{jsast::ast::identifier{"f"},
                                          {jsast::ast::identifier{"hello"},
                                           jsast::ast::number_literal{13}}},
              {}}},
      jsast::variable_declaration_type::var});
  gen.write(jsast::ast::labeled_statement{
      jsast::ast::identifier{"first"},
      jsast::ast::expression_statement{jsast::ast::assignment_expression{
          jsast::ast::identifier{"value"}, jsast::assignment_op::multiply,
          jsast::ast::raw_literal{"1e-30"}}}});
  gen.write(jsast::ast::if_statement{
      jsast::ast::binary_expression{
          jsast::ast::member_expression{jsast::ast::identifier{"value"},
                                        "$data"},
          jsast::binary_op::strict_not_equal,
          jsast::ast::string_literal{"Failed at \"source\"!"}},
      jsast::ast::block_statement{
          {jsast::ast::expression_statement{jsast::ast::assignment_expression{
              jsast::ast::computed_member_expression{
                  jsast::ast::identifier{"value"},
                  jsast::ast::binary_expression{jsast::ast::number_literal{9},
                                                jsast::binary_op::multiply,
                                                jsast::ast::number_literal{5}}},
              jsast::ast::binary_expression{
                  jsast::ast::binary_expression{
                      jsast::ast::binary_expression{
                          jsast::ast::number_literal{1}, jsast::binary_op::add,
                          jsast::ast::identifier{"value"}},
                      jsast::binary_op::multiply,
                      jsast::ast::identifier{"ratio"}},
                  jsast::binary_op::greater_equal,
                  jsast::ast::number_literal{5}}}}}}});
  gen.write(jsast::ast::expression_statement{jsast::ast::unary_expression{
      jsast::unary_op::delete_op, jsast::ast::identifier{"value"}}});
  gen.write(jsast::ast::variable_declaration{
      jsast::ast::variable_declarator{
          jsast::ast::array_pattern{{jsast::ast::identifier{"a"}, std::nullopt,
                                     jsast::ast::identifier{"b"}}},
          jsast::ast::array_expression{{jsast::ast::number_literal{1},
                                        jsast::ast::number_literal{2},
                                        jsast::ast::number_literal{3}}}},
      jsast::variable_declaration_type::let});
  gen.write(jsast::ast::if_statement{
      jsast::ast::identifier{"asserted"},
      jsast::ast::expression_statement{
          jsast::ast::call_expression{jsast::ast::identifier{"worker"}, {}}},
      jsast::ast::expression_statement{
          jsast::ast::call_expression{jsast::ast::identifier{"cleaner"}, {}}}});
  gen.write(jsast::ast::return_statement{jsast::ast::member_expression{
      jsast::ast::identifier{"system"}, "93: ratio"}});

  std::cout << gen.str();

  return 0;
}