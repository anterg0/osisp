#include <windows.h>
#include <iostream>
#include <thread>
#include <vector>
#include <cstdlib>
#include <ctime>

const int NUM_PHILOSOPHERS = 5;
const int ITERATIONS = 10;
HANDLE forks[NUM_PHILOSOPHERS];

struct PhilosopherStats {
    int successfulEats = 0;
    int failedEats = 0;
};

PhilosopherStats stats[NUM_PHILOSOPHERS];

bool try_eat(int left, int right) {
    DWORD waitResultLeft = WaitForSingleObject(forks[left], 500);
    if (waitResultLeft != WAIT_OBJECT_0) return false;

    DWORD waitResultRight = WaitForSingleObject(forks[right], 500);
    if (waitResultRight != WAIT_OBJECT_0) {
        ReleaseMutex(forks[left]);
        return false;
    }

    return true;
}

void philosopher(int id) {
    int left = id;
    int right = (id + 1) % NUM_PHILOSOPHERS;

    if (left > right) std::swap(left, right);

    for (int i = 0; i < ITERATIONS; ++i) {
        std::cout << "Philosopher " << id << " is thinking.\n";
        Sleep(rand() % 2000 + 1);

        if (try_eat(left, right)) {
            std::cout << "Philosopher " << id << " is eating.\n";
            stats[id].successfulEats++;
            Sleep(rand() % 3000 + 1);

            ReleaseMutex(forks[left]);
            ReleaseMutex(forks[right]);
        }
        else {
            std::cout << "Philosopher " << id << " failed to eat.\n";
            stats[id].failedEats++;
        }
    }
}

int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        forks[i] = CreateMutex(nullptr, FALSE, nullptr);
        if (!forks[i]) {
            std::cerr << "Failed to create mutex for fork " << i << ".\n";
            return 1;
        }
    }

    std::vector<std::thread> philosophers;
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        philosophers.emplace_back(philosopher, i);
    }

    for (auto& t : philosophers) {
        t.join();
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        CloseHandle(forks[i]);
    }

    std::cout << "\nSimulation Results:\n";
    for (int i = 0; i < NUM_PHILOSOPHERS; ++i) {
        std::cout << "Philosopher " << i << ": \n";
        std::cout << "  Successful eats: " << stats[i].successfulEats << "\n";
        std::cout << "  Failed attempts: " << stats[i].failedEats << "\n";
    }

    return 0;
}
