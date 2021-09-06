#include <iostream>

struct A {
  A() { std::cout << "constructor\n"; }

  A(const A& rhs) {
    std::cout << "copy constructor\n";
    i = rhs.i;
  }

  A(A&& rhs) noexcept {
    std::cout << "move constructor\n";
    i = rhs.i;
  }

  ~A() noexcept { std::cout << "destructor\n"; }

  A& operator=(const A& rhs) {
    if (&rhs != this) i = rhs.i;
    std::cout << "copy asign op\n";
    return *this;
  }

  A& operator=(A&& rhs) noexcept {
    i = rhs.i;
    std::cout << "move asign op\n";
    return *this;
  }

  friend void swap(A& a, A& b) { a.swap(b); }

  void swap(A& rhs) {
    using std::swap;
    swap(i, rhs.i);
  }

  void test() const { std::cout << "hello world " << i << "\n"; }

  int i = 0;
};

int main() {
  A a;
  a.i = 2;
  A c = a;
  c.test();
  return 0;
}
