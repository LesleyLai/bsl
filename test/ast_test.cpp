#include <catch2/catch.hpp>

#include <sstream>

#include "../src/ast.hpp"

namespace beyond {

inline auto operator<<(std::ostream& os, const Value& v) -> std::ostream&
{
  std::visit([&os](auto v) { os << v; }, v);
  return os;
}

class ExprPrintVisitor : public beyond::ExprConstVisitor {
public:
  ExprPrintVisitor(std::stringstream& ss, std::size_t indentation = 0)
      : indent_{indentation}, ss_{ss}
  {
  }

  auto operator()(const ConstExpr& expr) -> void final
  {
    print_indent();
    ss_ << expr.value();
  }

  auto operator()(const LambdaExpr& expr) -> void final
  {
    print_indent();
    ss_ << "(lambda";
    for (const auto& par : expr.parameters()) {
      ss_ << ' ' << par;
    }
    ss_ << '\n';
    ExprPrintVisitor indented{ss_, indent_ + 2};
    expr.expr().accept(indented);

    ss_ << ")";
  }

  auto operator()(const ApplyExpr& expr) -> void final
  {
    print_indent();
    ss_ << '(';
    ExprPrintVisitor not_indented{ss_, 0};
    expr.func().accept(not_indented);
    for (const auto& arg : expr.arguments()) {
      ss_ << ' ';
      arg->accept(not_indented);
    }
    ss_ << ")";
  }
  auto operator()(const IdentExpr& expr) -> void final
  {
    print_indent();
    ss_ << expr.identifier();
  }

private:
  std::size_t indent_ = 0;
  std::stringstream& ss_;

  auto print_indent() const -> void
  {
    for (std::size_t i = 0; i < indent_; ++i) {
      ss_ << ' ';
    }
  }
};

} // namespace beyond

TEST_CASE("AST visiting and printing")
{
  using namespace beyond;

  using Params = std::vector<std::string>;

  SECTION("Print the expression ( x y -> z -> (x + z + y) )")
  {
    std::stringstream ss;
    ExprPrintVisitor visitor{ss};

    const auto body = []() {
      std::vector<std::unique_ptr<Expr>> body;
      body.reserve(3);
      body.emplace_back(IdentExpr::create("x"));
      body.emplace_back(IdentExpr::create("z"));
      body.emplace_back(IdentExpr::create("y"));
      return body;
    };

    const auto expr = LambdaExpr(
        Params{"x", "y"},
        LambdaExpr::create(Params{"z"},
                           ApplyExpr::create(IdentExpr::create("+"), body())));
    expr.accept(visitor);

    const auto expected = R"((lambda x y
  (lambda z
    (+ x z y))))";

    REQUIRE(ss.str() == expected);
  }
}
