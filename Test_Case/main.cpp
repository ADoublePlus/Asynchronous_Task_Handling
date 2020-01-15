#include "Async_Tasks.h"

#include "Basic_Input.h"
#include "Random.h"

/*
    statusToName - Convert an Async_Task status value into a printable string.

    param[in] p_Status - The Async_Tasks::ETask_Status to convert to a string.

    return const char* - Returns a c-string describing the status.
 */
inline const char* statusToName(const Async_Tasks::ETask_Status& p_Status)
{
    switch (p_Status)
    {
        case Async_Tasks::ETask_Status::Error:
            return "ERROR";

        case Async_Tasks::ETask_Status::Setup:
            return "SETUP";

        case Async_Tasks::ETask_Status::Pending:
            return "PENDING";

        case Async_Tasks::ETask_Status::In_Progress:
            return "IN_PROGRESS";

        case Async_Tasks::ETask_Status::Callback_On_Update:
            return "CALLBACK_ON_UPDATE";

        case Async_Tasks::ETask_Status::Completed:
            return "COMPLETE";

        default:
            return "Unknown Status";
    }
}

/*
    normalizingVectors - Normalize a large number of randomly sized vectors to test speed.
 */
void normalizingVectors()
{
    // Define basic Vec3 struct
    struct Vec3
    {
        float x, y, z;
    };

    // Store number of worker threads to create
    unsigned int threadCount;

    // Loop until valid thread 
    do 
    {
        // Clear screen
        system("CLS");

        // Display prompt to user
        getInput(threadCount, "Enter the number of worker threads to create (32 maximum): ");
    } while (threadCount > 32);

    // Add some space on screen
    printf("\n\n\n");

    // Create the Task_Manager
    if (Async_Tasks::Task_Manager::create(threadCount))
    {
        // Create basic input manager 
        Basic_Input input = Basic_Input(VK_ESCAPE, VK_SPACE);

        // Display initial instructions
        printf("Hold 'SPACE' to add a new task to normalize 3,000,000 Vector3 objects (Multiple Task Test)\n\n");

        // Loop until input breaks the Loop
        while (!input.keyPressed(VK_ESCAPE))
        {
            // Update input
            input.update();

            // Update Task_Manager
            Async_Tasks::Task_Manager::update();

            // Output a single '.'
            printf(".");

            // Determine if space was pressed 
            if (input.keyDown(VK_SPACE))
            {
                // Create a new task
                Async_Tasks::Task<std::pair<unsigned int, float>> newTask = Async_Tasks::Task_Manager::createTask<std::pair<unsigned int, float>>();

                // Force callback to print on main
                newTask->callbackOnUpdate = true;

                // Set functions
                newTask->process = [&]() -> std::pair<unsigned int, float>
                {
                    // Define the number of vectors to create
                    const unsigned int ARRAY_SIZE = 3000000;

                    // Create an array of 1 million vectors
                    Vec3* vectors = new Vec3[ARRAY_SIZE];

                    // Store the number of vectors that were successfully normalized
                    unsigned int totalNormal = 0;
                    float averageNonNormal = 0.f;

                    // Allocate the vectors random values
                    for (unsigned int i = 0; i < ARRAY_SIZE; i++)
                    {
                        // Assign random axis values
                        vectors[i].x = randomRange(-500.f, 500.f);
                        vectors[i].y = randomRange(-500.f, 500.f);
                        vectors[i].z = randomRange(-500.f, 500.f);

                        // Acquire the magnitude of the vector 
                        float mag = sqrtf(SQU(vectors[i].x) + SQU(vectors[i].y) + SQU(vectors[i].z));

                        // Confirm the magnitude is valid
                        if (mag)
                        {
                            // Divide along the axis
                            vectors[i].x /= mag;
                            vectors[i].y /= mag;
                            vectors[i].z /= mag;
                        }

                        // Check if magnitude has been normalized
                        mag = sqrtf(SQU(vectors[i].x) + SQU(vectors[i].y) + SQU(vectors[i].z));

                        if (mag == 0.f || mag == 1.f)
                        {
                            totalNormal++;
                        }
                        else 
                        {
                            averageNonNormal += mag;
                        }
                    }

                    // Delete vector objects
                    delete[] vectors;

                    // Return final count 
                    return std::pair<unsigned int, float>(totalNormal, averageNonNormal /= (float)(ARRAY_SIZE - totalNormal));
                };

                newTask->callback = [&](std::pair<unsigned int, float>& p_Val)
                {
                    printf("\Normalized %u of 3,000,000 Vector3 objects. The average non-normalized magnitude was %f\n", p_Val.first, p_Val.second);
                };

                // Add task to Task_Manager
                if (Async_Tasks::Task_Manager::addTask(newTask))
                {
                    printf("\n\nAdded new task to the manager. Processing...\n");
                }
                else 
                {
                    printf("\n\nFailed to add the new task to the manager.\n");
                }
            }

            // Slow main down to viewable pace
            Sleep(100);
        }
    }

    // Display error message
    else 
    {
        printf("Failed to create the Asynchronous Task Manager\n");
    }

    // Destroy Task_Manager
    Async_Tasks::Task_Manager::destroy();
}

