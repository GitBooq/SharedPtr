#include <gtest/gtest.h> // for Test, Message, TestPartResult, EXPECT_EQ
#include <compare>       // for operator<, strong_ordering
#include <map>           // for map
#include <stdexcept>     // for runtime_error
#include <string>        // for allocator, string
#include <utility>       // for swap, move
#include <vector>        // for vector
#include "Shared_ptr.h"  // for SharedPtr, WeakPtr, operator==, operator<=>

using namespace my::memory;

// Helper class for testing purposes
struct TObj
{
    int value;
    static int ctor_cnt;
    static int dtor_cnt;
    static int copy_cnt;
    static int move_cnt;

    TObj(int v = 0) : value(v)
    {
        ++ctor_cnt;
    }

    TObj(const TObj &other) : value(other.value)
    {
        ++copy_cnt;
        ++ctor_cnt;
    }

    TObj(TObj &&other) noexcept : value(other.value)
    {
        ++move_cnt;
        ++ctor_cnt;
        other.value = 0;
    }

    ~TObj()
    {
        ++dtor_cnt;
    }

    static void reset_counters()
    {
        ctor_cnt = 0;
        dtor_cnt = 0;
        copy_cnt = 0;
        move_cnt = 0;
    }
};

int TObj::ctor_cnt = 0;
int TObj::dtor_cnt = 0;
int TObj::copy_cnt = 0;
int TObj::move_cnt = 0;

// Helper class to test inheritance
struct Base
{
    virtual ~Base() = default;
    int base_value = 42;
};

struct Derived : Base
{
    int derived_value = 100;
};

TEST(SharedPtrTest, DefaultConstructor)
{
    SharedPtr<int> ptr;
    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(ptr.use_count(), 0);
    EXPECT_FALSE(static_cast<bool>(ptr));
}

TEST(SharedPtrTest, ConstructorFromRawPointer)
{
    TObj::reset_counters();

    auto *raw = new TObj(42);
    {
        SharedPtr<TObj> ptr(raw);
        EXPECT_EQ(ptr.get(), raw);
        EXPECT_EQ(ptr->value, 42);
        EXPECT_EQ((*ptr).value, 42);
        EXPECT_TRUE(static_cast<bool>(ptr));
        EXPECT_EQ(ptr.use_count(), 1);
    }

    EXPECT_EQ(TObj::dtor_cnt, 1);
    EXPECT_EQ(TObj::ctor_cnt, 1);
}

TEST(SharedPtrTest, ConstructorFromNullptr)
{
    SharedPtr<int> ptr(nullptr);
    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(ptr.use_count(), 0);
}

TEST(SharedPtrTest, CopyConstructor)
{
    TObj::reset_counters();

    SharedPtr<TObj> ptr1(new TObj(42));
    {
        SharedPtr<TObj> ptr2(ptr1);

        EXPECT_EQ(ptr1.get(), ptr2.get());
        EXPECT_EQ(ptr1.use_count(), 2);
        EXPECT_EQ(ptr2.use_count(), 2);
        EXPECT_EQ(ptr1->value, 42);
        EXPECT_EQ(ptr2->value, 42);
    }

    EXPECT_EQ(ptr1.use_count(), 1);
    EXPECT_EQ(TObj::dtor_cnt, 0);

    ptr1.reset();
    EXPECT_EQ(TObj::dtor_cnt, 1);
}

TEST(SharedPtrTest, MoveConstructor)
{
    TObj::reset_counters();

    SharedPtr<TObj> ptr1(new TObj(42));
    auto *raw = ptr1.get();

    SharedPtr<TObj> ptr2(std::move(ptr1));

    EXPECT_EQ(ptr2.get(), raw);
    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr1.use_count(), 0);
    EXPECT_EQ(ptr2.use_count(), 1);
    EXPECT_EQ(ptr2->value, 42);
}

TEST(SharedPtrTest, CopyAssignment)
{
    TObj::reset_counters();

    SharedPtr<TObj> ptr1(new TObj(42));
    SharedPtr<TObj> ptr2(new TObj(100));

    ptr2 = ptr1;

    EXPECT_EQ(ptr1.get(), ptr2.get());
    EXPECT_EQ(ptr1.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);
    EXPECT_EQ(ptr2->value, 42);
}

