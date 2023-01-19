#include "Common.h"
#include <array>
#include <atomic>
#include <thread>
#include <mutex>
#include <algorithm>
#include <tuple>
#include <sstream>


template <typename T>
struct RC;


// Base class for coroutine promise type.
struct AsyncTask
{
    friend struct TaskSystem;

    template <typename T>
    friend struct RC;

protected:
    using Deps_t = std::vector< RC<AsyncTask> >;

    enum class Status : unsigned
    {
        Initial,
        InQueue,
        InProgress,
        Completed,
    };

protected:
    std::atomic<int>    _refCount   {0};
    std::atomic<Status> _status     {Status::Initial};

    std::mutex          _depsGuard;
    Deps_t              _deps;

public:
    [[nodiscard]] bool  is_complete ()      const   { return _status.load() == Status::Completed; }
    [[nodiscard]] bool  has_dependencies ()         { std::scoped_lock lock {_depsGuard};  return not _deps.empty(); }

    // Returns pointer to coroutine as string.
    // Can be used for debugging.
    [[nodiscard]] std::string  name ()     const;

    template <typename Arg0, typename ...Args>
    void  add_dependencies (Arg0&& arg0, Args&& ...deps);
    void  add_dependency (AsyncTask* dep);

protected:
    virtual ~AsyncTask () { assert( _refCount.load() == 0 ); }

    // Execute task/corotine.
    virtual void  run () = 0;

    // Internally calls destructor.
    virtual void  release () = 0;
};


// Implements reference counting pattern.
template <typename T>
struct RC
{
private:
    T*   _ptr    = nullptr;

public:
    RC ()                                                    {}
    RC (std::nullptr_t)                                      {}
    RC (T* ptr) : _ptr{ptr}                                  { _Inc( _ptr ); }
    RC (const RC &other) : _ptr{other._ptr}                  { _Inc( _ptr ); }
    RC (RC && other) : _ptr{other.detach()}                  {}
    ~RC ()                                                   { _Dec(); }
        
    RC&  operator = (std::nullptr_t)                         {                     _Dec();  _ptr = nullptr;       return *this; }
    RC&  operator = (T* rhs)                                 { _Inc( rhs );        _Dec();  _ptr = rhs;           return *this; }
    RC&  operator = (const RC &rhs)                          { _Inc( rhs.get() );  _Dec();  _ptr = rhs.get();     return *this; }
    RC&  operator = (RC && rhs)                              {                     _Dec();  _ptr = rhs.detach();  return *this; }

    template <typename A>
    RC&  operator = (const RC<A> &rhs)                       { _Inc( rhs.get() );  _Dec();  _ptr = rhs.get();     return *this; }

    [[nodiscard]] T*  get () const                           { return _ptr; }
    [[nodiscard]] T*  detach ()                              { T* tmp = _ptr;  _ptr = nullptr;  return tmp; }

    [[nodiscard]] T*  operator -> () const                   { assert( _ptr != nullptr );  return _ptr; }
    [[nodiscard]] T&  operator *  () const                   { assert( _ptr != nullptr );  return *_ptr; }
        
    [[nodiscard]] bool  operator == (std::nullptr_t)   const { return _ptr == nullptr; }
    [[nodiscard]] bool  operator == (const RC<T> &rhs) const { return _ptr == rhs._ptr; }
    [[nodiscard]] bool  operator == (const T* rhs)     const { return _ptr == rhs; }

    template <typename A>
    [[nodiscard]] bool  operator != (const A &rhs)     const { return not (*this == rhs); }

    explicit operator bool () const                          { return _ptr != nullptr; }

private:
    static void  _Inc (T* ptr)
    {
        if ( ptr != nullptr )
            ptr->_refCount.fetch_add(1);
    }

    void  _Dec ()
    {
        if ( _ptr != nullptr )
        {
            const int   cnt = _ptr->_refCount.fetch_sub(1);
            assert( cnt > 0 );

            if ( cnt == 1 )
            {
                static_cast<AsyncTask *>(_ptr)->release();
                _ptr = nullptr;
            }
        }
    }
};


// Add input dependency to current task/coroutine.
// Task must be added back to task queue to wait until all input dependecy are complete.
inline void  AsyncTask::add_dependency (AsyncTask* dep)
{
    assert( _status.load() == Status::InProgress ); // if inside coroutine
    
    std::scoped_lock  lock {_depsGuard};
    _deps.push_back( RC<AsyncTask>{ dep });
}