/*
    reusableTask - Test reusing task objects for similar tasks repeatedly.
 */
void reusableTask()
{
    // Clear screen
    system("CLS");

    // Create Task_Manager
    if (Async_Tasks::Task_Manager::create(1))
    {
        // Create basic input manager
        Basic_Input input = Basic_Input(VK_ESCAPE, VK_SPACE);

        // Create the base job
        Async_Tasks::Task<unsigned int> stringTask = Async_Tasks::Task_Manager::createTask<unsigned int>();

        // Set callback to occur on main
        stringTask->callbackOnUpdate = true;

        // Set the process function 
        stringTask->process = [&]() -> unsigned int
        {
            // Create a random character string
            unsigned int sum = 0;

            // Loop through and add the totals from 0 to UINT_MAX
            for (unsigned int i = 0; i < UINT_MAX; i++)
            {
                sum++;
            }

            // Return the sum
            return sum;
        };

        // Attach the callback function
        stringTask->callback = [&](unsigned int& p_Num)
        {
            printf("\n\nCounted to: %u\n\n", p_Num);
        };

        // Display initial instructions
        printf("Press 'SPACE' to count to UINT_MAX when task object is available (Reusable Task Test)\n\n");

        // Loop till escape is pressed
        while (!input.keyPressed(VK_ESCAPE))
        {
            // Update input
            input.update();

            // Update Task_Manager
            Async_Tasks::Task_Manager::update();

            // Output a single '.'
            if (stringTask->status != Async_Tasks::ETask_Status::Completed && stringTask->status != Async_Tasks::ETask_Status::Setup)
            {
                printf(".");
            }

            // Determine if the user has pressed space
            if (input.keyPressed(VK_SPACE))
            {
                // Add task to the Task_Manager
                if (Async_Tasks::Task_Manager::addTask(stringTask))
                {
                    printf("Starting to count now:\n");
                }
            }

            // Slow main down to viewable pace
            Sleep(100);
        }
    }

    // Display error message
    else 
    {
        printf("Failed to create the Asynchronous Task Manager\n");
    }

    // Destroy Task_Manager
    Async_Tasks::Task_Manager::destroy();
}

/*
    errorReporting - Test task error reporting to the user upon exception thrown.
 */
