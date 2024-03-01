#include <coroutine>
#include <utility>
#include <iostream>

struct Generator {
    struct promise_type {
        Generator get_return_object() noexcept {
            return Generator{ *this };
        }

        /**
         * @brief generatorֻ������Ҫ��ʱ��Ż���м��㣬�����������Ƿ���std::suspend_always
         * @return 
         */
        std::suspend_always initial_suspend() noexcept {
            return {};
        }

        /**
         * @brief ������Ҫ�������ķ���ֵ�����Է���std::suspend_always
         * @return 
         */
        std::suspend_always final_suspend() noexcept {
            return {};
        }

        void unhandled_exception() noexcept {
            // �����쳣ָ��
            exceptionPtr = std::current_exception();
        }

        /**
         * @brief �ڻ�ȡco_yield��ֵ�����Ǽ�������Э��
         * @param v 
         * @return 
         */
        std::suspend_always yield_value(int v) noexcept {
            returnValue = v;
            return {};
        }

        /**
         * @brief ������ʱ����Ҫ����ֵ
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
     * @brief Generator�ĵ�������ÿ��++�����Ǿͻ����һ�μ���
     *        �����þͿ��Ի�ȡ���һ�ε�co_yield��ֵ
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
            // �ָ�Э�̼���ִ��Э�̺���
            coroutineHandle.resume();
            // ���ִ����ϣ����coroutine_handler�ÿ�
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
            // ����coroutine_handle�Ƚϵ���Э��֡�ĵ�ַ
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
        // Ĭ�Ϲ����coroutine_handle��Э��֡Ϊnullptr
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
        // ���м����������
        co_yield a3;
    }
}

int main(int argc, const char** argv) {
    Generator fibonacci = Fibonacci();
    auto iter = fibonacci.begin();
    std::cout << "��1��" << *iter << std::endl;
    ++iter;
    std::cout << "��2��" << *iter << std::endl;
    ++iter;
    std::cout << "��3��" << *iter << std::endl;
    ++iter;
    std::cout << "��4��" << *iter << std::endl;
    ++iter;
    std::cout << "��5��" << *iter << std::endl;
    std::cout << "�������ˣ���Ϣһ�°�..." << std::endl;
    // ��������
    ++iter;
    std::cout << "��6��" << *iter << std::endl;
    ++iter;
    std::cout << "��7��" << *iter << std::endl;
    //... and so on
	return 0;
}