TEST(SharedPtrTest, CopyAssignmentSelfAssignment)
{
    SharedPtr<int> ptr(new int(42));
    auto *raw = ptr.get();
    auto cnt = ptr.use_count();

    ptr = ptr;

    EXPECT_EQ(ptr.get(), raw);
    EXPECT_EQ(ptr.use_count(), cnt);
    EXPECT_EQ(*ptr, 42);
}

TEST(SharedPtrTest, MoveAssignment)
{
    TObj::reset_counters();

    SharedPtr<TObj> ptr1(new TObj(42));
    SharedPtr<TObj> ptr2(new TObj(100));
    auto *raw = ptr1.get();

    ptr2 = std::move(ptr1);

    EXPECT_EQ(ptr2.get(), raw);
    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr1.use_count(), 0);
    EXPECT_EQ(ptr2.use_count(), 1);
    EXPECT_EQ(ptr2->value, 42);
}

TEST(SharedPtrTest, ResetToNull)
{
    TObj::reset_counters();

    SharedPtr<TObj> ptr(new TObj(42));
    EXPECT_EQ(ptr.use_count(), 1);

    ptr.reset();

    EXPECT_EQ(ptr.get(), nullptr);
    EXPECT_EQ(ptr.use_count(), 0);
    EXPECT_EQ(TObj::dtor_cnt, 1);
}

TEST(SharedPtrTest, ResetToNewPointer)
{
    TObj::reset_counters();

    SharedPtr<TObj> ptr(new TObj(42));
    auto *raw1 = ptr.get();

    ptr.reset(new TObj(100));

    EXPECT_NE(ptr.get(), raw1);
    EXPECT_EQ(ptr->value, 100);
    EXPECT_EQ(ptr.use_count(), 1);
    EXPECT_EQ(TObj::ctor_cnt, 2);
    EXPECT_EQ(TObj::dtor_cnt, 1);
}

TEST(SharedPtrTest, ResetWithSharedOwnership)
{
    SharedPtr<int> ptr1(new int(42));
    SharedPtr<int> ptr2(ptr1);

    EXPECT_EQ(ptr1.use_count(), 2);
    EXPECT_EQ(ptr2.use_count(), 2);

    ptr1.reset(new int(100));

    EXPECT_EQ(ptr1.use_count(), 1);
    EXPECT_EQ(ptr2.use_count(), 1);
    EXPECT_NE(ptr1.get(), ptr2.get());
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(*ptr1, 100);
}

TEST(SharedPtrTest, UseCountBasic)
{
    SharedPtr<int> ptr1;
    EXPECT_EQ(ptr1.use_count(), 0);

    SharedPtr<int> ptr2(new int(42));
    EXPECT_EQ(ptr2.use_count(), 1);

    SharedPtr<int> ptr3(ptr2);
    EXPECT_EQ(ptr2.use_count(), 2);
    EXPECT_EQ(ptr3.use_count(), 2);

    SharedPtr<int> ptr4(std::move(ptr3));
    EXPECT_EQ(ptr2.use_count(), 2);
    EXPECT_EQ(ptr4.use_count(), 2);
    EXPECT_EQ(ptr3.use_count(), 0);
}

TEST(SharedPtrTest, DereferenceOperators)
{
    struct Point
    {
        int x, y;
    };

    SharedPtr<Point> ptr(new Point{10, 20});

    EXPECT_EQ((*ptr).x, 10);
    EXPECT_EQ(ptr->y, 20);

    (*ptr).x = 30;
    EXPECT_EQ(ptr->x, 30);
}

TEST(SharedPtrTest, Swap)
{
    SharedPtr<int> ptr1(new int(42));
    SharedPtr<int> ptr2(new int(100));

    auto *raw1 = ptr1.get();
    auto *raw2 = ptr2.get();

    std::swap(ptr1, ptr2);

    EXPECT_EQ(ptr1.get(), raw2);
    EXPECT_EQ(ptr2.get(), raw1);
    EXPECT_EQ(*ptr1, 100);
    EXPECT_EQ(*ptr2, 42);
    EXPECT_EQ(ptr1.use_count(), 1);
    EXPECT_EQ(ptr2.use_count(), 1);
}

