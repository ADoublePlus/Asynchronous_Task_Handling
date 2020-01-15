#pragma once

#include <Windows.h>
#include <unordered_map>
#include <utility>
#include <iostream>

#pragma region Key Checking
/*
    Purpose:
        Provide a simple way of testing various specific virtual keys which are defined at creation of the input object.
 */
class Basic_Input
{
    /*---------- Variables ----------*/
    // Maintain a map of the different keys to check 
    std::unordered_map<int, std::pair<bool, bool>> m_KeyStates;

     /*---------- Functions ----------*/
     // Key extraction functions 
     template <typename ... TKeys>
     inline void extractKeys(int p_Key, TKeys ... p_Keys);
     inline void extractKeys(int p_Key);

     public:
        // Constructor for allowing multiple key checks
        template <typename ... TKeys>
        inline Basic_Input(TKeys ... p_Keys);

        // Used to update input values
        inline void update();

        // Input checking functions
        inline bool keyDown(int p_Key);
        inline bool keyUp(int p_Key);
        inline bool keyPressed(int p_Key);
        inline bool keyReleased(int p_Key);
};

#pragma region Function Definitions
#pragma region Private Functions
/*
    Basic_Input : extractKeys - Extract the key values from variadic parameter pack.

    param[in] p_Key - The next key extracted from the parameter pack.
    param[in] p_Keys - The parameter pack containing the remaining keys to process.
 */
template <typename ... TKeys>
inline void Basic_Input::extractKeys(int p_Key, TKeys ... p_Keys)
{
    // Add key to map if it does not already exist
    if (m_KeyStates.find(p_Key) == m_KeyStates.end())
    {
        m_KeyStates[p_Key] = std::pair<bool, bool>(false, false);
    }

    // Move through the parameter pack
    extractKeys(p_Keys ...);
}

/*
    Basic_Input : extractKeys - Extract the final key value from the parameter pack.

    param[in] p_Key - The final key to add to the monitored map.
 */
inline void Basic_Input::extractKeys(int p_Key)
{
    // Add key to map if it does not already exist
    if (m_KeyStates.find(p_Key) == m_KeyStates.end())
    {
        m_KeyStates[p_Key] = std::pair<bool, bool>(false, false);
    }
}
#pragma endregion

#pragma region Main Functions
/*
    Basic_Input : Constructor - Initialize the Basic_Input object with a set of virtual keys to monitor.

    param[in] p_Keys - A parameter pack containing the virtual keys to check.
 */
template <typename ... TKeys>
inline Basic_Input::Basic_Input(TKeys ... p_Keys)
{
    // Pass the keys to the extraction functions
    extractKeys(p_Keys ...);
}

/*
    Basic_Input : update - Update the contained virtual key states.
 */
inline void Basic_Input::update()
{
    // Loop through map elements
    for (auto& iter : m_KeyStates)
    {
        // Save previous state 
        iter.second.first = iter.second.second;

        // Get current state
        iter.second.second = (GetKeyState(iter.first) < 0);
    }
}
#pragma endregion

#pragma region Input State Functions
/*
    Basic_Input : keyDown - Checks to see if the specified key is down.

    param[in] p_Key - The virtual key to check the state of.

    return bool - Returns true if the specified virtual key is down.
 */
inline bool Basic_Input::keyDown(int p_Key) { return m_KeyStates[p_Key].second; }

/* 
    Basic_Input : keyUp - Checks to see if the specified key is up.

    param[in] p_Key - The virtual key to check the state of.

    return bool - Returns true if the specified virtual key is up.
 */
inline bool Basic_Input::keyUp(int p_Key) { return !m_KeyStates[p_Key].second; }

/*
    Basic_Input : keyPressed - Checks to see if the specified key was pressed this update cycle.

    param[in] p_Key - The virtual key to check the state of.

    return bool - Returns true if the virtual key was just pressed.
 */
