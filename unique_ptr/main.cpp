#include <iostream>
#include <utility>
#include <memory>
#include <cassert>

template<typename T, typename Deleter = std::default_delete<T>>
class UniquePtr {
public:
    explicit UniquePtr(T *ptr = nullptr, const Deleter &d = Deleter()) noexcept: ptr_(ptr), deleter_(d) {}

    ~UniquePtr() noexcept {
        reset();
    }

    UniquePtr(UniquePtr &&other) noexcept: ptr_(other.ptr_), deleter_(std::move(other.deleter_)) {
        other.ptr_ = nullptr;
    }

    UniquePtr &operator=(UniquePtr &&other) noexcept {
        swap(other);
        return *this;
    }

    T &operator*() const {
        return *ptr_;
    }

    T *operator->() const {
        return ptr_;
    }

    T *get() const noexcept {
        return ptr_;
    }

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    T *release() noexcept {
        T *tmp = ptr_;
        ptr_ = nullptr;
        return tmp;
    }

    void reset(T *p = nullptr) noexcept {
        if (ptr_) {
            deleter_(ptr_);
        }
        ptr_ = p;
    }

    void swap(UniquePtr &other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(deleter_, other.deleter_);
    }

    UniquePtr(const UniquePtr &) = delete;

    UniquePtr &operator=(const UniquePtr &) = delete;

private:
    T *ptr_;
    Deleter deleter_;
};

template<typename T, typename... Args>
UniquePtr<T> make_unique(Args &&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

template<typename T, typename D>
void swap(UniquePtr<T, D> &a, UniquePtr<T, D> &b) noexcept {
    a.swap(b);
}

template<typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    explicit UniquePtr(T *p = nullptr, const Deleter &d = Deleter()) : ptr_(p), deleter_(d) {}

    ~UniquePtr() {
        reset();
    }

    void reset(T *p = nullptr) {
        if (ptr_) {
            deleter_(ptr_);
        }
        ptr_ = p;
    }

    T &operator[](std::size_t index) const {
        return ptr_[index];
    }

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    UniquePtr(const UniquePtr &) = delete;

    UniquePtr &operator=(const UniquePtr &) = delete;

    void swap(UniquePtr &other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(deleter_, other.deleter_);
    }

private:
    T *ptr_;
    Deleter deleter_;
};

struct TestClass {
    static int count;
    TestClass() {
        ++count;
    }
    ~TestClass() {
        --count;
    }
};

int TestClass::count = 0;

int main() {
    // Test base functions
    {
        UniquePtr<TestClass> p1(new TestClass);
        assert(TestClass::count == 1);

        UniquePtr<TestClass> p2(std::move(p1));
        assert(TestClass::count == 1);
        assert(!p1);

        UniquePtr<TestClass> p3;
        p3 = std::move(p2);
        assert(TestClass::count == 1);
        assert(!p2);

        p3.reset();
        assert(TestClass::count == 0);
    }

    // Test array specialization
    {
        UniquePtr<int[]> p1(new int[5]);
        for (int i = 0; i < 5; ++i) {
            p1[i] = i;
        }

        for (int i = 0; i < 5; ++i) {
            assert(p1[i] == i);
        }

        UniquePtr<int[], std::default_delete<int[]>> p2(new int[5]);
        p2.swap(p1);

        for (int i = 0; i < 5; ++i) {
            assert(p2[i] == i);
        }

        p2.reset();
    }

    // Test make_unique
    {
        auto p = make_unique<TestClass>();
        assert(TestClass::count == 1);
    }

    // Test custom deleter
    {
        bool deleted = false;
        auto deleter = [&deleted](TestClass* ptr) {
            delete ptr;
            deleted = true;
        };

        {
            UniquePtr<TestClass, decltype(deleter)> p(new TestClass, deleter);
        }

        assert(deleted);
    }

    std::cout << "All test cases passed!" << std::endl;

    return 0;
}
