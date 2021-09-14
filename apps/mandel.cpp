#include "os/bch.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <complex>
#include <cstdint>
#include <execution>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

// These are repeated throughout. Little point templating everything.
using PointType   = std::uint32_t;
using ComplexType = double;

struct Size {
  PointType      width;
  PointType      height;
  constexpr bool operator==(const Size& o) const { return width == o.width && height == o.height; }
  constexpr bool operator!=(const Size& o) const { return !(*this == o); }
};

struct Point {
  PointType x;
  PointType y;
};

struct Settings {
  Size size{640U, 640U};
  // std::complex<ComplexType> center{-0.546875298168001337L, -0.621118118440520923L};
  // ComplexType               x_scale = 3.31055062663404068e-17;
  // ComplexType               y_scale = 3.31055062663404068e-17;
  std::complex<ComplexType> center{-0.5, 0.0};
  ComplexType               x_scale   = 2.5; // complex size from left to right
  ComplexType               y_scale   = 2.5;
  bool                      first_run = false;

  static constexpr unsigned max_iter_start = 400;
  static constexpr unsigned max_iter_inc   = 200;
  static constexpr unsigned max_iter_max   = 2000;

  static constexpr int min_loop_ms = 20;

  explicit Settings(bool first_run_ = false) : first_run(first_run_) {}

  constexpr bool operator==(const Settings& o) const {
    return size == o.size && center == o.center && x_scale == o.x_scale && y_scale == o.y_scale &&
           first_run == o.first_run;
  }
  constexpr bool operator!=(const Settings& o) const { return !(*this == o); }

  [[nodiscard]] ComplexType get_x_min() const { return center.real() - x_scale / 2.0; }
  [[nodiscard]] ComplexType get_x_max() const { return center.real() + x_scale / 2.0; }
  [[nodiscard]] ComplexType get_y_min() const { return center.imag() - y_scale / 2.0; }
  [[nodiscard]] ComplexType get_y_max() const { return center.imag() + y_scale / 2.0; }

  [[nodiscard]] ComplexType get_x_step(bool shift = false) const {
    return x_scale / size.width * (shift ? 10.0 : 1.0);
  }
  [[nodiscard]] ComplexType get_y_step(bool shift = false) const {
    return y_scale / size.width * (shift ? 10.0 : 1.0);
  }

  using CT = std::complex<ComplexType>;
  void move_left(bool shift) { center -= CT{get_x_step(shift), 0.0}; }
  void move_right(bool shift) { center += CT{get_x_step(shift), 0.0}; }
  void move_up(bool shift) { center -= CT{0.0, get_y_step(shift)}; }
  void move_down(bool shift) { center += CT{0.0, get_y_step(shift)}; }

  void scale(ComplexType factor) {
    x_scale *= factor;
    y_scale *= factor;
  }
};

