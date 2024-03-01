#include <coroutine>
#include <iostream>
#include <thread>

class Task {
public:
	struct promise_type {
		friend class Awaitable;
		struct Awaitable {
			bool await_ready() const noexcept {
				return false;
			}
			template<typename PromiseType>
			void await_suspend(std::coroutine_handle<PromiseType> coroutineHandle) noexcept {
				// a long operation
				std::cout << "waiting for this operation to be finished" << std::endl;
				std::this_thread::sleep_for(std::chrono::seconds(5));
				coroutineHandle.resume();
			}
			void await_resume() {}
		};
		Task get_return_object() {
			return Task{*this};
		}
		std::suspend_always initial_suspend() noexcept {
			return {};
		}
		std::suspend_never final_suspend() noexcept {
			return {};
		}
		void return_void() {}
		void unhandled_exception() {}
	};

	Task() = default;
	Task(std::coroutine_handle<promise_type> coroutineHandle) noexcept
		: m_coroutineHandle(coroutineHandle) {
	}
	explicit Task(promise_type& promise) noexcept
		: m_coroutineHandle(std::coroutine_handle<promise_type>::from_promise(promise)) {
	}
	~Task() {
		// 对于suspend_never来说，编译器会帮我们释放掉协程帧
		// 这里就不需要再调用m_coroutineHandle.destroy()了
	}

	std::coroutine_handle<promise_type>& get_coroutine_handle() {
		return m_coroutineHandle;
	}

	auto operator co_await() const noexcept {
		return promise_type::Awaitable{};
	}

private:
	std::coroutine_handle<promise_type> m_coroutineHandle;
};

Task Foo() {
	co_return;
}

Task WaitFoo() {
	Task task = Foo();
	std::cout << "wait task" << std::endl;
	co_await task;
	std::cout << "resumed" << std::endl;
	co_return;
}

int main(int argc, const char** argv) {
	Task task = WaitFoo();
	std::cout << "task was created" << std::endl;
	// 执行
	task.get_coroutine_handle().resume();

	return 0;
}