#include <cstddef>
#define _USE_MATH_DEFINES
#include <random>

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
#include <variant>
#include <vector>

// liskov substitution principle
// using a multiple inheritance hierarchy to separate interface and inheritance
// https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c129-when-designing-a-class-hierarchy-distinguish-between-implementation-inheritance-and-interface-inheritance

namespace geo {
enum class color : int { red, green, blue };
constexpr std::array<std::string_view, 3> color_str{"red", "green", "blue"};

class square {
public:
  explicit square(double width) : width_(width) {}

  square& color(color color) {
    col_ = color;
    return *this;
  }

  square& width(double width) {
    width_ = width;
    return *this;
  }

  square& scale(double factor) {
    width_ *= factor;
    return *this;
  }

  [[nodiscard]] enum color       color() const { return col_; }
  [[nodiscard]] std::string_view color_str() const {
    return geo::color_str[static_cast<std::size_t>(col_)];
  }
  [[nodiscard]] static std::string name() { return "square"; }
  [[nodiscard]] double             area() const { return width_ * width_; }
  [[nodiscard]] double             width() const { return width_; };

private:
  enum color col_ = color::red;
  double     width_;
};

class rectangle {
public:
  explicit rectangle(double width, double height) : width_(width), height_(height){};

  rectangle& color(color color) {
    col_ = color;
    return *this;
  }

  rectangle& height(double height) {
    height_ = height;
    return *this;
  };

  rectangle& width(double width) {
    width_ = width;
    return *this;
  };

  rectangle& scale(double factor) {
    width_ *= factor;
    height_ *= factor;
    return *this;
  }

  [[nodiscard]] enum color       color() const { return col_; }
  [[nodiscard]] std::string_view color_str() const {
    return geo::color_str[static_cast<std::size_t>(col_)];
  }

  [[nodiscard]] static std::string name() { return "rectangle"; }
  [[nodiscard]] double             area() const { return width_ * height_; }
  [[nodiscard]] double             width() const { return width_; };
  [[nodiscard]] double             height() const { return height_; };

private:
  enum color col_ = color::red;
  double     width_;
  double     height_;
};

class circle {
public:
  explicit circle(double radius) : radius_(radius){};

  circle& color(color color) {
    col_ = color;
    return *this;
  }

  circle& scale(double factor) {
    radius_ *= factor;
    return *this;
  }

  circle& radius(double radius) {
    radius_ = radius;
    return *this;
  };

  [[nodiscard]] enum color       color() const { return col_; }
  [[nodiscard]] std::string_view color_str() const {
    return geo::color_str[static_cast<std::size_t>(col_)];
  }
  [[nodiscard]] static std::string name() { return "circle"; }
  [[nodiscard]] double             area() const { return M_PI * radius_ * radius_; }
  [[nodiscard]] double             radius() const { return radius_; };

private:
  enum color col_ = color::red;
  double     radius_;
};

} // namespace geo

using shape = std::variant<geo::square, geo::rectangle, geo::circle>;

template <typename... Base>
struct visitor : Base... {
  using Base::operator()...;
};

template <typename... T>
visitor(T...) -> visitor<T...>;

class shape_actions {
public:
  static std::string name(shape s) { return std::visit(shape_name, s); }
  static double      area(shape s) { return std::visit(shape_area, s); }

private:
  constexpr static visitor shape_name = {
      [](const geo::square& /*s*/) { return geo::square::name(); },
      [](const geo::rectangle& /*s*/) { return geo::rectangle::name(); },
      [](const geo::circle& /*s*/) { return geo::circle::name(); },
  };

  constexpr static visitor shape_area = {
      [](const geo::square& s) { return s.area(); },
      [](const geo::rectangle& s) { return s.area(); },
      [](const geo::circle& s) { return s.area(); },
  };

  // constexpr static visitor shape_color = {
  //     [](const geo::square& s) { return s.color(); },
  //     [](const geo::rectangle& s) { return s.color(); },
  //     [](const geo::circle& s) { return s.color(); },
  // };
};

void print(const std::vector<shape>& shapes) {
  for (auto&& s: shapes) {
    std::cout << shape_actions::name(s) << " area = " << std::setprecision(5) << std::fixed
              << shape_actions::area(s) << "\n";
  }
}

int main() {

  constexpr std::size_t                      N              = 1'000'000;
  constexpr std::size_t                      number_classes = 3;
  std::mt19937_64                            rgen(std::random_device{}());
  std::uniform_int_distribution<std::size_t> clsidxdist(0, number_classes - 1);

  std::vector<shape> shapes;
  shapes.reserve(N);
  for (std::size_t i = 0; i != N; ++i) {
    double len = static_cast<double>(i % 10) + 1.0;
    switch (i % 3) {
    case 0: {
      geo::square s(len);
      s.color(static_cast<geo::color>(i % 3));
      shapes.emplace_back(s);
      break;
    }
    case 1: {
      geo::rectangle s(len, len / 2.0);
      s.color(static_cast<geo::color>(i % 3));
      shapes.emplace_back(s);
      break;
    }
    case 2: {
      geo::square s(len);
      s.color(static_cast<geo::color>(i % 3));
      shapes.emplace_back(s);
      break;
    }
    }
  }

  std::cout << shapes.size() << " shapes generated\n";

  // constexpr double scale_factor = 2.0;
  double total_area = 0.0;
  for (auto&& s: shapes) {
    // std::cout << s->color_str() << " " << s->name() << "\n";
    // s->scale(scale_factor);
    total_area += shape_actions::area(s);
  }
  std::cout << "Total area = " << std::fixed << total_area << "\n";

  // double total_edge_len = 0.0;
  // try {
  //   for (auto&& shape: shapes) {
  //     visit(
  //         *shape, [&](geo::square& sq) { total_edge_len += sq.width(); },
  //         [&](geo::rectangle& rect) { total_edge_len += rect.height(); },
  //         [&](geo::circle& circle) { total_edge_len += circle.radius(); });
  //   }
  //   std::cout << N << " shapes downcast. total_edge_length = " << total_edge_len;
  // } catch (const std::bad_cast& e) {
  //   std::cerr << "Bad cast!\n";
  // } catch (const std::logic_error& e) {
  //   std::cerr << e.what() << "\n";
  // }
}
