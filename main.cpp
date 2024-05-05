#include <iostream>
#include <utility>
#include <tuple>
#include <type_traits>
#include <cfloat>


class Sample final {
public:
    Sample() {
        std::cout << "create A \n";
    }

    ~Sample() {
        std::cout << "delete A\n";
    }
};

template <typename T>
class UniquePointer {
public:
    UniquePointer() = default;
    UniquePointer(T *t): ptr_(t) {}

    UniquePointer(const UniquePointer&) = delete;
    UniquePointer& operator=(const UniquePointer&) = delete;

    UniquePointer(UniquePointer &&rhs) noexcept: ptr_(std::exchange(rhs.ptr_, nullptr)) {}
//    UniquePointer(UniquePointer &&rhs) noexcept: ptr_(rhs.ptr_) {
//        rhs.ptr_ = nullptr;
//    }
    UniquePointer& operator=(UniquePointer &&rhs) noexcept {
        std::swap(ptr_, rhs.ptr_);
        return *this;
    }

    T *operator->() const {
        return get();
    }

    T &operator*() const {
        return *ptr_;
    }

    [[nodiscard]]
    T *get() const {
        return ptr_;
    }

    ~UniquePointer() {
        if (ptr_) {
            delete ptr_;
        }
    }

private:
    T *ptr_{nullptr};
};

class Any {
public:
    template<typename T>
    Any(T value) {
        value_ = new ValueHolder<T>(value);
    }
    Any(const Any &rhs) {
        *this = rhs;
    }

    Any &operator=(const Any &rhs) {
        // 该函数启动时,首先将任意类型通过类型匹配机制,初始化为新的Any
        // 然后将该Any类型内的值进行拷贝
        value_ = rhs.value_->clone();
        return *this;
    }

//    template<typename T>
//    T *value_ptr() const {
//        if (auto p = dynamic_cast<ValueHolder<T> *>(value_)) {
//            return &(p->data);
//        }
//        return nullptr;
////        throw std::logic_error("type error");
//    }

    template<typename T>
    T &value() const {
        if (auto p = dynamic_cast<ValueHolder<T> *>(value_.get())) {
            return p->data;
        }
        throw std::logic_error("type error");
    }

    operator int() const {
        return value<int>();
    }


private:
    struct Value {
        virtual UniquePointer<Value> clone() = 0;
        virtual ~Value() = default;
    };

    template<typename T>
    struct ValueHolder: Value {
        ValueHolder(T data_): data(data_) {}
        UniquePointer<Value> clone() override {
            return new ValueHolder<T>(data);
        }

        T data;
    };

    UniquePointer<Value> value_;
};


/* main function for test Any
int main() {
    Any d = 'c';

    try {
        d = 1;
        int val = d;
        printf("%d\n", val);

        d = "1243";
        auto s = d.value<const char*>();
        printf("%s\n", s);
    } catch (const std::logic_error &e) {
        printf("%s\n", e.what());
    }


    return 0;
}
*/


template <typename T>
class DefaultCreationPolicy {
public:
    static T* create() {
        return new T;
    }
};

template <typename T, typename Creator = DefaultCreationPolicy<T>>
class SingletonHolder {
public:
    static T &getInstance() {
        static UniquePointer<T> t(Creator::create());
        return *t;
    }
};


class CreatorA;

struct A {
    friend class DefaultCreationPolicy<A>;
    friend class CreatorA;

    int data = 0;
private:
    A() {}
    A(int x) {data = x;}
};

class CreatorA {
public:
    static A *create() {
        return new A(1);
    }
};

namespace singleton {
    template <typename T, typename CreationPolicy = DefaultCreationPolicy<T> >
    T singleIns = SingletonHolder<T, CreationPolicy>::getInstance();

    A &AIns = singleIns<A, CreatorA>;
}

/* 单例singleton的测试函数
int main() {
    singleton::AIns.data++;
    printf("%d", singleton::AIns.data);
    return 0;
}
 */

template<typename T, typename... Args>
auto make_Unique(const Args&&... args) {
    return UniquePointer<T>(new T(args...));
}

struct Area {
    int val;
    int val2;

    Area(int v1, int v2) : val(v1), val2(v2) {}

    Area(const Area &a) : val(a.val), val2(a.val) {}

    Area &operator=(const Area &a) noexcept {
        return *this;
    }

