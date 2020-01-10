#pragma once

#include <assert.h>

#include <functional>

#include <algorithm>

#include <memory>

#include <thread>
#include <atomic>
#include <mutex>

#include <vector>
#include <string>

/*
    Purpose:
        Provide a method of performing various different tasks and functions in a multi-threaded environment.
 */
namespace Async_Tasks
{
    #pragma region Property Objects
    /*
        Purpose:
            Abstract the property objects from the main focus of the Async_Tasks namespace.
     */
    namespace Properties
    {
        /*
            Purpose:
                Provide a reference to a value that cannot be modified, while allowing regular variable use.

            Requires:
                Ensure the property object is destroyed before or at the same time as the value this property encapsulates.
                Property type must implement comparative operations (== and !=).
         */
        template <class T>
        class ReadOnlyProperty
        {
            // Maintain a constant reference to the value
            const T& m_Value;

            public:
                // Prevent default construction
                ReadOnlyProperty<T>() = delete;

                // Prevent copy construction
                ReadOnlyProperty<T>(const ReadOnlyProperty<T>&) = delete;

                // Allow for setup with a T& value
                inline ReadOnlyProperty<T>(const T& p_Value) : m_Value(p_Value) {}

                // Provide an explicit method for retrieving the value
                inline const T& value() const { return m_Value; }

                // Provide an implicit method for re-obtaining the value
                inline operator const T& () const { return m_Value; }

                // Create logical comparator operators
                inline bool operator==(const T& p_Val) { return m_Value == p_Val; }
                inline bool operator!=(const T& p_Val) { return m_Value != p_Val; }

                // Remove the copy operator
                ReadOnlyProperty<T>& operator=(const ReadOnlyProperty<T>&) = delete;                
        };

        /*
            Purpose:
                Provide a reference to a value that can be checked and modified until a flag is raised to prevent modification.
                Offers callback support for validating data.

            Requires:
                Ensure the property object is destroyed before or at the same time as the value this property encapsulates.
                Property type must implement comparative operations (== and !=).
         */
        template <class T>
        class ReadWriteFlaggedProperty
        {
            // Maintain a constant reference to the value
            T& m_Value;

            // Maintain a constant reference to the modification flag
            const bool& m_Flag;

            // Store an optional callback function to validate data
            const std::function<void()> m_Callback;

            public:
                // Prevent default construction
                ReadWriteFlaggedProperty<T>() = delete;

                // Prevent copy construction
                ReadWriteFlaggedProperty<T>(const ReadWriteFlaggedProperty<T>&) = delete;

                // Allow for setup with T& and bool& values
                inline ReadWriteFlaggedProperty<T>(T& p_Value, const bool& p_Flag, const std::function<void()>& pCb = nullptr) : m_Value(p_Value), m_Flag(p_Flag), m_Callback(pCb) {}

                // Provide an explicit method for retrieving the value
                inline const T& value() const { return m_Value; }

                // Allow for the property to be set 
                inline ReadWriteFlaggedProperty<T>& operator=(const T& p_Val)
                {
                    // Check if the flag has been raised
                    if (!m_Flag)
                    {
                        // Set the new value
                        m_Value = p_Val;

                        // Check if a callback has been set
                        if (m_Callback)
                        {
                            m_Callback();
                        }
                    }

                    // Return itself
                    return *this;
                }

                // Provide an implicit method for re-obtaining the value
                inline operator const T& () const { return m_Value; }

                // Create logical comparator operators
                inline bool operator==(const T& p_Val) { return m_Value == p_Val; }
                inline bool operator!=(const T& p_Val) { return m_Value != p_Val; }
        };
    }
    #pragma endregion

    #pragma region Forward Declare Task Classes
    // Forward declare the base type for tasks to inherit from
    class Async_Task_Base;

    // Forward declare the template for the different tasks that can be completed
    template <class T> class Async_Task_Job; 
    #pragma endregion

    #pragma region Type Defines
    // Define the task ID number
    typedef unsigned long long int taskID;

    // Create an alias for the different task items the user can receive
    template <class T> using Task = std::shared_ptr<Async_Task_Job<T>>;

    // Label the variety of states the task can be in
    enum class ETask_Status : char 
    {
        // An error occurred when attempting to process the task, check the tasks error
        Error = -1,

        // The task is in a stage where it is being setup for processing
        Setup,

        // The task has been added to the task manager and is waiting to be processed
        Pending,