inline bool Basic_Input::keyPressed(int p_Key) { return m_KeyStates[p_Key].second && !m_KeyStates[p_Key].first; }

/*
    Basic_Input : keyReleased - Checks to see if the specified key was released this update cycle.

    param[in] p_Key - The virtual key to check the state of.

    return bool - Returns true if the virtual key was just released.
 */
inline bool Basic_Input::keyReleased(int p_Key) { return !m_KeyStates[p_Key].second && m_KeyStates[p_Key].first; }
#pragma endregion
#pragma endregion
#pragma endregion

#pragma region User Input
/*
    clearInBuffer - Clear the buffer of characters passed in via std::cin.
 */
inline void clearInBuffer()
{
    // Reset stream state
    std::cin.clear();

    // Ignore till EOF
    std::cin.ignore(INT_MAX, '\n');
}

/*
    getInput - Receives input from the user of an indiscriminate type.

    param[in/out] p_In - A reference to the variable to store the users input.
    param[in] p_Message - An optional char pointer of a message to display prior to prompting the user for input (default nullptr).
 */
template <typename T>
inline void getInput(T& p_In, const char* p_Message = nullptr)
{
    // If there is an output message, print to console
    if (p_Message)
    {
        std::cout << p_Message;
    }

    // Read in user response
    std::cin >> p_In;

    // Reset stream state
    clearInBuffer();
}

/*
    getInput - Receives array input from the user into a stack allocated array.

    param[in/out] p_In - A reference to the T array to store the users input.
    param[in] p_Message - An optional char pointer of a message to display prior to prompting the user for input (default nullptr).
 */
template <typename T, size_t N>
inline void getInput(T(&p_In)[N], const char* p_Message = nullptr)
{
     // If there is an output message, print to console
    if (p_Message)
    {
        std::cout << p_Message;
    }

    // Read in user responses
    for (unsigned int i = 0; i < N; i++)
    {
        std::cin >> p_In[i];
    }

    // Reset stream state
    clearInBuffer();
}

/*
    getInput - Receives string input from the user into a stack allocated array.

    param[in/out] p_In - A reference to the char array to store the users input.
    param[in] p_Message - An optional char pointer of a message to display prior to prompting the user for input (default nullptr).
 */
template <size_t N>
inline void getInput(char(&p_In)[N], const char* p_Message = nullptr)
{
    // If there is an output message, print to console
    if (p_Message)
    {
        std::cout << p_Message;
    }

    // Read in user response
    std::cin.get(p_In, sizeof(char) * N);

    // Reset stream state
    clearInBuffer();
}

/*
    getInput - Receives array input from the user into a heap allocated array.

    param[in/out] p_In - A reference to the T array to store the users input.
    param[in] p_BufferSize - The size of the char buffer that is being filled.
    param[in] p_Message - An optional char pointer of a message to display prior to prompting the user for input (default nullptr).
 */
template <typename T>
inline void getInput(T*& p_In, const size_t p_BufferSize, const char* p_Message = nullptr)
{
    // If there is an output message, print to console
    if (p_Message)
    {
        std::cout << p_Message;
    }

    // Read in user responses
    for (unsigned int i = 0; i < p_BufferSize; i++)
    {
        std::cin >> p_In[i];
    }

    // Reset stream state
    clearInBuffer();
}

/*
    getInput - Receives string input from the user into a heap allocated array.

    param[in/out] p_In - A reference to the char pointer to store the users input.
    param[in] p_BufferSize - The size of the char buffer that is being filled.
    param[in] p_Message - An optional char pointer of a message to display prior to prompting the user for input (default nullptr).
 */
template <>
inline void getInput(char*& p_In, const size_t p_BufferSize, const char* p_Message)
{
    // If there is an output message, print to console
    if (p_Message)
    {
        std::cout << p_Message;
    }

    // Read in user response
    std::cin.get(p_In, sizeof(char) * p_BufferSize);

    // Reset stream state
    clearInBuffer();
}
#pragma endregion