    Area(Area &&a) noexcept: val(a.val), val2(a.val2) {}

    Area &operator=(A &&a) noexcept {
        return *this;
    }

    ~Area() {
        printf("Area has been deleted.");
    }
};

/*  make_Unique的测试函数
int main() {
    auto uptr = make_Unique<Area>(1, 2);
    printf("%d %d\n", uptr->val, uptr->val2);

    return 0;
}
*/

// 支持迭代有以下的条件要求:
// 1. 拥有begin和end函数,返回值是迭代器,分别指向第一个元素和最后一个元素
// 2. 迭代器本身支持*,++,!= 运算符
template<typename V, typename T>
class Iter {
public:
    Iter(V* arr): arr_(arr) {}
    Iter(V* arr, size_t N): arr_(arr), idx_(N) {}

    T &operator*() {
        return arr_->data_[idx_];
    }

    Iter &operator++() {
        idx_ ++;
        return *this;
    }

    bool operator!=(const Iter &rhs) {
        return rhs.idx_ != this->idx_;
    }

private:
    size_t idx_{0};
    V *arr_;
};


template<typename T, size_t L>
class array {
public:
    using iterator = Iter<array<T, L>, T>;

public:
    T data_[L];

    size_t size() {
        return L;
    }

    const T &operator[](const auto i) {
        if (i < size()) {
            return data_[i];
        }
        throw std::logic_error("index out of range");
    }

    iterator begin() {
        return iterator(this, 0);
    }

    iterator end() {
        return iterator(this, L);
    }

};

/* main function for array and iterator
int main() {
    array<int, 4> arr = {1, 2, 3, 5};
    printf("%d", arr[1]);

//    for (auto item: arr) {
//        printf("%d ", item);
//    }

    return 0;
}
*/

// 用户自定义推导指引
// explicit说明符（可选）模板名（形参声明子句） -> 简单模板标识
// common_type_t 确定所有类型 T...的共用类型，即所有T...均能隐式转换到的类型
template<typename... Args>
array(const Args... args) -> array<std::common_type_t<Args...>, sizeof...(args)>;
/* 模板参数自动推导 测试主函数
int main() {
    array arr = {1, 3, 5, 8, 9, 10, 25};
    for (auto item: arr) {
        printf("%d ", item);
    }

    return 0;
}
*/


// C++17的递归实现
void print() {}

template<typename First, typename... Rest>
void print(const First &first, const Rest&... rest) {
    std::cout << first << " ";
    print(rest...);
}

// C++20的实现
template<typename... Args>
void print_cxx20_(const Args&... rest) {
    // 折叠表达式
    ((std::cout << rest << " "), ...);
}

/* print 的测试函数
int main() {
    print_cxx20_("hello", 2.0, "world", 3.1415826);
    return 0;
}
*/

// 万能引用与完美转发
// 对于具体的引用，int && 是右值引用，int & 是左值引用
// 对于模板参数 T && 是万能引用
void process(int &i) {
    printf("%d lvalue\n", i);
}

void process(int &&i) {
    printf("%d rvalue\n", i);
}

template <typename T>
T &&Forward(T &&v) {
    // 不完美转发，丢失了引用的原本信息
    // process(v);

    // T & && -> T &
    // T && && -> T &&
    return static_cast<T &&>(v);
}

template <typename T>
T &&Forward(T &v) {
    // 不完美转发，丢失了引用的原本信息
    // process(v);

    // T & && -> T &
    // T && && -> T &&
    return static_cast<T &&>(v);
}

template <typename T>
void test(T &&v) {
    // 不完美转发，丢失了引用的原本信息
    // process(v);

    // T & && -> T &
    // T && && -> T &&
    process(Forward<T>(v));
}

/* Forward<>() 的测试函数
int main() {
    int ii;
    test(ii);   // 左值引用
    test(1);    // 左值引用
    return 0;
}
*/

template<typename T>
struct integral_const {
    using type = T;
};

template<typename T>
struct remove_ref: integral_const<T> {};

template<typename T>
struct remove_ref<T &>: integral_const<T> {};

template<typename T>
struct remove_ref<T &&>: integral_const<T> {};

template<typename T>
using remove_ref_t = typename remove_ref<T>::type;