TEST(SharedPtrTest, SwapWithEmpty)
{
    SharedPtr<int> ptr1(new int(42));
    SharedPtr<int> ptr2;

    auto *raw1 = ptr1.get();

    ptr1.swap(ptr2);

    EXPECT_EQ(ptr1.get(), nullptr);
    EXPECT_EQ(ptr2.get(), raw1);
    EXPECT_EQ(ptr2.use_count(), 1);
}

TEST(SharedPtrTest, ArrayConstructor)
{
    SharedPtr<int[]> ptr(new int[3]{1, 2, 3});

    EXPECT_EQ(ptr[0], 1);
    EXPECT_EQ(ptr[1], 2);
    EXPECT_EQ(ptr[2], 3);
    EXPECT_EQ(ptr.get(), static_cast<int *>(ptr.get()));
}

TEST(SharedPtrTest, ArrayMove)
{
    SharedPtr<int[]> ptr1(new int[3]{1, 2, 3});
    auto *raw = ptr1.get();

    SharedPtr<int[]> ptr2(std::move(ptr1));

    EXPECT_EQ(ptr2[1], 2);
    EXPECT_EQ(ptr2.get(), raw);
    EXPECT_EQ(ptr1.get(), nullptr);
}

TEST(SharedPtrTest, Polymorphism)
{
    SharedPtr<Derived> derived_ptr(new Derived());
    auto *raw = derived_ptr.get();

    SharedPtr<Base> base_ptr(derived_ptr);

    EXPECT_EQ(base_ptr.get(), raw);
    EXPECT_EQ(base_ptr->base_value, 42);
    EXPECT_EQ(derived_ptr.use_count(), 2);
    EXPECT_EQ(base_ptr.use_count(), 2);
}

TEST(SharedPtrTest, PolymorphismWithRawPointer)
{
    SharedPtr<Base> base_ptr(new Derived());

    EXPECT_EQ(base_ptr->base_value, 42);
    EXPECT_EQ(base_ptr.use_count(), 1);
}

TEST(SharedPtrTest, VectorOfSharedPtr)
{
    std::vector<SharedPtr<int>> vec;

    for (int i = 0; i < 10; ++i)
    {
        vec.push_back(SharedPtr<int>(new int(i)));
    }

    for (int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(*vec[i], i);
    }

    // Copying test
    auto vec2 = vec;
    EXPECT_EQ(vec[0].use_count(), 2);
    EXPECT_EQ(vec2[0].use_count(), 2);
}

TEST(SharedPtrTest, SharedPtrInMap)
{
    std::map<int, SharedPtr<std::string>> map;

    map[1] = SharedPtr<std::string>(new std::string("one"));
    map[2] = SharedPtr<std::string>(new std::string("two"));

    EXPECT_EQ(*map[1], "one");
    EXPECT_EQ(*map[2], "two");
    EXPECT_EQ(map[1].use_count(), 1);
}

TEST(SharedPtrTest, MultipleSharedOwnership)
{
    TObj::reset_counters();

    SharedPtr<TObj> ptr1(new TObj(42));

    std::vector<SharedPtr<TObj>> owners;
    for (int i = 0; i < 10; ++i)
    {
        owners.push_back(ptr1);
    }

    EXPECT_EQ(ptr1.use_count(), 11);
    EXPECT_EQ(TObj::ctor_cnt, 1);
    EXPECT_EQ(TObj::dtor_cnt, 0);

    owners.clear();
    EXPECT_EQ(ptr1.use_count(), 1);
    EXPECT_EQ(TObj::dtor_cnt, 0);

    ptr1.reset();
    EXPECT_EQ(TObj::dtor_cnt, 1);
}

TEST(SharedPtrTest, ExceptionSafety)
{
    struct ThrowingInConstructor
    {
        ThrowingInConstructor()
        {
            throw std::runtime_error("Construction failed");
        }
        ~ThrowingInConstructor() = default;
    };

    EXPECT_THROW({ SharedPtr<ThrowingInConstructor> ptr(new ThrowingInConstructor()); }, std::runtime_error);

    // Should't cause mem leaks, ASan shows ones if any
}