        // The task is currently being processed
        In_Progress,

        // The task is awaiting the "update" function to call the callback
        Callback_On_Update,

        // The task is complete 
        Completed
    };

    // Highlight the numerous priorities of the task objects
    enum ETask_Priority : unsigned int 
    {
        Low_Priority = 0x00000000,
        Medium_Priority = 0x7FFFFFFF,
        High_Priority = 0xFFFFFFFF
    };
    #pragma endregion

    #pragma region Task Manager Declaration
    /*
        Purpose:
            Complete tasks in a multi-threaded environment, while providing options for order completion, priority and callbacks for the information provided.

            Manager makes use of a manager thread to maintain the different tasks with a number of worker threads that complete the tasks.
            The number of these workers are defined when initializing the singleton instance.
     */
    class Task_Manager
    {
        // Prototype the worker class as a private object
        class Worker;

        /*---------- Singleton Values ----------*/
        static Task_Manager* m_Instance;
        Task_Manager(unsigned int p_Workers);
        ~Task_Manager() = default;

        Task_Manager() = delete;
        Task_Manager(const Task_Manager&) = delete;
        Task_Manager& operator=(const Task_Manager&) = delete;

        /*---------- Variables ----------*/
        // Hold the number of workers in use as a constant
        const unsigned int m_WorkerCount;

        // Store the values dictating the worker threads sleeping behaviour
        unsigned int m_WorkerInactiveTimeout; // Milliseconds
        unsigned int m_WorkerSleepLength; // Milliseconds

        // Maintain a pointer to the workers
        Worker* m_Workers;

        // Maintain a thread for organising the different tasks
        std::thread m_OrganisationThread;

        // Flag if the Task_Manager is operating
        std::atomic_flag m_Running;

        // Create a lock to prevent thread clashes over tasks
        std::mutex m_TaskLock;

        // Track the current ID to distribute to new tasks
        taskID m_NextID;

        // Store the maximum number of tasks that can have their callbacks executed on update per call
        unsigned int m_MaxCallbacksOnUpdate;

        // Keep a vector of all the tasks to be completed
        std::vector<std::shared_ptr<Async_Task_Base>> m_UncompletedTasks;

        // Keep a vector of all the tasks to have their callback called on an update call
        std::vector<std::shared_ptr<Async_Task_Base>> m_ToCallOnUpdate;

        /*---------- Functions ----------*/
        // Organise tasks on a separate thread
        void organiseTasks();

        public:
            // Main operation functionality
            static bool create(unsigned int p_Workers = 5u);
            static void update();
            static void destroy();

            // Task options
            template <class T> static Task<T> createTask();
            template <class T> static bool addTask(Task<T>& p_Task);

            /*---------- Setters ----------*/
            static inline void setWorkerTimeout(unsigned int p_Time);
            static inline void setWorkerSleep(unsigned int p_Time);
            static inline void setMaxCallbacks(unsigned int p_Max);
    };
    #pragma endregion

    #pragma region Task Objects
    /*
        Purpose:
            An abstract base class to allow for the creation and storing of template task jobs that can be managed and executed by the Task_Manager.
            Stores a number of base values that are shared across all tasks.
     */
    class Async_Task_Base
    {
        protected:
            // Set as a friend of the Task_Manager to allow for construction and use
            friend class Task_Manager;

            // Set as a friend of the worker to allow for use
            friend class Worker;

            // Store the ID of the current task
            taskID m_ID;

            // Store the current state of the task
            ETask_Status m_Status;

            // Store the priority of the task
            ETask_Priority m_Priority;

            // Store a flag to indicate if the callback function should be called from the Task_Manager's update function call
            bool m_CallbackOnUpdate;

            // Maintain a flag to indicate if values can be edited
            bool m_LockValues;

            // String used to contain and display error messages to the user
            std::string m_ErrorMsg;

            /*---------- Functions ----------*/
            Async_Task_Base();
            virtual ~Async_Task_Base() = default;
            Async_Task_Base(const Async_Task_Base&) = delete;
            Async_Task_Base& operator=(const Async_Task_Base&) = delete;

            // Provide main functions used to complete threaded jobs
            virtual void completeProcess() = 0;
            virtual void completeCallback() = 0;
            virtual void cleanupData() = 0;

        public:
            // Expose the ID and status values to the user for reading
            Properties::ReadOnlyProperty<taskID> id;
            Properties::ReadOnlyProperty<ETask_Status> status;

