#define _USE_MATH_DEFINES

#include "dyno.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>

using namespace dyno::literals; // NOLINT

struct shape;
enum class color : int { red, green, blue };
constexpr std::array<std::string_view, 3> color_str{"red", "green", "blue"};

// Define the interface of something on which we can call 'area()'.
struct Shape : decltype(dyno::requires_(
                   "area"_s = dyno::method<double() const>, "scale"_s = dyno::method<void(double)>,
                   "color"_s = dyno::method<void(enum color)>, dyno::CopyConstructible{},
                   dyno::MoveConstructible{}, dyno::Destructible{})) {};

// Define how concrete types can fulfill that interface
template <typename T>
auto const dyno::default_concept_map<Shape, T> = dyno::make_concept_map(
    "area"_s  = [](T const& self) { return self.area(); },
    "scale"_s = [](T& self, double sf) { self.scale(sf); },
    "color"_s = [](T& self, enum color sf) { self.color(sf); });

// Define an object that can hold anything on which we can call 'area()'.
struct shape {
  template <typename T>
  explicit shape(T x) : poly_{x} {}

  [[nodiscard]] double area() const { return poly_.virtual_("area"_s)(); }
  shape&               scale(double sf) {
    poly_.virtual_("scale"_s)(sf);
    return *this; // fluid API
  }
  shape& color(enum color col) {
    poly_.virtual_("color"_s)(col);
    return *this; // fluid API
  }

private:
  dyno::poly<Shape, dyno::local_storage<24>> poly_;
};

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
  [[nodiscard]] std::string_view color_str() const { return ::color_str[static_cast<int>(col_)]; }
  [[nodiscard]] std::string      name() const { return "square width " + std::to_string(width_); }
  [[nodiscard]] double           area() const { return width_ * width_; }
  [[nodiscard]] double           width() const { return width_; }

private:
  enum color col_ = color::red;
  double     width_;
};

class rectangle {
public:
  explicit rectangle(double width, double height) : width_(width), height_(height) {}

  rectangle& color(color color) {
    col_ = color;
    return *this;
  }

  rectangle& height(double height) {
    height_ = height;
    return *this;
  }

  rectangle& width(double width) {
    width_ = width;
    return *this;
  }

  rectangle& scale(double factor) {
    width_ *= factor;
    height_ *= factor;
    return *this;
  }

  [[nodiscard]] enum color       color() const { return col_; }
  [[nodiscard]] std::string_view color_str() const { return ::color_str[static_cast<int>(col_)]; }

  [[nodiscard]] std::string name() const {
    return "rectangle " + std::to_string(width_) + " x " + std::to_string(height_);
  }
  [[nodiscard]] double area() const { return width_ * height_; }
  [[nodiscard]] double width() const { return width_; }
  [[nodiscard]] double height() const { return height_; }

private:
  enum color col_ = color::red;
  double     width_;
  double     height_;
};

class circle {
public:
  explicit circle(double radius) : radius_(radius) {}

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
  }

  [[nodiscard]] enum color       color() const { return col_; }
  [[nodiscard]] std::string_view color_str() const { return ::color_str[static_cast<int>(col_)]; }
  [[nodiscard]] std::string      name() const { return "circle radius " + std::to_string(radius_); }
  [[nodiscard]] double           area() const { return M_PI * radius_ * radius_; }
  [[nodiscard]] double           radius() const { return radius_; }

private:
  enum color col_ = color::red;
  double     radius_;
};

int main() {
  constexpr std::size_t N              = 1'000'000;
  constexpr std::size_t number_classes = 3;

  std::vector<shape> shapes;
  shapes.reserve(N);
  for (std::size_t i = 0; i != N; ++i) {
    double len = static_cast<double>(i % 10) + 1.0;
    switch (i % number_classes) {
    case 0: {
      shapes.emplace_back(square(len));
      break;
    }
    case 1: {
      shapes.emplace_back(rectangle(len, len / 2.0));
      break;
    }
    case 2: {
      shapes.emplace_back(circle(len));
      break;
    }
    }
    // use common fluid API
    constexpr double scale_factor = 2.0;
    shapes.back().color(static_cast<color>(i % 3)).scale(scale_factor);
  }

  std::cout << shapes.size() << " shapes generated\n";

  // uncomment to see how epensive a sort by area is
  // std::sort(begin(shapes), end(shapes),
  //           [](const auto& a, const auto& b) { return a.area() > b.area(); });

  long double total_area = 0.0; // need long double to avoid loss of precision
  for (auto&& s: shapes) total_area += s.area();

  std::cout << "Total area = " << std::fixed << total_area << "\n";
}