template <typename Arg0, typename ...Args>
void  AsyncTask::add_dependencies (Arg0&& arg0, Args&& ...args)
{
    add_dependency( arg0 );

    if constexpr( sizeof...(Args) > 0 )
    {
        add_dependencies( std::forward<Args>(args) ... );
    }
}


// Task contains pointer to AsyncTask or croutine.
template <typename T = void>
struct Task;


// Task with return value, implements 'promise' pattern.
template <typename T>
struct Task
{
    struct promise_type;
    using handle_t = std::coroutine_handle< promise_type >;

    struct promise_type final : public AsyncTask
    {
    private:
        T   _result;

    public:
        promise_type ()     {}

        Task<T>             get_return_object ()        { return Task<T>{ handle_t::from_promise( *this )}; }
        std::suspend_always initial_suspend () noexcept { return {}; }        // delayed start
        std::suspend_always final_suspend () noexcept   { return {}; }        // avoid 'suspend_never', it immediatly destroy coroutine
        void                return_value (T value)      { _result = std::move(value); }
        void                unhandled_exception ()      {}

        [[nodiscard]] auto  get_rc ()                   { return RC<promise_type>{ this }; }
        [[nodiscard]] T     get_result () const         { assert(is_complete());  return _result; }

    private:
        void  run () override
        {
            assert( _status.load() == Status::InProgress );

            // resume coroutine
            auto    coro = handle_t::from_promise( *this );
            coro.resume();

            // if coroutine is complete - mark as completed, otherwise it will be added back to queue.
            if ( coro.done() )
                _status.store( Status::Completed );
        }

        void  release () override
        {
            // destroy coroutine, it implicitly calls 'promise_type' destructor
            auto    coro = handle_t::from_promise( *this );

            assert( coro.done() );
            coro.destroy();
        }
    };

private:
    RC<promise_type>    _ptr;
        
public:
    Task ()                                         {}
    Task (promise_type* ptr) : _ptr{ptr}            {}
    Task (handle_t h) : _ptr{h.promise().get_rc()}  {}
    ~Task ()                                        {}
    
    [[nodiscard]] bool      is_complete ()          { assert( _ptr );  return _ptr->is_complete(); }
    [[nodiscard]] bool      has_dependencies ()const{ assert( _ptr );  return _ptr->has_dependencies(); }
    [[nodiscard]] auto*     to_promise ()           { assert( _ptr );  return _ptr.get(); }
    [[nodiscard]] handle_t  to_corotine ()          { assert( _ptr );  return handle_t::from_promise( *_ptr ); }

    // Returns copy of result because it can be used many times.
    // Move-ctor requires additional synchronization.
    [[nodiscard]] T         get_result ()   const   { assert( _ptr );  return _ptr->get_result(); }

    explicit operator bool () const                 { return bool(_ptr); }
};


// Task without return value? implements 'task/job' pattern.
template <>
struct Task <void>
{
    struct promise_type;
    using handle_t = std::coroutine_handle< promise_type >;

    struct promise_type final : public AsyncTask
    {
    public:
        promise_type ()     {}

        Task<void>          get_return_object ()        { return Task<void>{ handle_t::from_promise( *this )}; }
        std::suspend_always initial_suspend () noexcept { return {}; }        // delayed start
        std::suspend_always final_suspend () noexcept   { return {}; }        // avoid 'suspend_never', it immediatly destroy coroutine
        void                return_void ()              {}
        void                unhandled_exception ()      {}

        [[nodiscard]] auto  get_rc ()                   { return RC<promise_type>{ this }; }

    private:
        void  run () override
        {
            assert( _status.load() == Status::InProgress );

            auto    coro = handle_t::from_promise( *this );
            coro.resume();

            if ( coro.done() )
                _status.store( Status::Completed );
        }

        void  release () override
        {
            auto    coro = handle_t::from_promise( *this );

            assert( coro.done() );
            coro.destroy();
        }
    };

private:
    RC<promise_type>    _ptr;
        
public:
    Task ()                                         {}
    Task (promise_type* ptr) : _ptr{ptr}            {}
    Task (handle_t h) : _ptr{h.promise().get_rc()}  {}
    ~Task ()                                        {}
    