TEST(SharedPtrTest, NoDoubleDelete)
{
    TObj::reset_counters();

    SharedPtr<TObj> ptr1(new TObj(42));
    SharedPtr<TObj> ptr2(ptr1);
    SharedPtr<TObj> ptr3(ptr2);

    EXPECT_EQ(TObj::ctor_cnt, 1);

    ptr1.reset();
    ptr2.reset();
    ptr3.reset();

    EXPECT_EQ(TObj::dtor_cnt, 1);
}

TEST(SharedPtrTest, ComparisonOperators)
{
    SharedPtr<int> ptr1(new int(42));
    SharedPtr<int> ptr2(ptr1);
    SharedPtr<int> ptr3(new int(100));
    SharedPtr<int> ptr4; // nullptr

    EXPECT_TRUE(ptr1 == ptr2);
    EXPECT_FALSE(ptr1 == ptr3);
    EXPECT_FALSE(ptr1 == ptr4);

    EXPECT_FALSE(ptr1 != ptr2);
    EXPECT_TRUE(ptr1 != ptr3);
    EXPECT_TRUE(ptr1 != ptr4);

    EXPECT_FALSE(ptr1 < ptr2);
    EXPECT_TRUE(ptr4 < ptr1);

    EXPECT_FALSE(ptr1 == nullptr);
    EXPECT_FALSE(nullptr == ptr1);

    EXPECT_TRUE(ptr4 == nullptr);
    EXPECT_TRUE(nullptr == ptr4);
}

TEST(WeakPtrTest, DefaultConstructor)
{
    WeakPtr<int> wptr;
    EXPECT_EQ(wptr.use_count(), 0);
    EXPECT_TRUE(wptr.expired());
    EXPECT_EQ(wptr.lock().get(), nullptr);
}

TEST(WeakPtrTest, ConstructFromSharedPtr)
{
    SharedPtr<int> sptr(new int(42));
    WeakPtr<int> wptr(sptr);

    EXPECT_EQ(wptr.use_count(), 1);
    EXPECT_FALSE(wptr.expired());
}

TEST(WeakPtrTest, CopyConstructor)
{
    SharedPtr<int> sptr(new int(42));
    WeakPtr<int> wptr1(sptr);
    WeakPtr<int> wptr2(wptr1);

    EXPECT_EQ(wptr1.use_count(), 1);
    EXPECT_EQ(wptr2.use_count(), 1);
    EXPECT_FALSE(wptr1.expired());
    EXPECT_FALSE(wptr2.expired());
}

TEST(WeakPtrTest, MoveConstructor)
{
    SharedPtr<int> sptr(new int(42));
    WeakPtr<int> wptr1(sptr);
    WeakPtr<int> wptr2(std::move(wptr1));

    EXPECT_EQ(wptr1.use_count(), 0);
    EXPECT_TRUE(wptr1.expired());
    EXPECT_EQ(wptr2.use_count(), 1);
    EXPECT_FALSE(wptr2.expired());
}

TEST(WeakPtrTest, LockReturnsValidSharedPtr)
{
    SharedPtr<int> sptr(new int(42));
    WeakPtr<int> wptr(sptr);

    auto locked = wptr.lock();
    EXPECT_EQ(*locked, 42);
    EXPECT_EQ(locked.use_count(), 2);
    EXPECT_EQ(wptr.use_count(), 2);
}

TEST(WeakPtrTest, LockReturnsEmptyAfterReset)
{
    SharedPtr<int> sptr(new int(42));
    WeakPtr<int> wptr(sptr);

    sptr.reset();

    auto locked = wptr.lock();
    EXPECT_EQ(locked.get(), nullptr);
    EXPECT_TRUE(wptr.expired());
    EXPECT_EQ(wptr.use_count(), 0);
}

TEST(WeakPtrTest, LockReturnsEmptyAfterAllSharedPtrsDestroyed)
{
    WeakPtr<int> wptr;

    {
        SharedPtr<int> sptr(new int(42));
        wptr = sptr;
        EXPECT_FALSE(wptr.expired());

        auto locked = wptr.lock();
        EXPECT_NE(locked.get(), nullptr);
        EXPECT_EQ(wptr.use_count(), 2);
    }

    EXPECT_TRUE(wptr.expired());
    EXPECT_EQ(wptr.use_count(), 0);

    auto locked = wptr.lock();
    EXPECT_EQ(locked.get(), nullptr);
}

