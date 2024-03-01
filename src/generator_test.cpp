#include <coroutine>
#include <utility>
#include <iostream>

struct Generator {
    struct promise_type {
        Generator get_return_object() noexcept {
            return Generator{ *this };
        }

        /**
         * @brief generator只有在需要的时候才会进行计算，所以这里我们返回std::suspend_always
         * @return 
         */
        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        /**
         * @brief 我们需要函数最后的返回值，所以返回std::suspend_always
         * @return 
         */
        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void unhandled_exception() noexcept {
            // 保存异常指针
            exceptionPtr = std::current_exception();
        }

        /**
         * @brief 在获取co_yield的值后，我们继续挂起协程
         * @param v 
         * @return 
         */
        std::suspend_always yield_value(int v) noexcept {
            returnValue = v;
            return {};
        }

        /**
         * @brief 例子暂时不需要返回值
         */
        void return_void() noexcept {}

        void await_transform() = delete;

        void rethrow_if_exception() {
	        if (exceptionPtr) {
		        std::rethrow_exception(exceptionPtr);
	        }
        }

        int returnValue;
        std::exception_ptr exceptionPtr;
    };

    /**
     * @brief Generator的迭代器，每次++，我们就会进行一次计算
     *        解引用就可以获取最近一次的co_yield的值
     */
    struct iterator {
        using iterator_category = std::input_iterator_tag;
        using difference_type = ptrdiff_t;
        using value_type = int;
        using reference = const int&;
        using pointer = const int*;

        iterator() = default;
        explicit iterator(std::coroutine_handle<promise_type> acoroutineHandle) noexcept
    		: coroutineHandle(acoroutineHandle) {
        }

        iterator& operator++() {
            // 恢复协程继续执行协程函数
            coroutineHandle.resume();
            // 如果执行完毕，则把coroutine_handler置空
            if (coroutineHandle.done()) {
                auto oldHandle = std::exchange(coroutineHandle, nullptr);
                oldHandle.promise().rethrow_if_exception();
            }

            return *this;
        }

        void operator++(int) {
            ++* this;
        }

        bool operator==(const iterator& rhs) const noexcept {
            // 两个coroutine_handle比较的是协程帧的地址
            return coroutineHandle == rhs.coroutineHandle;
        }

        bool operator!=(const iterator& rhs) const noexcept {
            return !(*this == rhs);
        }

        reference operator*() const noexcept {
            return coroutineHandle.promise().returnValue;
        }

        pointer operator->() const noexcept {
            return &coroutineHandle.promise().returnValue;
        }

        std::coroutine_handle<promise_type> coroutineHandle = nullptr;
    };

    iterator begin() {
        if (m_coroutineHandle) {
            m_coroutineHandle.resume();
            if (m_coroutineHandle.done()) {
                m_coroutineHandle.promise().rethrow_if_exception();
                return {};
            }
        }

        return iterator{ m_coroutineHandle };
    }

    iterator end() noexcept {
        // 默认构造的coroutine_handle的协程帧为nullptr
        return {};
    }

    explicit Generator(promise_type& promise) noexcept
		: m_coroutineHandle(std::coroutine_handle<promise_type>::from_promise(promise)) {
    }

    Generator() = default;

    Generator(Generator&& other) noexcept
		: m_coroutineHandle(std::exchange(other.m_coroutineHandle, nullptr)) {
    }

    Generator& operator=(Generator&& other) noexcept {
        m_coroutineHandle = std::exchange(other.m_coroutineHandle, nullptr);
        return *this;
    }

    ~Generator() {
        if (m_coroutineHandle) {
            m_coroutineHandle.destroy();
        }
    }

private:
    std::coroutine_handle<promise_type> m_coroutineHandle = nullptr;
};

Generator Fibonacci() {
	int a1 = 1;
    int a2 = 1;

    co_yield a1;
    co_yield a2;

    for (int i = 0; i < 20; ++i) {
	    int a3 = a1 + a2;
        a1 = a2;
        a2 = a3;
        // 将中间计算结果返回
        co_yield a3;
    }
}

int main(int argc, const char** argv) {
    Generator fibonacci = Fibonacci();
    auto iter = fibonacci.begin();
    std::cout << "第1项" << *iter << std::endl;
    ++iter;
    std::cout << "第2项" << *iter << std::endl;
    ++iter;
    std::cout << "第3项" << *iter << std::endl;
    ++iter;
    std::cout << "第4项" << *iter << std::endl;
    ++iter;
    std::cout << "第5项" << *iter << std::endl;
    std::cout << "计算累了，休息一下吧..." << std::endl;
    // 继续计算
    ++iter;
    std::cout << "第6项" << *iter << std::endl;
    ++iter;
    std::cout << "第7项" << *iter << std::endl;
    //... and so on
	return 0;
}