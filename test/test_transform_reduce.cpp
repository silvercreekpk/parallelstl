/*
    Copyright (c) 2017-2018 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.




*/

// Tests for inner_product

#include "pstl/execution"
#include "pstl/numeric"
#include "test/utils.h"
#include "pstl/internal/numeric_impl.h" //for usage a serial algo version

using namespace TestUtils;

// Equal for all types
template <typename T>
static bool Equal(T x, T y) { return x==y; }

// Functor for xor-operation for modeling binary operations in inner_product
class XOR {
public:
    template <typename T>
    T operator()(const T& left, const T& right) const {
        return left^right;
    }
};

// Model of User-defined class
class MyClass {
public:
    int32_t my_field;
    MyClass() { my_field = 0; }
    MyClass(int32_t in) { my_field = in; }
    MyClass(const MyClass& in) { my_field = in.my_field; }

    friend MyClass operator+(const MyClass& x, const MyClass& y) {
        return MyClass(x.my_field + y.my_field);
    }
    friend MyClass operator-(const MyClass& x) {
        return MyClass(-x.my_field);
    }
    friend MyClass operator*(const MyClass& x, const MyClass& y) {
        return MyClass(x.my_field * y.my_field);
    }
    bool operator==(const MyClass& in) {
        return my_field == in.my_field;
    }
};

template <typename T>
void CheckResults(const T& expected, const T& in) {
    EXPECT_TRUE(Equal(expected, in), "wrong result of transform_reduce");
}

// We need to check correctness only for "int" (for example) except cases
// if we have "floating-point type"-specialization
void CheckResults(const float32_t& expected, const float32_t& in) {
}

// Test for different types and operations with different iterators
struct test_transform_reduce {
    template <typename Policy, typename InputIterator1, typename InputIterator2, typename T, typename BinaryOperation1, typename BinaryOperation2, typename UnaryOp>
    void operator()(Policy&& exec, InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, InputIterator2 last2 ,
        T init, BinaryOperation1 opB1, BinaryOperation2 opB2, UnaryOp opU) {

        auto expectedB = std::inner_product(first1, last1, first2, init, opB1, opB2);
        auto expectedU = __pstl::internal::brick_transform_reduce(first1, last1, init, opB1, opU, std::false_type());
        T resRA = std::transform_reduce(exec, first1, last1, first2, init, opB1, opB2);
        CheckResults(expectedB, resRA);
        resRA = std::transform_reduce(exec, first1, last1, init, opB1, opU);
        CheckResults(expectedU, resRA);
    }
};


template <typename T, typename BinaryOperation1, typename BinaryOperation2, typename UnaryOp, typename Initializer>
void test_by_type(T init, BinaryOperation1 opB1, BinaryOperation2 opB2, UnaryOp opU, Initializer initObj) {

    std::size_t maxSize = 100000;
    Sequence<T> in1(maxSize, initObj);
    Sequence<T> in2(maxSize, initObj);

    for (std::size_t n = 0; n < maxSize; n = n < 16 ? n + 1 : size_t(3.1415 * n)) {
        invoke_on_all_policies(test_transform_reduce(), in1.begin(), in1.end(), in2.begin(), in2.end(), init, opB1, opB2, opU);
        invoke_on_all_policies(test_transform_reduce(), in1.cbegin(), in1.cend(), in2.cbegin(), in2.cend(), init, opB1, opB2, opU);
    }
}

int32_t main() {
    test_by_type<int32_t>(42,
        std::plus<int32_t>(),
        std::multiplies<int32_t>(),
        std::negate<int32_t>(),
        [](std::size_t a)->int32_t {return int32_t(rand() % 1000); });
    test_by_type<int64_t>(0,
        [](const int64_t& a, const int64_t& b) -> int64_t { return a | b; },
        XOR(),
        [](const int64_t& x) -> int64_t { return x * 2; },
        [](std::size_t a)->int64_t {return int64_t(rand() % 1000); });
    test_by_type<float32_t>(1.0f,
        std::multiplies<float32_t>(),
        [](const float32_t& a, const float32_t& b) -> float32_t { return a + b; },
        [](const float32_t& x) -> float32_t { return x + 2; },
        [](std::size_t a)->float32_t {return rand() % 1000; });
    test_by_type<MyClass>(MyClass(),
        std::plus<MyClass>(),
        std::multiplies<MyClass>(),
        std::negate<MyClass>(),
        [](std::size_t a)->MyClass {return MyClass(rand() % 1000); });

    std::cout << "done" << std::endl;
    return 0;
}
