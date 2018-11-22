#include <lely/tap/tap.h>
#include <lely/util/c_factory.hpp>

class Base {
 public:
  virtual ~Base(){};

  virtual int operator()(int) = 0;
};

class Derived : public Base {
 public:
  explicit Derived(int x) : m_x(x) {}

  virtual int
  operator()(int x) {
    return m_x + x;
  }

 private:
  int m_x;
};

LELY_C_STATIC_FACTORY_1("test", Derived, int)

int
main() {
  tap_plan(1);

  lely::c_factory<Derived*(int), Base*> f("test");

  Base* p = f.create(42);
  tap_assert(p);

  tap_test((*p)(12) == 54);

  f.destroy(p);

  return 0;
}
