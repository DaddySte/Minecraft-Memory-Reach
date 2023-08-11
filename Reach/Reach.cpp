#include "Reach.h"

//Set to Multibyte
HANDLE checkMinecraftHandle() {
    HANDLE pHandle = NULL;

    while (!pHandle) {

        if (HWND hwnd = FindWindow("LWJGL", NULL)) {

            DWORD pid;
            GetWindowThreadProcessId(hwnd, &pid);
            pHandle = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return pHandle;
}

int CustomRandomNumber(int min, int max) {

    std::random_device random;
    std::uniform_int_distribution<int> dist(min, max);

    return dist(random);
}

void DoubleScanner(double target_value, std::vector<uint64_t>& memory_addresses) {
    MEMORY_BASIC_INFORMATION memInfo = {};
    SYSTEM_INFO sysInfo = {};
    GetSystemInfo(&sysInfo);

    LPVOID currentAddress = sysInfo.lpMinimumApplicationAddress;
    LPVOID maxAddress = sysInfo.lpMaximumApplicationAddress;

    while (currentAddress < maxAddress) {

        if (VirtualQueryEx(MINECRAFT_HANDLE, currentAddress, &memInfo, sizeof(memInfo))) {
            if ((memInfo.State == MEM_COMMIT) && ((memInfo.Protect & PAGE_GUARD) == 0) && ((memInfo.Protect & PAGE_EXECUTE) ||
                (memInfo.Protect & PAGE_EXECUTE_READ) || (memInfo.Protect & PAGE_EXECUTE_READWRITE) ||
                (memInfo.Protect & PAGE_EXECUTE_WRITECOPY))) {
                SIZE_T bytesRead;
                std::vector<double> buffer(memInfo.RegionSize / sizeof(double));

                if (ReadProcessMemory(MINECRAFT_HANDLE, currentAddress, buffer.data(), memInfo.RegionSize, &bytesRead)) {
                    uint64_t addressOffset = 0;
                    for (size_t i = 0; i < bytesRead / sizeof(double); i++) {
                        if (buffer[i] == target_value) {
                            LPBYTE address = (LPBYTE)currentAddress + addressOffset;
                            if (std::find(memory_addresses.begin(), memory_addresses.end(), (uint64_t)address) == memory_addresses.end()) {
                                memory_addresses.push_back((uint64_t)address);
                            }
                        }
                        addressOffset += sizeof(double);
                    }
                }
            }
            currentAddress = (LPBYTE)memInfo.BaseAddress + memInfo.RegionSize;
        }
        else {
            currentAddress = (LPBYTE)currentAddress + sysInfo.dwPageSize;
        }
    }
}

void RewriteFloatAddress(float new_value, std::vector<uint64_t> memory_addresses) {
    for (int i = 0; i < memory_addresses.size(); i++) {
        WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)memory_addresses[i], &new_value, sizeof(new_value), NULL);
    }
}

void RewriteDoubleAddress(double new_value, std::vector<uint64_t> memory_addresses) {
    for (int i = 0; i < memory_addresses.size(); i++) {
        WriteProcessMemory(MINECRAFT_HANDLE, (LPVOID)memory_addresses[i], &new_value, sizeof(new_value), NULL);
    }
}

struct {
    double block_reach_value = 4.5;
    float reach_value = 3.f;
    double previous_block_reach_value = 4.5;
    float previous_reach_value = 3.f;
    std::vector<uint64_t> memory_default_addresses;
    std::vector<uint64_t> memory_reach_double_addresses;
    std::vector<uint64_t> memory_reach_float_addresses;
}memoryReach;

void ReachAddressHelper(std::vector<uint64_t>& base_address) {
    for (int i = 0; i < base_address.size(); i++) {
        float buffer = 0;
        for (int j = 0; j < 200; j += 4) {
            ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)(base_address[i] + j), &buffer, sizeof(buffer), NULL);
            if (buffer == memoryReach.reach_value) {
                memoryReach.memory_reach_double_addresses.push_back(base_address[i]);
                memoryReach.memory_reach_float_addresses.push_back(base_address[i] + j);
                break;
            }
        }
    }
}

