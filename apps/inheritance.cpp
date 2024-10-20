#include <cstddef>
#define _USE_MATH_DEFINES

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>
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
    return iface::color_str[static_cast<std::size_t>(col_)];
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

  [[nodiscard]] std::string name() const override {
    return "square width " + std::to_string(width_);
  }
  [[nodiscard]] double area() const override { return width_ * width_; }
  [[nodiscard]] double width() const override { return width_; }

private:
  double width_;
};

class rectangle : public virtual iface::rectangle, public shape {
public:
  explicit rectangle(double width, double height) : width_(width), height_(height) {}

  rectangle& height(double height) override {
    height_ = height;
    return *this;
  }

  rectangle& width(double width) override {
    width_ = width;
    return *this;
  }

  rectangle& scale(double factor) override {
    width_ *= factor;
    height_ *= factor;
    return *this;
  }

  [[nodiscard]] std::string name() const override {
    return "rectangle " + std::to_string(width_) + " x " + std::to_string(height_);
  }
  [[nodiscard]] double area() const override { return width_ * height_; }
  [[nodiscard]] double width() const override { return width_; }
  [[nodiscard]] double height() const override { return height_; }

private:
  double width_;
  double height_;
};

class circle : public virtual iface::circle, public shape {
public:
  explicit circle(double radius) : radius_(radius) {}

  circle& scale(double factor) override {
    radius_ *= factor;
    return *this;
  }

  circle& radius(double radius) override {
    radius_ = radius;
    return *this;
  }

  [[nodiscard]] std::string name() const override {
    return "circle radius " + std::to_string(radius_);
  }
  [[nodiscard]] double area() const override { return M_PI * radius_ * radius_; }
  [[nodiscard]] double radius() const override { return radius_; }

private:
  double radius_;
};

} // namespace geo

int main() {

  constexpr std::size_t N              = 1'000'000;
  constexpr std::size_t number_classes = 3;

  std::vector<std::unique_ptr<geo::iface::shape>> shapes;
  shapes.reserve(N);
  for (std::size_t i = 0; i != N; ++i) {
    double len = static_cast<double>(i % 10) + 1.0;
    switch (i % number_classes) {
    case 0: {
      shapes.emplace_back(std::make_unique<geo::square>(len));
      break;
    }
    case 1: {
      shapes.emplace_back(std::make_unique<geo::rectangle>(len, len / 2.0));
      break;
    }
    case 2: {
      shapes.emplace_back(std::make_unique<geo::circle>(len));
      break;
    }
    }
    constexpr double scale_factor = 2.0;
    shapes.back()->color(static_cast<geo::iface::color>(i % 3)).scale(scale_factor);
  }

  std::cout << shapes.size() << " shapes generated\n";

  // uncomment to see how epensive a sort by area is
  // std::sort(begin(shapes), end(shapes),
  //           [](const auto& a, const auto& b) { return a->area() > b->area(); });

  long double total_area = 0.0; // need long double to avoid loss of precision
  for (auto&& s: shapes) {
    // std::cout << s->area() << "\n";
    total_area += s->area();
  }

  std::cout << "Total area = " << std::fixed << total_area << "\n";
}