//template <typename T>
//T &&Move(T &v) {
//    // move的本质是将所有引用强制转换为右值引用
//    // 左值变为右值引用
//    return static_cast<remove_ref_t<T> &&>(v);
//}

template <typename T>
remove_ref_t<T> &&Move(T &&v) {
    // move的本质是将所有引用强制转换为右值引用
    return static_cast<remove_ref_t<T> &&>(v);
}

/* Forward<>() 的测试函数
int main() {
    int ii;
    process(Move(ii));
    process(Move(1));
    return 0;
}
*/


// std::tie的实现
template <typename... Args>
std::tuple<Args&...> Tie(Args&... args) {
    return {args...};
}

/* tie 的测试函数
int main() {
    int iValue = 0;
    bool bValue = false;
    // 1. 返回一个左值引用的tuple
    auto t1 = Tie(iValue, bValue);
    // auto t2 = Tie(1, false);    // false只接收左值引用
    std::get<1>(t1) = true;

    // 2. 可用于解包tuple
    std::tuple t2{false, 24};
    std::tie(std::ignore, iValue) = t2;
    printf("%d", iValue);

    return 0;
}
*/

// 以if constexpr开始的语句被称为constexpr if语句
// if constexpr 等同于编译期的宏
template<typename T>
T Fabs(const T &a) {
    return a > 0 ? a : -a;
}


template<typename T>
bool is_zero(T v) {
    if (std::is_same<T, float>::value) {
        return (Fabs(v) < FLT_EPSILON);
    } else if (std::is_same<T, double>::value) {
        return (Fabs(v) < DBL_EPSILON);
    }
    return v == 0;
}

template<typename T, typename U>
struct is_Same {
    static constexpr bool value = false;
};

template<typename T>
struct is_Same<T, T> {
    static constexpr bool value = true;
};

template<typename T, typename U>
bool is_Same_v = is_Same<T, U>::value;

/* is_same_v<> 测试函数
int main() {
    if (is_Same_v<int, decltype(1.0)>) {
        printf("hello");
    }
    return 0;
}
*/

// conditional 若b在编译时为true，则定义为T，若b为false则定义为F
template<bool b, typename T, typename F>
struct Conditional {
    // 形式表达
    // using type = b ? T : F;
    using type = T;
};

template<typename T, typename F>
struct Conditional<true, T, F> {
    using type = T;
};

template<typename T, typename F>
struct Conditional<false, T, F> {
    using type = F;
};

struct B {
    void print() {
        printf("B\n");
    }
};

struct C {
    void print() {
        printf("C\n");
    }
};

template<bool b, typename T, typename F>
using Conditional_t = typename Conditional<b, T, F>::type;

template<bool b>
struct D: Conditional_t<b, B, C> {};

/* conditional<> 的测试函数
int main() {
    using T1 = Conditional_t<true, B, C>;
    using T2 = Conditional_t<false, B, C>;
    D<true>{}.print();
    D<false>{}.print();
}
*/

// 参考实现python的enumerate -> [index, element]
// std::declval 构造一个右值引用用以推断类型，但不调用函数本身
template<typename T,
        typename TIter = decltype(std::begin(std::declval<T>())),
        typename = decltype(std::end(std::declval<T>()))>
constexpr auto enumerate(T &&iterable) {
    struct iterator {
        size_t i;
        TIter iter;
        bool operator!= (const iterator &other) const {
            return iter != other.iter;
        }

        void operator++() {
            ++ i;
            ++iter;
        }

        auto operator*() const {
            return Tie(i, *iter);
        }
    };

    struct iterable_wrapper {
        T iterable;
        auto begin() {
            return iterator{0, std::begin(iterable)};
        }

        auto end() {
            return iterator{0, std::end(iterable)};
        }
    };

    return iterable_wrapper{Forward<T>(iterable)};
}

/* enumerate 的主函数
int main() {
    std::array arr = {1, 2, 4};

    for (const auto [i, v] : enumerate(arr)) {
        printf("%d %d \n", i, v);
    }

    return 0;
}
*/

// SFINAE(Substitution Failure Is Not An Error) 替换失败并非错误
// enable_if
template<bool, typename T>
struct enable_if {
    using type = T;
};

template<typename T>
struct enable_if<false, T> {};

/* enable_if 测试函数
int main() {
    using T = enable_if<true, int>::type;
    using T2 = enable_if<false, int>::type;
}
*/