    [[nodiscard]] bool      is_complete ()          { assert( _ptr );  return _ptr->is_complete(); }
    [[nodiscard]] bool      has_dependencies ()const{ assert( _ptr );  return _ptr->has_dependencies(); }
    [[nodiscard]] auto*     to_promise ()           { assert( _ptr );  return _ptr.get(); }
    [[nodiscard]] handle_t  to_corotine ()          { assert( _ptr );  return handle_t::from_promise( *_ptr ); }
    
    void                    get_result ()   const   {}

    explicit operator bool () const                 { return bool(_ptr); }
};


struct TaskSystem
{
private:
    using Status = AsyncTask::Status;

    // Single task queue for all threads.
    // It is very slow, for real projects use queue per thread and work stealing.
    std::mutex                      _queueGuard;
    std::vector< RC<AsyncTask> >    _queue;

    std::vector< std::thread >      _threads;
    std::atomic<bool>               _looping    {false};

public:
    void  wait ();
    
    template <typename T>
    void  add (Task<T> task);
    void  add (RC<AsyncTask> task);

    static TaskSystem&  instance ();
    static TaskSystem&  create (int threadCount);
    static void         destroy ();

private:
    TaskSystem () {}

    void  init_threads (int count);
    void  process_tasks (size_t seed);

    [[nodiscard]] RC<AsyncTask>  extract_task (size_t first, size_t count);
};


// Used placement new to avoid false positive warning on memleak
inline TaskSystem&  TaskSystem::instance ()
{
    static std::aligned_storage_t< sizeof(TaskSystem), alignof(TaskSystem) >   ts;
    return *static_cast<TaskSystem *>( static_cast<void*>( &ts ));
}

inline TaskSystem&  TaskSystem::create (int threadCount)
{
    auto* ts = new(&instance()) TaskSystem{};
    ts->init_threads( threadCount );
    return *ts;
}

inline void  TaskSystem::destroy ()
{
    instance().~TaskSystem();
}


// Add task/coroutine to the queue.
template <typename T>
inline void  TaskSystem::add (Task<T> task)
{
    return add( RC<AsyncTask>{ task.to_promise() });
}

inline void  TaskSystem::add (RC<AsyncTask> task)
{
    assert( task );

    Status  stat = task->_status.exchange( Status::InQueue );
    assert( stat == Status::Initial or stat == Status::InProgress );

    std::scoped_lock    lock {_queueGuard};
    _queue.push_back( task );
}


// Find task which is ready to be executed and extract it from the queue.
inline RC<AsyncTask>  TaskSystem::extract_task (size_t first, size_t count)
{
    if ( _queue.empty() )
        return {};

    for (size_t i = 0; i < count; ++i)
    {
        size_t  idx     = (i + first) % _queue.size();
        auto&   t       = *_queue[ idx ];
        bool    ready   = true;

        {
            std::scoped_lock  lock2 {t._depsGuard};
            for (auto& dep : t._deps) {
                ready &= dep->is_complete();
            }

            if ( ready )
            {
                Status  stat = t._status.exchange( Status::InProgress );
                assert( stat == Status::InQueue );

                t._deps.clear();
            }
        }

        if ( ready )
        {
            RC<AsyncTask>    task = &t;
            
            if ( idx+1 != _queue.size() )
                _queue[idx] = _queue.back();

            _queue.pop_back();
            return task;
        }
    }

    return {};
}


inline void  TaskSystem::process_tasks (size_t seed)
{
    const size_t  range_size = 8;

    for (size_t i = 0, count = 1; i < count; i += range_size)
    {
        RC<AsyncTask>   t;
        {
            std::scoped_lock    lock {_queueGuard};

            t     = extract_task( i+seed, range_size );
            count = _queue.size();
        }
        if ( t )
        {
            // Execute task/coroutine.
            t->run();

            // If not complete - add back to the queue and wait until new dependencies are complete.
            if ( not t->is_complete() )
            {
                assert( t->has_dependencies() );

                add( t );
            }
        }
    }
}


inline void  TaskSystem::init_threads (int count)
{
    _looping.store( true );

    for (int i = 0, cnt = std::clamp(count, 1, 32); i < cnt; ++i)
    {
        _threads.push_back( std::thread{ [this, i] ()
                            {
                                for (; _looping.load();)
                                {
                                    process_tasks(i);
                                }
                            }});
    }
}


