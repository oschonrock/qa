#define _USE_MATH_DEFINES

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

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
    return geo::color_str[static_cast<int>(col_)];
  }
  [[nodiscard]] std::string name() const { return "square width " + std::to_string(width_); }
  [[nodiscard]] double      area() const { return width_ * width_; }
  [[nodiscard]] double      width() const { return width_; };

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
    return geo::color_str[static_cast<int>(col_)];
  }

  [[nodiscard]] std::string name() const {
    return "rectangle " + std::to_string(width_) + " x " + std::to_string(height_);
  }
  [[nodiscard]] double area() const { return width_ * height_; }
  [[nodiscard]] double width() const { return width_; };
  [[nodiscard]] double height() const { return height_; };

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
    return geo::color_str[static_cast<int>(col_)];
  }
  [[nodiscard]] std::string name() const { return "circle radius " + std::to_string(radius_); }
  [[nodiscard]] double      area() const { return M_PI * radius_ * radius_; }
  [[nodiscard]] double      radius() const { return radius_; };

private:
  enum color col_ = color::red;
  double     radius_;
};

template <std::size_t sbo_size = 32>
class shape {
public:
  shape() = delete;

  template <typename T>
  explicit shape(T obj) { // NOLINT storage not initialised
    static_assert(sizeof(model<T>) <= sizeof(storage), "too big");
    static_assert(alignof(model<T>) <= alignof(double), "badly aligned");
    new (&storage) model<T>(std::move(obj)); // manually init with placement new
  }
  ~shape() { object().~conc(); } // manually destroy;

  // it's a value type, default everything else (could be ommitted?)
  shape(const shape& other)     = default;
  shape(shape&& other) noexcept = default;
  shape& operator=(const shape& other) = default;
  shape& operator=(shape&& other) noexcept = default;

  [[nodiscard]] std::string name() const { return cobject().name(); }
  [[nodiscard]] double      area() const { return cobject().area(); }

  shape& scale(double scale_factor) {
    object().scale(scale_factor);
    return *this; // for fluid API
  }

  shape& color(color color) {
    object().color(color);
    return *this;
  }

private:
  struct conc {
    virtual ~conc() = default;

    [[nodiscard]] virtual std::string name() const = 0;
    [[nodiscard]] virtual double      area() const = 0;

    virtual void scale(double scale_factor) = 0;
    virtual void color(enum color col)      = 0;
  };

  template <typename T>
  struct model : conc {
    explicit model(const T& t) : obj_(t) {}
    [[nodiscard]] std::string name() const override { return obj_.name(); }
    [[nodiscard]] double      area() const override { return obj_.area(); }

    void scale(double scale_factor) override { obj_.scale(scale_factor); };
    void color(enum color col) override { obj_.color(col); };

  private:
    T obj_;
  };

  alignas(double) std::byte storage[sbo_size]; // NOLINT c-array

  conc& object() { return *reinterpret_cast<conc*>(&storage); } // NOLINT rein_cast
  [[nodiscard]] const conc& cobject() const {
    return *reinterpret_cast<const conc*>(&storage); // NOLINT rein_cast
  }
};

} // namespace geo

int main() {
  constexpr std::size_t N              = 1'000'000;
  constexpr std::size_t number_classes = 3;

  std::vector<geo::shape<>> shapes;
  shapes.reserve(N);
  for (std::size_t i = 0; i != N; ++i) {
    double len = static_cast<double>(i % 10) + 1.0;
    switch (i % number_classes) {
    case 0: {
      shapes.emplace_back(geo::square(len));
      break;
    }
    case 1: {
      shapes.emplace_back(geo::rectangle(len, len / 2.0));
      break;
    }
    case 2: {
      shapes.emplace_back(geo::circle(len));
      break;
    }
    }
    // use common fluid API
    constexpr double scale_factor = 2.0;
    shapes.back().color(static_cast<geo::color>(i % 3)).scale(scale_factor);
  }

  std::cout << shapes.size() << " shapes generated\n";

  // uncomment to see how epensive a sort by area is
  // std::sort(begin(shapes), end(shapes),
  //           [](const auto& a, const auto& b) { return a.area() > b.area(); });

  long double total_area = 0.0; // need long double to avoid loss of precision
  for (auto&& s: shapes) total_area += s.area();

  std::cout << "Total area = " << std::fixed << total_area << "\n";
}
