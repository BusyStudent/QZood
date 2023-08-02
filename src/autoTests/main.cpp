#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#pragma comment(linker, "/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")
#endif

#include <gtest/gtest.h>
using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::UnitTest;

namespace {
// We will track memory used by this class.
class Water {
 public:
  // Normal Water declarations go here.

  // operator new and operator delete help us control water allocation.
  void* operator new(size_t allocation_size) {
    allocated_++;
    return malloc(allocation_size);
  }

  void operator delete(void* block, size_t /* allocation_size */) {
    allocated_--;
    free(block);
  }

  static int allocated() { return allocated_; }

 private:
  static int allocated_;
};

int Water::allocated_ = 0;

class LeakChecker : public EmptyTestEventListener {
 private:
  // Called before a test starts.
  void OnTestStart(const TestInfo& /* test_info */) override {
    initially_allocated_ = Water::allocated();
  }

  // Called after a test ends.
  void OnTestEnd(const TestInfo& /* test_info */) override {
    int difference = Water::allocated() - initially_allocated_;

    // You can generate a failure in any event handler except
    // OnTestPartResult. Just use an appropriate Google Test assertion to do
    // it.
    EXPECT_LE(difference, 0) << "Leaked " << difference << " unit(s) of Water!";
  }

  int initially_allocated_;
};

TEST(ListenersTest, DoesNotLeak) {
  Water* water = new Water;
  delete water;
}

// This should fail when the --check_for_leaks command line flag is
// specified.
TEST(ListenersTest, LeaksWater) {
  Water* water = new Water;
  EXPECT_TRUE(water != nullptr);
}
}  // namespace

int main(int argc, char** argv) {
  InitGoogleTest(&argc, argv);

  bool check_for_leaks = false;
  if (argc > 1 && strcmp(argv[1], "--check_for_leaks") == 0)
    check_for_leaks = true;
  else
    printf("%s\n",
           "Run this program with --check_for_leaks to enable "
           "custom leak checking in the tests.");

  // If we are given the --check_for_leaks command line flag, installs the
  // leak checker.
  if (check_for_leaks) {
    TestEventListeners& listeners = UnitTest::GetInstance()->listeners();

    listeners.Append(new LeakChecker);
  }
  return RUN_ALL_TESTS();
}