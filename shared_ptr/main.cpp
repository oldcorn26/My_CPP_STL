#include <iostream>
#include <atomic>
#include <functional>
#include <cassert>

template<typename T>
class SharedPtr {
public:
    SharedPtr() : ptr_(nullptr), count_(nullptr), deleter_(defaultDeleter) {}

    explicit SharedPtr(T *ptr, std::function<void(T *)> deleter = defaultDeleter)
            : ptr_(ptr), count_(new std::atomic<int>(1)), deleter_(deleter) {}

    ~SharedPtr() noexcept {
        release();
    }

    SharedPtr(const SharedPtr &other) noexcept {
        acquire(other);
    }

    SharedPtr &operator=(const SharedPtr &other) noexcept {
        if (this != &other) {
            release();
            acquire(other);
        }
        return *this;
    }

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    T *get() const noexcept {
        return ptr_;
    }

    T &operator*() const noexcept {
        return *ptr_;
    }

    T *operator->() const noexcept {
        return ptr_;
    }

    void reset(T *ptr = nullptr, std::function<void(T *)> deleter = defaultDeleter) {
        release();
        ptr_ = ptr;
        count_ = new std::atomic<int>(1);
        deleter_ = deleter;
    }

    int use_count() const noexcept {
        return count_ ? count_->load() : 0;
    }

    void swap(SharedPtr &other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(count_, other.count_);
        std::swap(deleter_, other.deleter_);
    }

private:
    static void defaultDeleter(T *ptr) {
        delete ptr;
    }

    void acquire(const SharedPtr &other) noexcept {
        ptr_ = other.ptr_;
        count_ = other.count_;
        deleter_ = other.deleter_;
        if (count_) {
            count_->fetch_add(1);
        }
    }

    void release() noexcept {
        if (count_ && count_->fetch_sub(1) == 1) {
            deleter_(ptr_);
            delete count_;
        }
    }

    T *ptr_;
    std::atomic<int> *count_;
    std::function<void(T *)> deleter_;
};

template<typename T>
void swap(SharedPtr<T> &a, SharedPtr<T> &b) noexcept {
    a.swap(b);
}

template<typename T, typename... Args>
SharedPtr<T> make_shared(Args &&... args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

int main() {
    // Test default constructor
    SharedPtr<int> ptr1(new int(5));
    assert(*ptr1 == 5);
    assert(ptr1.use_count() == 1);

    // Test copy constructor
    SharedPtr<int> ptr2(ptr1);
    assert(ptr2.use_count() == 2);
    assert(ptr1.use_count() == 2);

    // Test copy assignment
    SharedPtr<int> ptr3;
    ptr3 = ptr1;
    assert(ptr3.use_count() == 3);
    assert(ptr1.use_count() == 3);
    assert(ptr2.use_count() == 3);

    // Test make_shared
    auto ptr4 = make_shared<int>(42);
    assert(*ptr4 == 42);
    assert(ptr4.use_count() == 1);

    // Test reset
    ptr1.reset(new int(10));
    assert(*ptr1 == 10);
    assert(ptr1.use_count() == 1);
    assert(ptr2.use_count() == 2); // 注意这里 ptr2 和 ptr3 还共享同一资源

    // Test swap
    swap(ptr1, ptr4);
    assert(*ptr1 == 42);
    assert(*ptr4 == 10);

    std::cout << "All tests passed!" << std::endl;

    return 0;
}