            // Expose the priority value to the user
            Properties::ReadWriteFlaggedProperty<ETask_Priority> priority;

            // Display the execute callback on update flag
            Properties::ReadWriteFlaggedProperty<bool> callbackOnUpdate;

            // Display the error string to the user for reading
            Properties::ReadOnlyProperty<std::string> error;
    };

    /*
        Purpose:
            Provide the task item to allow for the completion of jobs in a multi-threaded environment with a generic return type from the process.
     */
    template <class T>
    class Async_Task_Job : public Async_Task_Base
    {
        // Set as a friend of the Task_Manager to allow for construction and use
        friend class Task_Manager;

        // Store a pointer of type T to hold the return result in
        T* m_Result;

        // Store the function calls to process
        std::function<T()> m_Process;
        std::function<void(T&)> m_Callback;

        /*---------- Functions ----------*/
        // Restrict job creation to the Task_Manager
        Async_Task_Job<T>();

        // Override the base class abstract functions
        void completeProcess() override;
        void completeCallback() override;
        void cleanupData() override;

        public:
            // Expose the destructor to allow for the shared pointers to delete used jobs
            ~Async_Task_Job<T>() override;

            // Expose the functions function calls as properties
            Properties::ReadWriteFlaggedProperty<std::function<T()>> process;
            Properties::ReadWriteFlaggedProperty<std::function<void(T&)>> callback;
    };

    #pragma region Async_Task_Job Function Defines
    /*
        Async_Task_Job<T> : Constructor - Initialize with default values.
     */
    template <class T>
    inline Async_Task_Job<T>::Async_Task_Job() :
        Async_Task_Base::Async_Task_Base(),
        m_Result(nullptr),
        process(m_Process, Async_Task_Base::m_LockValues),
        callback(m_Callback, Async_Task_Base::m_LockValues) {}
    
    /*
        Async_Task_Job<T> : completeProcess - Perform the threaded process.
     */
    template <class T>
    inline void Async_Task_Job<T>::completeProcess()
    {
        // Create a pointer to a new object of type T and call the copy constructor on the return from the process
        m_Result = new T(m_Process());
    }

    /*
        Async_Task_Job<T> : completeCallback - Perform the callback process.
     */
    template <class T>
    inline void Async_Task_Job<T>::completeCallback()
    {
        // Run the callback and pass in a reference to the previous result
        if (m_Callback)
        {
            m_Callback(*m_Result);
        }
    }

    /*
        Async_Task_Job<T> : cleanupData - Cleanup memory allocated by the task when completing the process.
     */
    template <class T>
    inline void Async_Tasks::Async_Task_Job<T>::cleanupData()
    {
        // Check if the data has been set
        if (m_Result)
        {
            delete m_Result;
        }

        // Reset the pointer
        m_Result = nullptr;
    }

    /*
        Async_Task_Job<T> : Destructor - Delete any memory used by the task.
     */
    template <class T>
    inline Async_Task_Job<T>::~Async_Task_Job()
    {
        cleanupData();
    }
    #pragma endregion

    #pragma region Void Task Specialisation
    /*
        Purpose:
            Provide the task item to allow for the completion of jobs in a multi-threaded environment with a void return type from the process.
     */
    template <>
    class Async_Task_Job<void> : public Async_Task_Base
    {
        // Set as a friend of the Task_Manager to allow for construction and use
        friend class Task_Manager;

        // Store the functions calls to process
        std::function<void()> m_Process;
        std::function<void()> m_Callback;

        /*---------- Functions ----------*/
        // Restrict job creation to the Task_Manager
        Async_Task_Job();

        // Override the base class abstract functions
        void completeProcess() override;
        void completeCallback() override;
        void cleanupData() override;

        public:
            // Expose the destructor to allow for the shared pointers to delete used jobs
            ~Async_Task_Job() override = default;

            // Expose the functions function call as properties
            Properties::ReadWriteFlaggedProperty<std::function<void()>> process;
            Properties::ReadWriteFlaggedProperty<std::function<void()>> callback;
    };

    /*
        Async_Task_Job<void> : Constructor - Initialize with default values.
     */
    inline Async_Task_Job<void>::Async_Task_Job() :
        Async_Task_Base::Async_Task_Base(),
        process(m_Process, Async_Task_Base::m_LockValues, 
        [this]()
        {
            m_Status = ETask_Status::Setup;
            m_ErrorMsg = "";
        }),
        callback(m_Callback, Async_Task_Base::m_LockValues, 
        [this]()
        {
            m_Status = ETask_Status::Setup;
            m_ErrorMsg = "";
        }) {}