TEST(WeakPtrTest, CopyAssignment)
{
    SharedPtr<int> sptr1(new int(42));
    SharedPtr<int> sptr2(new int(100));
    WeakPtr<int> wptr1(sptr1);
    WeakPtr<int> wptr2(sptr2);

    wptr1 = wptr2;

    EXPECT_EQ(wptr1.use_count(), 1);
    EXPECT_EQ(wptr2.use_count(), 1);

    auto locked = wptr1.lock();
    EXPECT_EQ(*locked, 100);
}

TEST(WeakPtrTest, MoveAssignment)
{
    SharedPtr<int> sptr(new int(42));
    WeakPtr<int> wptr1(sptr);
    WeakPtr<int> wptr2;

    wptr2 = std::move(wptr1);

    EXPECT_EQ(wptr1.use_count(), 0);
    EXPECT_TRUE(wptr1.expired());
    EXPECT_EQ(wptr2.use_count(), 1);
    EXPECT_FALSE(wptr2.expired());
}

TEST(WeakPtrTest, Reset)
{
    SharedPtr<int> sptr(new int(42));
    WeakPtr<int> wptr(sptr);

    EXPECT_FALSE(wptr.expired());

    wptr.reset();

    EXPECT_EQ(wptr.use_count(), 0);
    EXPECT_TRUE(wptr.expired());
    EXPECT_EQ(wptr.lock().get(), nullptr);

    EXPECT_EQ(*sptr, 42);
}

TEST(WeakPtrTest, Swap)
{
    SharedPtr<int> sptr1(new int(42));
    SharedPtr<int> sptr2(new int(100));
    WeakPtr<int> wptr1(sptr1);
    WeakPtr<int> wptr2(sptr2);

    wptr1.swap(wptr2);

    EXPECT_EQ(*wptr1.lock(), 100);
    EXPECT_EQ(*wptr2.lock(), 42);
}

TEST(WeakPtrTest, UseCountAfterMultipleWeakPtrs)
{
    SharedPtr<int> sptr(new int(42));
    WeakPtr<int> wptr1(sptr);
    WeakPtr<int> wptr2(wptr1);
    WeakPtr<int> wptr3(wptr1);

    EXPECT_EQ(wptr1.use_count(), 1);
    EXPECT_EQ(wptr2.use_count(), 1);
    EXPECT_EQ(wptr3.use_count(), 1);

    auto locked = wptr1.lock();
    EXPECT_EQ(wptr1.use_count(), 2);
    EXPECT_EQ(wptr2.use_count(), 2);
    EXPECT_EQ(wptr3.use_count(), 2);

    locked.reset();
    EXPECT_EQ(wptr1.use_count(), 1);
}

TEST(WeakPtrTest, ConvertibleTypes)
{
    struct Base
    {
        virtual ~Base() = default;
        int b = 1;
    };
    struct Derived : Base
    {
        int d = 2;
    };

    SharedPtr<Derived> sptr(new Derived());
    WeakPtr<Derived> wptr_derived(sptr);
    WeakPtr<Base> wptr_base(wptr_derived);

    EXPECT_FALSE(wptr_base.expired());

    auto locked = wptr_base.lock();
    EXPECT_EQ(locked->b, 1);
    EXPECT_EQ(wptr_base.use_count(), 2);
}

TEST(WeakPtrTest, ExpiredAfterSharedPtrResetWithMultipleWeak)
{
    SharedPtr<int> sptr(new int(42));
    WeakPtr<int> wptr1(sptr);
    WeakPtr<int> wptr2(sptr);

    sptr.reset();

    EXPECT_TRUE(wptr1.expired());
    EXPECT_TRUE(wptr2.expired());
    EXPECT_EQ(wptr1.use_count(), 0);
    EXPECT_EQ(wptr2.use_count(), 0);
}

