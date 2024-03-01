#include <coroutine>
#include <iostream>
#include <thread>

#ifdef _MSC_VER
#define PRETTY_FUNCTION_NAME __FUNCSIG__
#else
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#endif

class Task {
public:
	struct promise_type {
		promise_type() noexcept {
			std::cout << PRETTY_FUNCTION_NAME << std::endl;
		}

		Task get_return_object() {
			std::cout << PRETTY_FUNCTION_NAME << std::endl;
			return Task{std::coroutine_handle<Task::promise_type>::from_promise(*this)};
		}

		/**
		 * @brief ������ֱ����Э�̴���ʱ��ִ��Э�̺���
		 * @return 
		 */
		std::suspend_never initial_suspend() {
			std::cout << PRETTY_FUNCTION_NAME << std::endl;
			return {};
		}

		/**
		 * @brief Э��ִ�����ȹ��𣬵����ǻ�ȡ������ֵ��������Э��֡
		 */
		std::suspend_always final_suspend() noexcept {
			std::cout << PRETTY_FUNCTION_NAME << std::endl;
			return {};
		}

		/**
		 * \brief Э�̷���ʱ�ķ���ֵ����return_value
		 * \param v Э�̺�������ֵ
		 */
		void return_value(int v) {
			// ����Э�̵ķ���ֵ
			returnValue = v;
			std::cout << PRETTY_FUNCTION_NAME << ". get return value=" << v << std::endl;
		}

		void unhandled_exception() {
			std::exception_ptr eptr = std::current_exception();
			std::cout << PRETTY_FUNCTION_NAME << std::endl;
			std::rethrow_exception(eptr);
		}

		int returnValue = 0;
	};

	Task() = default;
	Task(std::coroutine_handle<Task::promise_type>&& handle)
		: m_handle(std::move(handle)) {
	}
	~Task() {
		std::cout << PRETTY_FUNCTION_NAME;
		if (m_handle) {
			std::cout << " destroy coroutine" << std::endl;
			m_handle.destroy();
		}
	}

	std::coroutine_handle<Task::promise_type>& get_coroutine_handle() noexcept {
		return m_handle;
	}

private:
	std::coroutine_handle<Task::promise_type> m_handle;
};

Task foo1() {
	std::cout << PRETTY_FUNCTION_NAME << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(5));
	co_return 1;
}

int main(int argc, const char** argv) {
	Task task1 = foo1();
	std::cout << "task1 returned " << task1.get_coroutine_handle().promise().returnValue << std::endl;

	return 0;
}