void errorReporting()
{
    // Create Task_Manager
    if (Async_Tasks::Task_Manager::create(1))
    {
        // Store a user string
        char usrError[256];

        // Get a custom error message from the user
        do 
        {
            // Clear screen
            system("CLS");

            // Get the message from the user 
            getInput(usrError, "Enter an error message to be reported on error thrown (Max 256 characters): ");
        } while (!strlen(usrError));

        // Add screen space
        printf("\n\n\n");

        // Create basic input manager
        Basic_Input input = Basic_Input(VK_ESCAPE, VK_SPACE);

        // Store a flag for describing the position the error should be called
        bool errorFlag = false;

        // Setup the basic task to throw the error after a period of time
        Async_Tasks::Task<void> errorTask = Async_Tasks::Task_Manager::createTask<void>();

        // Setup the error task with its values
        errorTask->callbackOnUpdate = false;
        errorTask->process = [&]() 
        {
            // Sleep the thread for a period of time
            std::this_thread::sleep_for(std::chrono::milliseconds(randomRange(2000U, 5000U)));

            // Test to see if the error should be thrown at this particular instance
            if (!errorFlag)
            {
                // Toggle error flag
                errorFlag = true;

                // Throw the users error
                throw std::runtime_error(std::string("Task process function threw the error: ") + usrError);
            }
        };
        errorTask->callback = [&]()
        {
            // Toggle error flag
            errorFlag = false;

            // Throw the users error
            throw std::runtime_error(std::string("The task callback function threw the error: ") + usrError);
        };

        // Store the previous status of the task
        Async_Tasks::ETask_Status prevStatus = Async_Tasks::ETask_Status::Error;

        // Ouput starting message
        printf("Press 'SPACE' to start the task. Task progress and error output will be displayed.\n\n");

        // Loop till escape is pressed
        while (!input.keyPressed(VK_ESCAPE))
        {
            // Update input
            input.update();

            // Update Task_Manager
            Async_Tasks::Task_Manager::update();

            // Check task status
            if (errorTask->status != prevStatus)
            {
                // Set the new status value
                prevStatus = errorTask->status;

                // Output the new status of the task
                printf("Task Status: %s (%i)\n", statusToName(prevStatus), prevStatus);

                // If the task has failed
                if (prevStatus == Async_Tasks::ETask_Status::Error)
                {
                    // Output error message
                    printf("%s\n\n\n", errorTask->error.value().c_str());

                    // Force task to reset itself
                    errorTask->callbackOnUpdate = (bool)errorTask->callbackOnUpdate;
                }
            }

            // Check to see if the user wants to add the task to the manager
            if (input.keyPressed(VK_SPACE) && (errorTask->status == Async_Tasks::ETask_Status::Setup || errorTask->status == Async_Tasks::ETask_Status::Error))
            {
                Async_Tasks::Task_Manager::addTask(errorTask);
            }
        }
    }

    // Display error message
    else
    {
        printf("Failed to create the Asynchronous Task Manager\n");
    }

    // Destroy Task_Manager
    Async_Tasks::Task_Manager::destroy();
}

/*
    main - Test the current implementation of the Async_Tasks namespace.
 */
int main()
{
    // Create a simple struct to describe possible tests
    struct Executable_Test 
    {
        const char* label;
        void(*const functionPtr)();
    };

    // Create an array of the possible Executable_Tests
    const Executable_Test POSSIBLE_TESTS[] =
    {
        { "Normalizing Vectors", normalizingVectors },
        { "Reusable Task", reusableTask },
        { "Error Reporting", errorReporting }
    };

    // Store number of possible tests to select from
    const unsigned int TEST_COUNT = sizeof(POSSIBLE_TESTS) / sizeof(Executable_Test);

    // Store user input
    char usrChoice;

    // Loop so user can choose between the different tests
    do
    {
        // Clear screen
        system("CLS");

        // Display options
        printf("Implemented Tasks (%u):\n", TEST_COUNT);

        for (unsigned int i = 0; i < TEST_COUNT; i++)
        {
            printf("%i. %s\n", i + 1, POSSIBLE_TESTS[i].label);
        }

        // Receive input selection from the user
        getInput(usrChoice, "\nEnter the desired test (Invalid character to quit): ");

        // Adjust for character/digit offset
        usrChoice -= '1';

        // Check the selection is within range
        if (usrChoice >= 0 && usrChoice < TEST_COUNT)
        {
            // Call the function
            POSSIBLE_TESTS[usrChoice].functionPtr();

            // Allow the user to see the final output
            printf("\n\n\n\n\n");

            system("PAUSE");
        }

        // Otherwise, exit program
        else 
            break;
    } while (true);

    return EXIT_SUCCESS;
}