inline void  TaskSystem::wait ()
{
    std::this_thread::sleep_for( std::chrono::seconds{3} );

    _looping.store( false );

    for (auto& t : _threads) {
        t.join();
    }
    _threads.clear();

    for (; not _queue.empty();)
    {
        process_tasks(0);
    }
}


// Awaiter implementation for single dependency.
template <typename T>
struct TaskAwaiter
{
    Task<T>     dep;

    bool  await_ready () const  { return false; }
    T     await_resume ()       { return dep.get_result(); }
        
    template <typename P>
    bool  await_suspend (std::coroutine_handle<P> curCoro)
    {
        static_assert( std::is_base_of_v< AsyncTask, P >);

        if ( dep.is_complete() )
            return false;  // resume

        curCoro.promise().add_dependency( dep.to_promise() );
        return true;  // suspend
    }
};

template <typename T>
[[nodiscard]] auto  operator co_await (Task<T> dep)
{
    return TaskAwaiter{ dep };
}


// Awaiter implementation for multiple dependencies.
template <typename ...Types>
struct TaskAwaiter2
{
    std::tuple<Task<Types>...>     deps;

    bool  await_ready () const  { return false; }

    std::tuple<Types...>  await_resume ()
    {
        return std::apply(  [] (auto&& ...args) {
                                return std::tuple<Types...>{ args.get_result() ... };
                            },
                            deps );
    }

    template <typename ...Bools>
    [[nodiscard]] static bool  all (bool arg0, Bools... args)
    {
        if constexpr( sizeof...(Bools) )
            return arg0 and all( args... );
        else
            return arg0;
    }
        
    template <typename P>
    bool  await_suspend (std::coroutine_handle<P> curCoro)
    {
        static_assert( std::is_base_of_v< AsyncTask, P >);

        if ( std::apply( [] (auto&& ...args) { return all( args.is_complete() ... ); }, deps ))
            return false;  // resume

        std::apply( [p = &curCoro.promise()] (auto&& ...args) {
                        p->add_dependencies( args.to_promise() ... );
                    },
                    deps );

        return true;  // suspend
    }
};

template <typename ...Types>
[[nodiscard]] auto  operator co_await (std::tuple<Task<Types>...> deps)
{
    return TaskAwaiter2{ deps };
}


struct AsyncTask_Name
{
    struct Awaiter
    {
        AsyncTask*  task = nullptr;

        bool         await_ready () const  { return false; }
        std::string  await_resume ()       { return task->name(); }
        
        template <typename P>
        bool  await_suspend (std::coroutine_handle<P> curCoro)
        {
            static_assert( std::is_base_of_v< AsyncTask, P >);

            task = &curCoro.promise();
            return false;  // always resume
        }
    };

    AsyncTask_Name () {}

    [[nodiscard]] auto  operator co_await ()
    {
        return Awaiter{};
    }
};


[[nodiscard]] inline size_t  hash16 (size_t h)
{
    return (h & 0xFFFF) ^ ((h >> 16) & 0xFFFF) ^ ((h >> 32) & 0xFFFF) ^ ((h >> 48) & 0xFFFF);
}


inline std::string  AsyncTask::name () const
{
    std::stringstream str;
    str << std::hex << hash16(size_t(this) >> 3);
    return str.str();
}


[[nodiscard]] inline size_t  TID ()
{
    size_t  h = std::hash< std::thread::id >{}( std::this_thread::get_id() );
    return hash16( h );
}

[[nodiscard]] inline std::string  TIDs ()
{
    std::stringstream	str;
    str << "[" << std::hex << TID() << "] ";
    return str.str();
}



#ifdef _MSC_VER
#   include <Windows.h>

    // write to VS console.
    // it has much less overhead than 'cout' and allow to log multithreading code without additional syncs.
#   define LOG( _msg_ ) \
        {::OutputDebugStringA( (std::string{__FILE__} + "(" + std::to_string(__LINE__) + "): " + TIDs() + _msg_ + "\n").c_str() );}

#   define ASSERT( _expr_ )     \
        if ( not (_expr_) ) {   \
            LOG( # _expr_ );    \
            assert( _expr_ );   \
        }

#else

#   define LOG( _msg_ )         {}
#   define ASSERT( _expr_ )     {assert( _expr_ );}

#endif
