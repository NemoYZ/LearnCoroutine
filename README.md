<div align='center'> <h1> C++20协程 </h1> </div>

## 一、协程介绍

#### 协程定义

协程是可以挂起和恢复的函数。

#### 协程分类

##### 按调用方式

按照协程调用方式，可分为对称式协程(symmetric coroutine)和非对称式协程(asymmetric coroutine)。

对称式协程在挂起后可以不用返回到调用者(caller)，也就是不用恢复到协程挂起的地方；非对称式协程在从挂起恢复后，会返回到挂起该协程的地方，这和我们平时接触的函数调用方式是一致的，所以非对称式协程使用的更多。

##### 按上下文的存储方式

按照存储上下文的方式，可分为有栈协程(stackful)和无栈(stackless)协程.。有栈协程会申请一块内存，叫做协程栈，来保护当前函数执行的上下文(寄存器，调用栈等)；无栈协程也会申请一块内存，叫做协程帧， 用于保存函数参数和局部变量。

相较于有栈协程，无栈协程不用保存调用栈，而且保存的寄存器也比有栈协程少(主要是函数参数和局部变量)。所以显然无栈协程内存使用比有栈协程低，执行挂起和恢复操作也比有栈协程快。

## 二、C++的协程

C++20的协程属于非对称式，无栈协程。

C++20协程相关的类在头文件<coroutine>里

#### 协程关键字

##### co_await

co_await为一元操作符，表示"等待"一个协程任务。一般作用于Awaitable对象上。

##### co_yield

co_yield用于暂停当前的协程并返回(包括void)一个值，可通过resume继续执行。

##### co_return

co_return用于停止当前协程并返回(包括void)一个值。区别于co_yield，不能再次恢复执行。注意，如果协程函数里没写co_return，那么编译器会帮我们在最后加上co_return，也就是协程返回值为void。

#### 协程相关类

标准库已经为我们准备好的类

| 类                   		 | 解释                                 |
| ---------------------------| ------------------------------------ |
| ```std::suspend_never;```  | 表示不会挂起，在promise_type里会用到 |
| ```std::suspend_always;``` | 表示要挂起，在promise_type里会用到   |
| ```std::coroutine_handle;``` | 用于操作协程的类，可用于把挂起的协程恢复，释放协程帧 |



##### promise_type

promise_type用于定义协程的行为。定义了协程创建的行为，包括创建方式、协程初始化完成和结束时的行为、发生异常时的行为。但这些行为包括promise_type都需要我们自己定义，编译器只负责告诉我们何时发生这些行为。

promise_type可定义的成员函数如下(不用全定义)，以下的auto一般为std::suspend_never或者std::suspend_always; T为泛型，可能是值/左值引用/右值引用类型。

| 成员函数                			| 解释                              |
| ----------------------------------| --------------------------------- |
| ```auto initial_suspend();``` | 协程初始化完成后是否挂起。如果返回std::suspend_never表示不挂起，std::suspend_always表示挂起 |
| ```auto final_suspend();``` | 协程执行完毕(co_return)后是否挂起。如果返回std::suspend_never表示不挂起，std::suspend_always表示挂起 |
| ```T get_return_object();```  	| 返回给调用者(caller)一个对象，这个对象一般有个std::coroutine_handle<Promise>的成员变量 |
| ```void unhandled_exception();``` | 发生异常时调用 |
| ```void return_void();```			| co_return时被调用 |
| ```void return_value(T);``` 		| co_return xxx时被调用，保存协程返回的值 |
| ```auto yield_value(T);``` | co_yield xxx时调用，xxx会被当做参数传给该函数 |
| ```T await_transform(T);``` | 用于定义协程函数里co_await xxx;语句的行为。定义该函数后，编译器会把每个co_await xxx;转为co_await promise.await_transform(xxx); |

##### std::suspend_always/std::suspend_never

suspend_always和suspend_never里定义了三个成员函数，用于告诉编译器当前协程在初始化完成后的操作，挂起时的操作，恢复时的操作。更多解释如下

| 成员函数                                		| 解释 |
| ----------------------------------------------| ---- |
| ```bool await_ready();```                     | 是否可以挂起当前协程，返回false表示不挂起，true表示可挂起。 |
| ```void/bool await_suspend(coroutine_handle<>);``` | 协程被挂起后调用，如果返回的是void或者true，则执行权会交还给caller<br>如果返回false，则直接调用await_resume |
| ```void await_resume();```                   	| 在co_await恢复之前调用 |

##### std::coroutine_handle<Promise = void>

