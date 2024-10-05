#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <conio.h>   // For _kbhit() and _getch() in Windows
#include <windows.h>
#include <vector>
#include <atomic>
#include <mutex>

class MarqueeConsole {
public:
    MarqueeConsole(const std::string& text, int marqueeHeight, int teleportSpeed)
        : marqueeText(text), marqueeHeight(marqueeHeight), teleportSpeed(teleportSpeed),
        x(0), y(0), xDirection(1), yDirection(1), consoleWidth(0) {
        getConsoleWidth(consoleWidth);
    }

    // Function to update the marquee text
    void updateMarquee(std::atomic<bool>& stopFlag, std::mutex& inputMutex, std::vector<std::string>& commandList, std::string& userInput) {
        while (!stopFlag.load()) {
            clear();  // Clear the console first before updating the marquee
            displayHeader();  // Display the header
            drawMarquee();  // Draw the new marquee text 
            displayUserInput(inputMutex, commandList, userInput);  // Display user input and command history
            updatePositions();  // Update positions for the next frame
            std::this_thread::sleep_for(std::chrono::milliseconds(teleportSpeed));  // Control speed
        }
    }

private:
    std::string marqueeText;
    int marqueeHeight;
    int teleportSpeed;
    int x, y;
    int xDirection, yDirection;
    int consoleWidth;  // Store the console width once

    void clear() {
        system("cls");
    }

    void getConsoleWidth(int& width) {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
            width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        }
    }

    void setCursor(int x = 0, int y = 0) {
        HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
        COORD coordinates;
        coordinates.X = x;
        coordinates.Y = y;
        SetConsoleCursorPosition(handle, coordinates);
    }

    void displayHeader() {
        std::cout << "*******************************\n";
        std::cout << "* Displaying a marquee console! *\n";
        std::cout << "*******************************\n";
    }

    void drawMarquee() {
        for (int i = 0; i < marqueeHeight; ++i) {
            if (i == y) {
                std::cout << std::string(x, ' ') << marqueeText;
            }
            std::cout << std::endl;
        }
    }

    void updatePositions() {
        if (x + marqueeText.length() >= consoleWidth) {
            xDirection = -1;  // Move left
        }
        if (x <= 0) {
            xDirection = 1;  // Move right
        }

        x += xDirection;
        y += yDirection;

        if (y >= marqueeHeight - 1 || y <= 0) {
            yDirection *= -1;  // Reverse vertical direction
        }
    }

    void displayUserInput(std::mutex& inputMutex, std::vector<std::string>& commandList, std::string& userInput) {
        std::lock_guard<std::mutex> lock(inputMutex);
        setCursor(0, 24);  // Position for displaying command history
        for (const std::string& command : commandList) {
            std::cout << "\nCommand processed in MARQUEE CONSOLE: " << command;
        }
        setCursor(0, 23);  // Position for the input prompt
        std::cout << "\nInput Command for MARQUEE_CONSOLE: " << userInput;
    }
};


class InputHandler {
public:
    InputHandler(std::atomic<bool>& stopFlag) : stopFlag(stopFlag) {}

    // Function to handle user input
    void processInput(std::mutex& inputMutex, std::vector<std::string>& commandList, std::string& userInput) {
        while (!stopFlag.load()) {
            if (_kbhit()) {
                char ch = _getch();
                std::lock_guard<std::mutex> lock(inputMutex);  // Protect user input
                if (ch == '\r') {  // If Enter key is pressed, process the input
                    if (userInput == "exit") {
                        stopFlag.store(true);  // Stop both threads if "exit" is typed
                    }
                    else if (userInput == "clear-history") {  // Detect clear-history command
                        commandList.clear();  // Clear the history
                        userInput.clear();  // Clear input after processing
                    }
                    else {
                        commandList.push_back(userInput);  // Add command to the history
                        if (commandList.size() > 5) {  // Limit the history to the last 5 commands
                            commandList.erase(commandList.begin());  // Remove the oldest command
                        }
                        userInput.clear();  // Clear input after processing
                    }
                }
                else if (ch == '\b') {  // Handle backspace
                    if (!userInput.empty()) {
                        userInput.pop_back();
                    }
                }
                else {
                    userInput += ch;  // Add character to user input
                }
            }
        }
    }

private:
    std::atomic<bool>& stopFlag;  // Reference to the stop flag
};


int main() {
    std::string message = "Hello World in Marquee!";
    int marqueeHeight = 20;  // Fixed height for vertical movement of marquee
    int teleportSpeed = 50;  // Speed in milliseconds

    std::atomic<bool> stopFlag(false);  // Flag to stop both threads
    std::mutex inputMutex;  // Mutex to protect input handling
    std::vector<std::string> commandList;  // Stores the history of commands
    std::string userInput;  // User input buffer

    MarqueeConsole marqueeConsole(message, marqueeHeight, teleportSpeed);
    InputHandler inputHandler(stopFlag);

    // Create two threads: one for updating the marquee and one for processing input
    std::thread marqueeThread(&MarqueeConsole::updateMarquee, &marqueeConsole, std::ref(stopFlag), std::ref(inputMutex), std::ref(commandList), std::ref(userInput));
    std::thread inputThread(&InputHandler::processInput, &inputHandler, std::ref(inputMutex), std::ref(commandList), std::ref(userInput));

    // Wait for both threads to finish
    marqueeThread.join();
    inputThread.join();

    std::cout << "Marquee stopped. Exiting the program.\n";
    return 0;
}