    /*
        Async_Task_Job<void> : completeProcess - Perform the threaded process.
     */
    inline void Async_Task_Job<void>::completeProcess()
    {
        // Call the process
        m_Process();
    }

    /*
        Async_Task_Job<void> : completeCallback - Perform the callback process.
     */
    inline void Async_Task_Job<void>::completeCallback()
    {
        // Call and run the callback
        if (m_Callback)
        {
            m_Callback();
        }
    }

    /*
        Async_Task_Job<void> : cleanupData - Empty function as void tasks allocate no memory.
     */
    inline void Async_Task_Job<void>::cleanupData() {}
    #pragma endregion
    #pragma endregion

    #pragma region Worker Definition
    /*
        Purpose:
            Execute the tasks provided to it by the Task_Manager.
     */
    class Task_Manager::Worker
    {
        /*---------- Variables ----------*/
        // Flags if the processing thread is running 
        std::atomic_flag m_Running;

        // Thread that runs the task processes
        std::thread m_ProcessingThread;

        // Used to indicate how many seconds of inactivity must pass before the worker goes to sleep
        Properties::ReadOnlyProperty<unsigned int> m_InactiveTimeout;

        // Used to indicate how long the processing thread should sleep when inactive 
        Properties::ReadOnlyProperty<unsigned int> m_SleepLength;

        /*---------- Functions ----------*/
        // Function run on the worker thread to process tasks passed to it
        void doWork();

        public:
            /*---------- Variables ----------*/
            // Used to store an active task to complete
            std::shared_ptr<Async_Task_Base> task;

            // Used to flag when the worker has finished their task and protect modification clashes
            std::mutex taskLock;

            /*---------- Functions ----------*/
            Worker();
            ~Worker();
    };
    #pragma endregion

    #pragma region Task Manager Templated Definitions
    /*
        Task_Manager : createTask - Return a new task object with a return type of T.

        return Task<T> - Returns a Task<T> shared pointer to a new task object.
     */
    template <class T>
    inline Task<T> Task_Manager::createTask()
    {
        // Create a new task
        Task<T> newTask = Task<T>(new Async_Task_Job<T>());

        // ID stamp the new task
        newTask->m_ID = ++m_Instance->m_NextID;

        // Return the task
        return newTask;
    }

    /*
        Task_Manager : addTask - Add a new task to the Task_Manager for processing.

        Note:
            Priority of the task does not ensure execution before lower priority tasks.
            If a lower priority task was added before the higher priority task it is possible for the lower priority task to begin processing before the higher priority task is added to the Task_Manager.

        param[in/out] p_Task - A Task<T> object to be added to the list.
                               Once added to the Task_Manager the property values will be uneditable.

        return bool - Returns a flag determining if the task was added to the manager successfully.
     */
    template <class T>
    inline bool Task_Manager::addTask(Task<T>& p_Task)
    {
        // Ensure the pointer is valid
        if (!p_Task)
            return false;

        // Ensure the task is in the setup or complete state
        switch (p_Task->m_Status)
        {
            case ETask_Status::Setup:

            case ETask_Status::Completed:
                break;

            default:
                return false;
        }

        // Ensure the task has at minimum a process functions set
        if (!p_Task->m_Process)
            return false;

        // Lock down the tasks values
        p_Task->m_LockValues = true;

        // Change the state to indicate pending processing
        p_Task->m_Status = ETask_Status::Pending;

        // Lock the task list
        m_Instance->m_TaskLock.lock();

        // Add the task to the vector
        m_Instance->m_UncompletedTasks.push_back(p_Task);

        // Sort the list based on the priority of the tasks
        std::sort(m_Instance->m_UncompletedTasks.begin(),
                  m_Instance->m_UncompletedTasks.end(),
                  [&](const std::shared_ptr<Async_Task_Base>& p_First, const std::shared_ptr<Async_Task_Base>& p_Second)
        {
            return p_First->m_Priority > p_Second->m_Priority;
        });

        // Unlock the task list
        m_Instance->m_TaskLock.unlock();

        // Return success 
        return true;
    }

