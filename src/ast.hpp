#ifndef BEYOND_SHADING_LANGUAGE_AST_HPP
#define BEYOND_SHADING_LANGUAGE_AST_HPP

#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <type_traits>

#include <iostream>

namespace beyond {

using Value = std::variant<int>;

class ConstExpr;
class LambdaExpr;
class ApplyExpr;
class IdentExpr;

/// @brief Provides a wrapper of `std::make_unique` to its derived classes
template <typename Derived> struct FactoryMixin {
  /**
   * @brief A factory member function that creates a `std::unique_ptr` of itself
   */
  template <typename... Args>
  static auto create(Args&&... args) -> std::unique_ptr<Derived>
  {
    return std::make_unique<Derived>(std::forward<Args>(args)...);
  }
};

enum class IsConst {
  yes,
  no,
};

template <IsConst is_const> struct ExprVisitorBase {
  ExprVisitorBase() = default;
  virtual ~ExprVisitorBase() = default;
  ExprVisitorBase(const ExprVisitorBase&) = default;
  ExprVisitorBase& operator=(const ExprVisitorBase&) = default;
  ExprVisitorBase(ExprVisitorBase&&) = default;
  ExprVisitorBase& operator=(ExprVisitorBase&&) = default;

  template <typename Expr>
  using Ref = std::conditional_t<is_const == IsConst::yes, const Expr&, Expr&>;

  virtual auto operator()(Ref<ConstExpr> expr) -> void = 0;
  virtual auto operator()(Ref<LambdaExpr> expr) -> void = 0;
  virtual auto operator()(Ref<ApplyExpr> expr) -> void = 0;
  virtual auto operator()(Ref<IdentExpr> expr) -> void = 0;
};

struct ExprVisitor : ExprVisitorBase<IsConst::no> {
  using ExprVisitorBase::ExprVisitorBase;
};

struct ExprConstVisitor : ExprVisitorBase<IsConst::yes> {
  using ExprVisitorBase::ExprVisitorBase;
};

struct Expr {
  Expr() = default;
  virtual ~Expr() = default;
  Expr(const Expr&) = delete;
  auto operator=(const Expr&) -> Expr& = delete;
  Expr(Expr&&) = default;
  auto operator=(Expr &&) -> Expr& = default;

  virtual auto accept(ExprConstVisitor& visitor) const -> void = 0;
  virtual auto accept(ExprVisitor& visitor) -> void = 0;
};

class ConstExpr final : public Expr, public FactoryMixin<ConstExpr> {
public:
  explicit ConstExpr(Value value) : value_{std::move(value)} {}

  [[nodiscard]] auto value() const noexcept -> const Value&
  {
    return value_;
  }

  auto accept(ExprVisitor& visitor) -> void final
  {
    visitor(*this);
  }

  auto accept(ExprConstVisitor& visitor) const -> void final
  {
    visitor(*this);
  }

private:
  Value value_;
};

class LambdaExpr final : public Expr, public FactoryMixin<LambdaExpr> {
public:
  LambdaExpr(std::vector<std::string> parameters, std::unique_ptr<Expr> expr)
      : parameters_{std::move(parameters)}, expr_{std::move(expr)}
  {
  }

  [[nodiscard]] auto parameters() const noexcept
      -> const std::vector<std::string>&
  {
    return parameters_;
  }

  [[nodiscard]] auto expr() const noexcept -> const Expr&
  {
    return *expr_;
  }

  auto accept(ExprVisitor& visitor) -> void final
  {
    visitor(*this);
  }

  auto accept(ExprConstVisitor& visitor) const -> void final
  {
    visitor(*this);
  }

private:
  std::vector<std::string> parameters_;
  std::unique_ptr<Expr> expr_;
};

class ApplyExpr final : public Expr, public FactoryMixin<ApplyExpr> {
public:
  explicit ApplyExpr(std::unique_ptr<Expr> func,
                     std::vector<std::unique_ptr<Expr>> argument)
      : func_{std::move(func)}, arguments_{std::move(argument)}
  {
  }

  [[nodiscard]] auto func() const noexcept -> const Expr&
  {
    return *func_;
  }

  [[nodiscard]] auto arguments() const noexcept
      -> const std::vector<std::unique_ptr<Expr>>&
  {
    return arguments_;
  }

  auto accept(ExprVisitor& visitor) -> void final
  {
    visitor(*this);
  }

  auto accept(ExprConstVisitor& visitor) const -> void final
  {
    visitor(*this);
  }

private:
  std::unique_ptr<Expr> func_;
  std::vector<std::unique_ptr<Expr>> arguments_;
};

class IdentExpr final : public Expr, public FactoryMixin<IdentExpr> {
public:
  explicit IdentExpr(std::string identifier) : identifier_{identifier} {}

  auto identifier() const -> std::string
  {
    return identifier_;
  }

  auto accept(ExprVisitor& visitor) -> void final
  {
    visitor(*this);
  }

  auto accept(ExprConstVisitor& visitor) const -> void final
  {
    visitor(*this);
  }

private:
  std::string identifier_;
};

} // namespace beyond

#endif // BEYOND_SHADING_LANGUAGE_AST_HPP