TEST(SharedPtrTest, BreakCyclicDependencies)
{
    TObj::reset_counters();

    struct Node
    {
        TObj data;
        SharedPtr<Node> next;
        WeakPtr<Node> prev;

        Node(int v) : data(v) {}
        ~Node() = default;
    };

    {
        auto n1 = new Node(1);
        auto n2 = new Node(2);
        SharedPtr<Node> node1(n1);
        SharedPtr<Node> node2(n2);

        node1->next = node2;
        node2->prev = node1;

        EXPECT_EQ(node1.use_count(), 1);
        EXPECT_EQ(node2.use_count(), 2);

        EXPECT_EQ(TObj::ctor_cnt, 2);
        EXPECT_EQ(TObj::dtor_cnt, 0);
    }

    EXPECT_EQ(TObj::ctor_cnt, 2);
    EXPECT_EQ(TObj::dtor_cnt, 2);
}

TEST(SharedPtrCompareTest, EqualityAndInequality)
{
    SharedPtr<int> p1(new int(42));
    SharedPtr<int> p2(new int(42)); // different pointer, same value
    SharedPtr<int> p3(p1);          // same pointer as p1
    SharedPtr<int> p4;              // empty

    // Same pointer -> equal
    EXPECT_TRUE(p1 == p3);
    EXPECT_FALSE(p1 != p3);

    // Different pointers -> not equal
    EXPECT_FALSE(p1 == p2);
    EXPECT_TRUE(p1 != p2);

    // Empty vs non-empty
    EXPECT_FALSE(p1 == p4);
    EXPECT_TRUE(p1 != p4);

    // Both empty
    SharedPtr<int> p5;
    SharedPtr<int> p6;
    EXPECT_TRUE(p5 == p6);
    EXPECT_FALSE(p5 != p6);
}

TEST(SharedPtrCompareTest, NullptrComparison)
{
    SharedPtr<int> p1(new int(42));
    SharedPtr<int> p2;

    // Non-empty vs nullptr
    EXPECT_FALSE(p1 == nullptr);
    EXPECT_TRUE(p1 != nullptr);
    EXPECT_FALSE(nullptr == p1);
    EXPECT_TRUE(nullptr != p1);

    // Empty vs nullptr
    EXPECT_TRUE(p2 == nullptr);
    EXPECT_FALSE(p2 != nullptr);
    EXPECT_TRUE(nullptr == p2);
    EXPECT_FALSE(nullptr != p2);
}

TEST(SharedPtrCompareTest, ThreeWayComparison)
{
    SharedPtr<int> p1(new int(42));
    SharedPtr<int> p2(new int(100));
    SharedPtr<int> p3(p1); // same as p1
    SharedPtr<int> p4;     // empty

    // Order is based on memory addresses
    if (p1.get() < p2.get())
    {
        EXPECT_TRUE(p1 < p2);
        EXPECT_TRUE(p1 <= p2);
        EXPECT_FALSE(p1 > p2);
        EXPECT_FALSE(p1 >= p2);
    }
    else if (p1.get() > p2.get())
    {
        EXPECT_TRUE(p1 > p2);
        EXPECT_TRUE(p1 >= p2);
        EXPECT_FALSE(p1 < p2);
        EXPECT_FALSE(p1 <= p2);
    }

    // Same pointer
    EXPECT_TRUE(p1 == p3);
    EXPECT_TRUE(p1 <= p3);
    EXPECT_TRUE(p1 >= p3);
    EXPECT_FALSE(p1 < p3);
    EXPECT_FALSE(p1 > p3);

    // Empty pointer is less than any non-empty
    if (p1.get() != nullptr)
    {
        EXPECT_TRUE(p4 < p1);
        EXPECT_TRUE(p4 <= p1);
        EXPECT_FALSE(p4 > p1);
        EXPECT_FALSE(p4 >= p1);

        EXPECT_TRUE(p1 > p4);
        EXPECT_TRUE(p1 >= p4);
        EXPECT_FALSE(p1 < p4);
        EXPECT_FALSE(p1 <= p4);
    }
}

