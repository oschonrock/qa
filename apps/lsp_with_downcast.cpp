#include <cstddef>
#include <random>
#include <type_traits>
#include <typeinfo>
#define _USE_MATH_DEFINES

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace geo {

namespace iface {

enum class color : int { red, green, blue };
constexpr std::array<std::string_view, 3> color_str{"red", "green", "blue"};

class shape {
public:
  virtual ~shape() = default;

  virtual shape& scale(double factor) = 0;
  virtual shape& color(color color)   = 0;

  [[nodiscard]] virtual double           area() const      = 0;
  [[nodiscard]] virtual iface::color     color() const     = 0;
  [[nodiscard]] virtual std::string_view color_str() const = 0;
  [[nodiscard]] virtual std::string      name() const      = 0;
};

class square : public virtual shape {
  virtual square& width(double width) = 0;

  [[nodiscard]] virtual double width() const = 0;
};

class rectangle : public virtual shape {
  virtual rectangle& width(double width)   = 0;
  virtual rectangle& height(double height) = 0;

  [[nodiscard]] virtual double width() const  = 0;
  [[nodiscard]] virtual double height() const = 0;
};

class circle : public virtual shape {
  virtual circle& radius(double radius) = 0;

  [[nodiscard]] virtual double radius() const = 0;
};

} // namespace iface

class shape : public virtual iface::shape {
public:
  shape& color(iface::color color) override {
    col_ = color;
    return *this;
  }

  [[nodiscard]] iface::color     color() const override { return col_; }
  [[nodiscard]] std::string_view color_str() const override {
    return iface::color_str[static_cast<int>(col_)];
  }

private:
  iface::color col_ = iface::color::red;
};

class square : public virtual iface::square, public shape {
public:
  explicit square(double width) : width_(width) {}

  square& width(double width) override {
    width_ = width;
    return *this;
  }

  square& scale(double factor) override {
    width_ *= factor;
    return *this;
  }

  [[nodiscard]] std::string name() const override { return "square"; }
  [[nodiscard]] double      area() const override { return width_ * width_; }
  [[nodiscard]] double      width() const override { return width_; };

private:
  double width_;
};

class rectangle : public virtual iface::rectangle, public shape {
public:
  explicit rectangle(double width, double height) : width_(width), height_(height){};

  rectangle& height(double height) override {
    height_ = height;
    return *this;
  };

  rectangle& width(double width) override {
    width_ = width;
    return *this;
  };

  rectangle& scale(double factor) override {
    width_ *= factor;
    height_ *= factor;
    return *this;
  }

  [[nodiscard]] std::string name() const override { return "rectangle"; }
  [[nodiscard]] double      area() const override { return width_ * height_; }
  [[nodiscard]] double      width() const override { return width_; };
  [[nodiscard]] double      height() const override { return height_; };

private:
  double width_;
  double height_;
};

class circle : public virtual iface::circle, public shape {
public:
  explicit circle(double radius) : radius_(radius){};

  circle& scale(double factor) override {
    radius_ *= factor;
    return *this;
  }

  circle& radius(double radius) override {
    radius_ = radius;
    return *this;
  };

  [[nodiscard]] std::string name() const override { return "circle"; }
  [[nodiscard]] double      area() const override { return M_PI * radius_ * radius_; }
  [[nodiscard]] double      radius() const override { return radius_; };

private:
  double radius_;
};

} // namespace geo

void print(const std::vector<std::unique_ptr<geo::iface::shape>>& shapes) {
  for (auto&& s: shapes) {
    std::cout << s->name() << " width area = " << std::setprecision(5) << std::fixed << s->area()
              << "\n";
  }
}

template <typename>
struct deduce_arg_type;

template <typename Return, typename X, typename T>
struct deduce_arg_type<Return (X::*)(T) const> {
  using type = T;
};

template <typename F>
using arg_type = typename deduce_arg_type<decltype(&F::operator())>::type;

template <typename Base, typename... Fs>
void visit(Base& base, Fs&&... fs) {
  const auto attempt = [&](auto&& f) {
    using f_type = std::decay_t<decltype(f)>;
    using p_type = std::remove_reference_t<arg_type<f_type>>;

    if (auto cp = dynamic_cast<p_type*>(&base); cp != nullptr) {
      std::forward<decltype(f)>(f)(*cp);
    }
  };

  (attempt(std::forward<Fs>(fs)), ...);
}

template <typename... Base>
struct visitor : Base... {
  using Base::operator()...;
};

template <typename... T>
visitor(T...) -> visitor<T...>;

int main() {

  constexpr std::size_t N              = 1'000'000;
  constexpr std::size_t number_classes = 3;

  std::vector<std::unique_ptr<geo::iface::shape>> shapes;
  shapes.reserve(N);
  for (std::size_t i = 0; i != N; ++i) {
    double len = static_cast<double>(i % 10) + 1.0;
    switch (i % number_classes) {
    case 0: {
      shapes.push_back(std::make_unique<geo::square>(len));
      break;
    }
    case 1: {
      shapes.push_back(std::make_unique<geo::rectangle>(len, len / 2.0));
      break;
    }
    case 2: {
      shapes.push_back(std::make_unique<geo::circle>(len));
      break;
    }
    }
    constexpr double scale_factor = 2.0;
    shapes.back()->color(static_cast<geo::iface::color>(i % 3)).scale(scale_factor);
  }

  std::cout << shapes.size() << " shapes generated\n";

  double total_area = 0.0;
  for (auto&& s: shapes) total_area += s->area();
  std::cout << "Total area = " << std::fixed << total_area << "\n";

  // double total_edge_len = 0.0;
  // try {
  //   for (auto&& shape: shapes) {
  //     visit(*shape,
  //           [&](geo::square& sq) { total_edge_len += sq.width(); },
  //           [&](geo::rectangle& rect) { total_edge_len += rect.height(); },
  //           [&](geo::circle& circle) { total_edge_len += circle.radius(); });
  //   }
  //   std::cout << N << " shapes downcast. total_edge_length = " << total_edge_len;
  // } catch (const std::bad_cast& e) {
  //   std::cerr << "Bad cast!\n";
  // } catch (const std::logic_error& e) {
  //   std::cerr << e.what() << "\n";
  // }
}