    /*
        Task_Manager : setWorkerTimeout - Set the time each worker waits for work before going to sleep.

        param[in] p_Time - The amount of time (in milliseconds) to wait.
     */
    inline void Task_Manager::setWorkerTimeout(unsigned int p_Time)
    {
        m_Instance->m_WorkerInactiveTimeout = p_Time;
    }

    /*
        Task_Manager : setWorkerSleep - Set the time each worker is asleep for in between checking for new tasks.

        param[in] p_Time - The amount of time (in milliseconds) to sleep.
     */
    inline void Task_Manager::setWorkerSleep(unsigned int p_Time)
    {
        m_Instance->m_WorkerSleepLength = p_Time;
    }

    /*
        Task_Manager : setMaxCallbacks - Set the maximum number of callbacks that can occur everytime Task_Manager::update is called.

        param[in] p_Max - The maximum number of callbacks that can be executed.
     */
    inline void Task_Manager::setMaxCallbacks(unsigned int p_Max)
    {
        m_Instance->m_MaxCallbacksOnUpdate = p_Max;
    }
    #pragma endregion
}

#ifdef _ASYNCHRONOUS_TASK_HANDLING_
// Define static singleton instance
Async_Tasks::Task_Manager* Async_Tasks::Task_Manager::m_Instance = nullptr;

    #pragma region Task Manager Function Definitions
    /*
        Task_Manager : Custom Constructor - Set default pre-creation singleton values.

        param[in] p_Workers - A constant value for the number of workers that will be used by the Task_Manager.
     */
    Async_Tasks::Task_Manager::Task_Manager(unsigned int p_Workers) :
        /*---------- Workers ----------*/
        m_WorkerCount(p_Workers),
        m_Workers(nullptr),
        m_WorkerInactiveTimeout(2000),
        m_WorkerSleepLength(100),

        /*---------- Tasks ----------*/
        m_MaxCallbacksOnUpdate(10),
        m_NextID(0) {}

    /*
        Task_Manager : organiseTasks - Manage the active tasks and close finished jobs.
     */
    void Async_Tasks::Task_Manager::organiseTasks()
    {
        // Loop so long as the Task_Manager is running
        while (m_Running.test_and_set())
        {
            // Lock the data
            m_TaskLock.lock();

            // Loop through all workers and check their progress 
            for (unsigned int i = 0; i < m_WorkerCount; i++)
            {
                // Lock the workers task
                if (m_Workers[i].taskLock.try_lock())
                {
                    // Check if the worker has a task assigned
                    if (m_Workers[i].task)
                    {
                        // Check if the task is finished
                        switch (m_Workers[i].task->m_Status)
                        {
                            case ETask_Status::Callback_On_Update:
                                // Add the task to the on update callback vector
                                m_ToCallOnUpdate.push_back(m_Workers[i].task);

                                // Sort the vector based on priority
                                std::sort(m_ToCallOnUpdate.begin(), m_ToCallOnUpdate.end(),
                                          [&](const std::shared_ptr<Async_Task_Base>& p_First, const std::shared_ptr<Async_Task_Base>& p_Second)
                                {
                                    return p_First->m_Priority < p_Second->m_Priority;
                                });

                            case ETask_Status::Error:

                            case ETask_Status::Completed:
                                // Clear the workers task
                                m_Workers[i].task = nullptr;
                                break;
                        }
                    }

                    // Check if there are any tasks to handout and assign to a worker who isn't busy
                    if (m_UncompletedTasks.size() && !m_Workers[i].task)
                    {
                        // Hand the worker the next task
                        m_Workers[i].task = m_UncompletedTasks[0];

                        // Clear the task from the uncompleted list
                        m_UncompletedTasks.erase(m_UncompletedTasks.begin());
                    }

                    // Unlock the data
                    m_Workers[i].taskLock.unlock();
                }
            }

            // Unlock the data
            m_TaskLock.unlock();
        }
    }

    /*
        Task_Manager : create - Initialize and setup the Task_Manager.

        param[in] p_Workers - The number of workers the Task_Manager is to create and use (Default 5).

        return bool - Returns true if the Task_Manager was created successfully.
     */
    bool Async_Tasks::Task_Manager::create(unsigned int p_Workers)
    {
        // Assert that the Task_Manager doesn't already exist
        assert(!m_Instance);

        // Assert that there is atleast one worker created
        assert(p_Workers);

        // Create the new Task_Manager
        m_Instance = new Task_Manager(p_Workers);

        // Test to ensure the instance was created
        if (!m_Instance)
        {
            printf("Unable to create the Task_Manager singleton instance.");
            return false;
        }

        // Create the workers
        m_Instance->m_Workers = new Worker[m_Instance->m_WorkerCount];

        // Test to ensure the workers were created
        if (!m_Instance->m_Workers)
        {
            printf("Unable to create the workers for the Task_Manager.");
            return false;
        }

        // Set the operating flag
        m_Instance->m_Running.test_and_set();

        // Start the organisation thread
        m_Instance->m_OrganisationThread = std::thread([&]() 
        {
            // Call the organisation thread
            m_Instance->organiseTasks();
        });

        // Return creation was completed successfully
        return true;
    }

    /*
        Task_Manager : update - Update the different tasks and complete on update callbacks.

        Requires:
            This function will call all tasks that have callbacks on the main thread.
            This function must be ran on the main thread to operate accordingly.
     */
    void Async_Tasks::Task_Manager::update()
    {
        // Lock the tasks
        m_Instance->m_TaskLock.lock();

        // Check if there are any tasks to complete
        if (m_Instance->m_ToCallOnUpdate.size())
        {
            // Loop through tasks that need executing
            for (int i = (int)m_Instance->m_ToCallOnUpdate.size() - 1, count = 0;
                     i >= 0 && (unsigned int)count < m_Instance->m_MaxCallbacksOnUpdate;
                     i--, count++)
            {
                // Get a reference to the task
                std::shared_ptr<Async_Task_Base>& task = m_Instance->m_ToCallOnUpdate[i];

                // Try to execute the task
                try
                {
                    // Run the callback process
                    task->completeCallback();

                    // Flag the task as completed
                    task->m_Status = ETask_Status::Completed;

                    // Allow editing of task values
                    task->m_LockValues = false;
                }

                // If an error occurs, store the error message inside the task
                catch (const std::exception& p_Exc)
                {
                    // Store the message
                    task->m_ErrorMsg = p_Exc.what();

                    // Flag the task with an error flag
                    task->m_Status = ETask_Status::Error;

                    // Allow editing of task values
                    task->m_LockValues = false;
                }

                catch (const std::string& p_Exc)
                {
                    // Store the message
                    task->m_ErrorMsg = p_Exc;

                    // Flag the task with an error flag
                    task->m_Status = ETask_Status::Error;

                    // Allow editing of task values
                    task->m_LockValues = false;
                }

                catch (...)
                {
                    // Store generic message
                    task->m_ErrorMsg = "An unknown error occurred while executing the task. Error thrown did not provide any information as to the cause\n";

                    // Flag the task with an error flag
                    task->m_Status = ETask_Status::Error;

                    // Allow editing of task values
                    task->m_LockValues = false;
                }

                // Clear task's allocated memory
                task->cleanupData();

                // Remove task from the list
                m_Instance->m_ToCallOnUpdate.erase(m_Instance->m_ToCallOnUpdate.begin() + i);
            }
        }

        // Unlock the tasks
        m_Instance->m_TaskLock.unlock();
    }

    /*
        Task_Manager : destroy - Close all threads and delete the Task_Manager.
     */
    void Async_Tasks::Task_Manager::destroy()
    {
        // Test if the singleton instance was created
        if (m_Instance)
        {
            // Kill the organisation thread
            m_Instance->m_Running.clear();

            // Join the organisation thread
            if (m_Instance->m_OrganisationThread.get_id() != std::thread::id())
            {
                m_Instance->m_OrganisationThread.join();
            }

            // Delete the workers
            if (m_Instance->m_Workers)
            {
                delete[] m_Instance->m_Workers;
            }

            // Delete the singleton instance
            delete m_Instance;
            m_Instance = nullptr;
        }
    }
    #pragma endregion

    #pragma region Task Objects Function Definition
    /*
        Async_Task_Base : Constructor - Initialize the Task_Base values.
     */
    Async_Tasks::Async_Task_Base::Async_Task_Base() :
        m_ID(0),
        m_Status(Async_Tasks::ETask_Status::Setup),
        m_Priority(Async_Tasks::Low_Priority),
        m_CallbackOnUpdate(false),
        m_LockValues(false),
        id(m_ID),
        status(m_Status),
        priority(m_Priority, m_LockValues, 
        [this]()
        {
            m_Status = ETask_Status::Setup;
            m_ErrorMsg = "";
        }),
        callbackOnUpdate(m_CallbackOnUpdate, m_LockValues,
        [this]()
        {
            m_Status = ETask_Status::Setup;
            m_ErrorMsg = "";
        }),
        error(m_ErrorMsg) {}
    #pragma endregion

    #pragma region Worker Object Function Definitions
    /*
        Task_Manager::Worker : doWork - Complete the task objects assigned by the Task_Manager.
     */
    void Async_Tasks::Task_Manager::Worker::doWork()
    {
        // Track the period in time where the worker will sleep
        auto workerSleepPoint = std::chrono::system_clock::now() + std::chrono::milliseconds(m_InactiveTimeout);

        // Loop while the thread is running
        while (m_Running.test_and_set())
        {
            // Lock the task
            taskLock.lock();

            // Check if there is a job to do
            if (!task || (task && task->m_Status != ETask_Status::Pending))
            {
                // Get the current time
                auto currentTime = std::chrono::system_clock::now();

                // Calculate the difference between time points
                auto difference = std::chrono::duration_cast<std::chrono::milliseconds>(workerSleepPoint - currentTime);

                // Check if the worker is still awake
                if (difference.count() > 0)
                {
                    // Unlock the task
                    taskLock.unlock();

                    // Yield to other threads
                    std::this_thread::yield();
                }

                // If the worker is sleeping, sleep for the appropriate period of time
                else
                {
                    // Unlock the task
                    taskLock.unlock();

                    // Sleep the thread
                    std::this_thread::sleep_for(std::chrono::milliseconds(m_SleepLength));
                }

                // Continue executing the Loop
                continue;
            }

            // Set the new sleep time
            workerSleepPoint = std::chrono::system_clock::now() + std::chrono::milliseconds(m_InactiveTimeout);

            // Try to execute the task
            try
            {
                // Update the tasks current state
                task->m_Status = ETask_Status::In_Progress;

                // Run the process
                task->completeProcess();

                // Check if the callback doesn't need to be run on the main thread
                if (!task->m_CallbackOnUpdate)
                {
                    // Run the callback process
                    task->completeCallback();

                    // Flag the task as completed
                    task->m_Status = ETask_Status::Completed;

                    // Allow editing of task values
                    task->m_LockValues = false;

                    // Clear tasks allocated memory
                    task->cleanupData();
                }

                // Otherwise flag the task as needing to be called on the main thread
                else 
                {
                    task->m_Status = ETask_Status::Callback_On_Update;
                }
            }

            // If an error occurs, store the error message inside the task
            catch (const std::exception& p_Exc)
            {
                // Store the message
                task->m_ErrorMsg = p_Exc.what();

                // Flag the task with an error flag
                task->m_Status = ETask_Status::Error;

                // Allow editing of task values
                task->m_LockValues = false;
            }

            catch (const std::string& p_Exc)
            {
                // Store the message
                task->m_ErrorMsg = p_Exc;

                // Flag the task with an error flag
                task->m_Status = ETask_Status::Error;

                // Allow editing of task values
                task->m_LockValues = false;
            }

            catch (...)
            {
                // Store generic message
                task->m_ErrorMsg = "An unknown error occurred while executing the task. Error thrown did not provide any information as to the cause\n";

                // Flag the task with an error flag
                task->m_Status = ETask_Status::Error;

                // Allow editing of task values
                task->m_LockValues = false;
            }

            // Unlock the task
            taskLock.unlock();
        }
    }

    /*
        Task_Manager::Worker : Constructor - Initialize with default values and start the worker thread.
     */
    inline Async_Tasks::Task_Manager::Worker::Worker() : 
        m_InactiveTimeout(Async_Tasks::Task_Manager::m_Instance->m_WorkerInactiveTimeout),
        m_SleepLength(Async_Tasks::Task_Manager::m_Instance->m_WorkerSleepLength),
        task(nullptr)
    {
        m_ProcessingThread = std::thread([&]()
        {
            // Set the running flag
            m_Running.test_and_set();

            // Start the processing function
            doWork();
        }); 
    }

    /*
        Task_Manager::Worker : Destructor - Join the worker thread.
     */
    inline Async_Tasks::Task_Manager::Worker::~Worker()
    {
        // Clear the running flag
        m_Running.clear();

        // Join the worker thread
        if (this->m_ProcessingThread.get_id() != std::thread::id())
        {
            m_ProcessingThread.join();
        }
    }
    #pragma endregion
#endif