TEST(SharedPtrCompareTest, PolymorphicComparisons)
{
    SharedPtr<Derived> derived1(new Derived());
    SharedPtr<Derived> derived2(new Derived());
    SharedPtr<Base> base1(derived1); // points to same as derived1
    SharedPtr<Base> base2(new Base());
    SharedPtr<Base> base3; // empty

    // Check that pointers are different
    EXPECT_NE(derived1.get(), derived2.get());
    EXPECT_NE(derived1.get(), base2.get());

    // Different types should compare correctly
    EXPECT_TRUE(derived1 == base1);
    EXPECT_FALSE(derived1 == base2);

    EXPECT_TRUE(derived1 != base2);
    EXPECT_FALSE(derived1 != base1);

    // Ordering between different types
    if (derived1.get() < derived2.get())
    {
        EXPECT_TRUE(derived1 < derived2);
        EXPECT_TRUE(derived1 < base2);
        EXPECT_TRUE(base1 < derived2);
    }
    else if (derived1.get() > derived2.get())
    {
        EXPECT_TRUE(derived1 > derived2);
        EXPECT_TRUE(derived1 > base2);
        EXPECT_TRUE(base1 > derived2);
    }

    // Empty vs non-empty across types
    EXPECT_TRUE(base3 < derived1);
    EXPECT_TRUE(derived1 > base3);

    // Check member access to ensure objects are correct
    EXPECT_EQ(base1->base_value, 42);
    EXPECT_EQ(derived1->derived_value, 100);
}

TEST(SharedPtrCompareTest, ArrayComparisons)
{
    SharedPtr<int[]> arr1(new int[5]{1, 2, 3, 4, 5});
    SharedPtr<int[]> arr2(new int[5]{1, 2, 3, 4, 5}); // different array
    SharedPtr<int[]> arr3(arr1);                      // same as arr1
    SharedPtr<int[]> arr4;                            // empty

    // Equality
    EXPECT_TRUE(arr1 == arr3);
    EXPECT_FALSE(arr1 == arr2);
    EXPECT_TRUE(arr1 != arr2);

    // nullptr comparison
    EXPECT_FALSE(arr1 == nullptr);
    EXPECT_TRUE(arr1 != nullptr);
    EXPECT_TRUE(arr4 == nullptr);
    EXPECT_FALSE(arr4 != nullptr);

    // Ordering
    if (arr1.get() < arr2.get())
    {
        EXPECT_TRUE(arr1 < arr2);
        EXPECT_TRUE(arr1 <= arr2);
        EXPECT_FALSE(arr1 > arr2);
        EXPECT_FALSE(arr1 >= arr2);
    }
    else if (arr1.get() > arr2.get())
    {
        EXPECT_TRUE(arr1 > arr2);
        EXPECT_TRUE(arr1 >= arr2);
        EXPECT_FALSE(arr1 < arr2);
        EXPECT_FALSE(arr1 <= arr2);
    }

    // Empty vs non-empty
    EXPECT_TRUE(arr4 < arr1);
    EXPECT_TRUE(arr1 > arr4);
}

TEST(SharedPtrCompareTest, ConstCorrectness)
{
    SharedPtr<int> p1(new int(42));
    const SharedPtr<int> p2(p1);
    const SharedPtr<int> p3;

    // Const objects should be comparable
    EXPECT_TRUE(p2 == p1);
    EXPECT_TRUE(p1 == p2);
    EXPECT_TRUE(p2 != p3);
    EXPECT_TRUE(p2 > p3);
    EXPECT_TRUE(p3 < p2);

    // Const with nullptr
    EXPECT_FALSE(p2 == nullptr);
    EXPECT_TRUE(p3 == nullptr);
}

TEST(SharedPtrCompareTest, ComparisonAfterReset)
{
    SharedPtr<int> p1(new int(42));
    SharedPtr<int> p2(new int(100));
    SharedPtr<int> p3(p1);

    EXPECT_TRUE(p1 == p3);
    EXPECT_FALSE(p1 == p2);

    p1.reset();
    EXPECT_FALSE(p1 == p3);
    EXPECT_TRUE(p1 != p3);
    EXPECT_TRUE(p1 == nullptr);

    p2.reset();
    EXPECT_TRUE(p2 == nullptr);
    EXPECT_TRUE(p1 == p2); // both empty
}