template <typename InType, typename OutType>
OutType map(InType val, InType in_min, InType in_max, OutType out_min, OutType out_max) {
  return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

std::complex<ComplexType> make_cpoint(Point p, const Settings& s) {
  using PT = PointType;
  return std::complex<ComplexType>(
      map(p.x, PT{0}, static_cast<PT>(s.size.width - 1), s.get_x_min(), s.get_x_max()),
      map(p.y, PT{0}, static_cast<PT>(s.size.height - 1), s.get_y_min(), s.get_y_max()));
}

sf::Color make_color(unsigned iterations, std::complex<ComplexType> current) {
  using std::log, std::abs, std::floor;
  constexpr ComplexType power = 2.0;

  // https://en.wikipedia.org/wiki/Plotting_algorithms_for_the_Mandelbrot_set#Continuous_(smooth)_coloring
  const auto value =
      (iterations + 1) - (log(log(abs(current.real() * current.imag())))) / log(power);

  // color palette
  const auto colorval = abs(static_cast<int>(floor(value * 10.0)));

  const auto colorband = colorval % (256 * 7) / 256;
  const auto mod256    = colorval % 256;
  const auto to_1      = mod256 / 255.0;
  const auto to_0      = 1.0 - to_1;

  auto to_sf_color = [](ComplexType r, ComplexType g, ComplexType b) -> sf::Color {
    const auto to_8bit = [](const auto& f) { return static_cast<std::uint8_t>(floor(f * 255)); };
    return sf::Color(to_8bit(r), to_8bit(g), to_8bit(b));
  };

  switch (colorband) {
  case 0:
    return to_sf_color(to_1, 0.0, 0.0);
  case 1:
    return to_sf_color(1.0, to_1, 0.0);
  case 2:
    return to_sf_color(to_0, 1.0, 0.0);
  case 3:
    return to_sf_color(0.0, 1.0, to_1);
  case 4:
    return to_sf_color(0.0, to_0, 1.0);
  case 5:
    return to_sf_color(to_1, 0.0, 1.0);
  case 6:
    return to_sf_color(to_0, 0.0, to_0);
  default:
    return to_sf_color(.988, .027, .910);
  }
}

sf::Color compute_pixel(const Point& p, const Settings& s, unsigned max_iter) {
  auto cpoint = make_cpoint(p, s);

  // https://en.wikipedia.org/wiki/Plotting_algorithms_for_the_Mandelbrot_set#Optimized_escape_time_algorithms
  // optimised code to reduce number of floating point operations and eliminate any calls
  // ~20% faster than: cur = cur * cur + cpoint; if (std::norm(cur) > 2.0 * 2.0) return ...

  ComplexType x0 = cpoint.real();
  ComplexType y0 = cpoint.imag();
  ComplexType x  = 0.0;
  ComplexType y  = 0.0;

  unsigned stop_iter = max_iter;
  unsigned i         = 0;
  while (i < stop_iter) {
    ComplexType x2 = x * x;
    ComplexType y2 = y * y;

    y = 2 * x * y + y0;
    x = x2 - y2 + x0;

    if (x2 + y2 > 2.0 * 2.0 && stop_iter == max_iter)
      stop_iter = i + 5; // extra iterations to remove artifacts
    ++i;
  }
  return (i == max_iter) ? sf::Color::Black : make_color(i, std::complex<ComplexType>(x, y));
}

// In order to facilitate easy parallelisation of the img.set_pixel(compute_color()) call
// we create a matrix of Points such that std::for_each() can just iterate over them.
// Recreated on each window resize
std::vector<Point> get_grid(Size size) {
  std::vector<Point> grid(size.width * size.height);
  for (PointType y = 0; y != size.height; ++y)
    for (PointType x = 0; x != size.width; ++x) grid[x + y * size.width] = Point{x, y};
  return grid;
}

void render(const Settings& s, sf::Image& img) {
  static Settings           last_settings;
  static std::vector<Point> grid = get_grid(s.size);
  if (last_settings.size != s.size) grid = get_grid(s.size); // rebuild grid on window resize

  static unsigned max_iter = Settings::max_iter_start;

  if (last_settings != s) max_iter = Settings::max_iter_start; // start with a fast render

  if (max_iter <= Settings::max_iter_max) {
    using namespace std::string_literals;
    os::bch::Timer t1("max_iter = "s + std::to_string(max_iter) + ": "s);
    std::for_each(std::execution::par_unseq, begin(grid), end(grid),
                  [&img, &s](auto& p) { img.setPixel(p.x, p.y, compute_pixel(p, s, max_iter)); });
    max_iter += Settings::max_iter_inc; // progressively refine
  }
  last_settings = s;
}

void update_settings(Settings& s) {
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageUp)) s.scale(0.9);
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::PageDown)) s.scale(1.1);

  bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) s.move_left(shift);
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) s.move_right(shift);
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) s.move_up(shift);
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) s.move_down(shift);
  if (sf::Keyboard::isKeyPressed(sf::Keyboard::P))
    std::cout << std::setprecision(std::numeric_limits<ComplexType>::digits10) << s.center << " @ "
              << s.x_scale << "\n";
}

void resize(sf::RenderWindow& window, sf::Image& img, sf::Texture& texture, sf::Sprite& sprite,
            Settings& s) {
  s.x_scale *= 1.0 * window.getSize().x / s.size.width;
  s.y_scale *= 1.0 * window.getSize().y / s.size.height;
  s.size.width  = window.getSize().x;
  s.size.height = window.getSize().y;
  sf::Image img2;
  img2.create(s.size.width, s.size.height);
  img = img2;
  texture.loadFromImage(img); // load is needed for a size change?
  sprite.setTexture(texture, true);
  window.setView(sf::View(
      sf::Rect(0.0F, 0.0F, static_cast<float>(s.size.width), static_cast<float>(s.size.height))));
}

int main() {
  Settings s(true);

  sf::RenderWindow window(sf::VideoMode(s.size.width, s.size.height), "Mandelbrot");
  window.setVerticalSyncEnabled(true);

  sf::Image img;
  img.create(s.size.width, s.size.height);
  sf::Texture texture;
  texture.loadFromImage(img);
  sf::Sprite sprite(texture);
  sf::Clock  clock;

  while (window.isOpen()) {
    sf::Event event{};
    while (window.pollEvent(event)) {
      if (event.type == sf::Event::Closed) window.close();
      if (event.type == sf::Event::Resized) resize(window, img, texture, sprite, s);
    }

    update_settings(s);
    render(s, img);
    texture.update(img);
    window.draw(sprite);
    window.display(); // frame rate limited to vsync
  }
  return EXIT_SUCCESS;
}