操作协程的类，协程句柄(句柄真的是一个翻译的败笔)。可访问协程帧，将协程从挂起中恢复，释放协程帧(协程执行完毕时)。

不同于promise_type，标准库已经帮我们定义好了coroutine_handle的操作，我们直接调用就OK了。

对于coroutine_handle<Promise = void>，有如下操作

| 成员函数                                                   		| 解释                                       |
| ------------------------------------------------------------------| ------------------------------------------ |
| ```std::coroutine_handle<Promise> from_promise();```       		| 从promise_type对象创建一个coroutine_handle |
| ```bool done();```                                               	| 协程是否执行完毕(调用co_return)            |
| ```operator bool();```                                           	| 是否为一个有效的coroutine_handle           |
| ```operator();```                                             	| 恢复协程的执行                             |
| ```void resume();```                                             	| 同operator()，恢复协程执行                 |
| ```void destroy();```                                            	| 销毁协程帧                                 |
| ```Promise& promise();```                                        	| 获取协程持有的promise_type对象             |
| ```void* address();```                                           	| 获取协程帧的地址                           |
| ```static std::coroutine_handle<Promise> from_address(void*);``` 	| 从协程帧创建一个coroutine_handle           |

## 三、协程例子

先定义一些函数/宏，方便我们debug

PRETTY_FUNCTION_NAME会被编译为人类可读的函数名字符串，相较于\__FUNCTION__，PRETTY_FUNCTION_NAM不是mangle后的字符串。 便于我们查看被调用函数的名字。

```c++
#ifdef _MSC_VER
#define PRETTY_FUNCTION_NAME __FUNCSIG__
#else
#define PRETTY_FUNCTION_NAME __PRETTY_FUNCTION__
#endif
```

##### 例子一：promise_type的使用

定义promise_type

见 [promise_type_test.cpp](src/promise_type_test.cpp)

```c++
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
		 * @brief 不挂起，直接在协程创建时就执行协程函数
		 * @return 
		 */
		std::suspend_never initial_suspend() {
			std::cout << PRETTY_FUNCTION_NAME << std::endl;
			return {};
		}
		/**
		 * @brief 协程执行完先挂起，等我们获取到返回值后再销毁协程帧
		 */
		std::suspend_always final_suspend() noexcept {
			std::cout << PRETTY_FUNCTION_NAME << std::endl;
			return {};
		}
		/**
		 * \brief 协程返回时的返回值传给return_value
		 * \param v 协程函数返回值
		 */
		void return_value(int v) {
			// 保存协程的返回值
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
```

输出结果如下:

```
Task::promise_type::promise_type(void) noexcept
class Task Task::promise_type::get_return_object(void)
struct std::suspend_never Task::promise_type::initial_suspend(void)
class Task foo1(void)
void Task::promise_type::return_value(int). get return value=1
struct std::suspend_always Task::promise_type::final_suspend(void) noexcept
task1 returned 1
Task::~Task(void) destroy coroutine
```

因为我们的initial_suspend返回的是std::suspend_never，所以在main函数第一行创建task1对象后就会执行协程函数而不是挂起task1，随后协程执行完毕后，我们再打印foo1的返回值，最后task1对象析构时，一起销毁coroutine_handle。

##### 例子二：generator

generator类可以用于一些计算问题，比如一步一步执行算法。

比如我想知道DFS遍历图的过程，generator就可以帮助我们实现这个功能。

还比如我想计算fibonacci数列的第n项，但刚开始我只需要前面几项，随着需求增加，我需要越来越后面的fibonacci数。这时候generator就非常有用。

来源：MSVC头文件<expermental/generator> 见 [generator_test.cpp](src/generator_test.cpp)

```c++
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
```

输出结果如下:

```c++
第1项1
第2项1
第3项2
第4项3
第5项5
计算累了，休息一下吧...
第6项8
第7项13
```

可以看到, 只有在iter初始化以及++iter时，我们才会得到下一个fibonacci数。即执行了计算。如果我们暂时不需要执行计算时，完全可以去做一些别的事情，等需要的时候再继续执行计算。

注意，这个generator只是一个例子。如果想看完整的实现，可看gcc的标准库实现:  [gcc的github链接](https://github.com/gcc-mirror/gcc)，把项目拉下来后打开 listdc++-v3/include/std/generator即可。

##### 例子三：Lazy

lazy可用于延迟计算，我们在创建好协程对象的时候不会立即计算，而是在我们需要计算结果的时候再去获取计算结果。

见 [lazy_test.cpp](src/lazy_test.cpp)

```c++
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
```

输出结果如下:

```
task was created
wait task
waiting for this operation to be finished
resumed
```

可以看到，在resume的时候才进行了计算