void ReachScanner() {
    while (true) {
        //Find addresses
        DoubleScanner(memoryReach.block_reach_value, memoryReach.memory_default_addresses);

        //Search for reach float
        ReachAddressHelper(memoryReach.memory_default_addresses);
        memoryReach.memory_default_addresses.clear();

        //Delete repetitions
        std::sort(memoryReach.memory_reach_double_addresses.begin(), memoryReach.memory_reach_double_addresses.end());
        std::sort(memoryReach.memory_reach_float_addresses.begin(), memoryReach.memory_reach_float_addresses.end());
        memoryReach.memory_reach_double_addresses.erase(std::unique(memoryReach.memory_reach_double_addresses.begin(), memoryReach.memory_reach_double_addresses.end()), memoryReach.memory_reach_double_addresses.end());
        memoryReach.memory_reach_float_addresses.erase(std::unique(memoryReach.memory_reach_float_addresses.begin(), memoryReach.memory_reach_float_addresses.end()), memoryReach.memory_reach_float_addresses.end());

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

void ReachSafetyCheck() {
    for (int i = memoryReach.memory_reach_double_addresses.size() - 1; i >= 0; i--) {
        double double_buffer_reader = 0;
        ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)memoryReach.memory_reach_double_addresses[i], &double_buffer_reader, sizeof(double_buffer_reader), NULL);
        if (double_buffer_reader != memoryReach.block_reach_value && double_buffer_reader != memoryReach.previous_block_reach_value) {
            memoryReach.memory_reach_double_addresses.erase(memoryReach.memory_reach_double_addresses.begin() + i);
        }
    }

    for (int i = memoryReach.memory_reach_float_addresses.size() - 1; i >= 0; i--) {
        float float_buffer_reader = 0;
        ReadProcessMemory(MINECRAFT_HANDLE, (LPCVOID)memoryReach.memory_reach_float_addresses[i], &float_buffer_reader, sizeof(float_buffer_reader), NULL);
        if (float_buffer_reader != memoryReach.reach_value && float_buffer_reader != memoryReach.previous_reach_value) {
            memoryReach.memory_reach_float_addresses.erase(memoryReach.memory_reach_float_addresses.begin() + i);
        }
    }
}

void ChangeReach(int min_reach, int max_reach, int chance, bool while_sprinting) { //Int are used to be compatible with trackbars
    ReachSafetyCheck();

    int reach_buffer = CustomRandomNumber(min_reach, max_reach);
    double double_buffer = memoryReach.block_reach_value;
    float float_buffer = memoryReach.reach_value;

    if (chance > CustomRandomNumber(0, 99)) {
        if (while_sprinting && (GetAsyncKeyState('W') & 0x8000) || !while_sprinting) {
            double_buffer = (reach_buffer / 100.f) + 1.5;
            float_buffer = reach_buffer / 100.f;
        }
        else {
            double_buffer = memoryReach.block_reach_value;
            float_buffer = memoryReach.reach_value;
        }
    }

    RewriteDoubleAddress(double_buffer, memoryReach.memory_reach_double_addresses);
    RewriteFloatAddress(float_buffer, memoryReach.memory_reach_float_addresses);

    memoryReach.previous_block_reach_value = double_buffer;
    memoryReach.previous_reach_value = float_buffer;

    std::this_thread::sleep_for(std::chrono::milliseconds(MINECRAFT_TICK));
}

/* How to use
* Run Reach scanner in a different thread. Jvm keeps changing addresses so the loop must keep running as long as the program is opened (Infinite loop)
* reachAddressHelper checks for the single correct address. Reach address can be found by scanning for a 4.5 double (block placement distance) and scan the next
* 50 float addresses for 3.0 float (reach distance).
* The program saves in memory_reach_double_addresses the addresses of all 4.5 double values and deletes duplicates.
* ReachAddressHelper scans the next 50 addresses for the 3.0 float and if found it saves 4.5 address in memory_reach_double_addresses and 3.0 address in memory_reach_float_addresses
* The function loops and pauses for 2 seconds to avoid high cpu usage
* To modify reach use changeReach functions. reachSafetyCheck checks that the previous values are correct to avoid game crashes
*/