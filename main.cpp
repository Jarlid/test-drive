#include <iostream>
#include <windows.h>
#include <random>
#include <chrono>

using namespace std;

int main() {
    const unsigned int REPEATS = 1'000, FILE_SECTORS = 1'000'000;
    DWORD SECTOR_SIZE, trash;

    char path[256];
    GetModuleFileName(nullptr, path, sizeof(path));
    auto disk = strtok(path, "\\");
    char file_name[] = "test.txt";

    GetDiskFreeSpace(
            disk,
            &trash,
            &SECTOR_SIZE,
            &trash,
            &trash
            );

    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(0,255);

    char* data = (char*) malloc(FILE_SECTORS * SECTOR_SIZE);
    for (int i = 0; i < FILE_SECTORS * SECTOR_SIZE; ++i)
        data[i] = (char) dist(gen);

    vector<long long> write_delays;
    vector<long long> read_delays;

    cout << "Setup complete!" << endl << endl;

    for (int i = 0; i < REPEATS; ++i) {
        auto file = CreateFile(
                file_name,
                GENERIC_WRITE,
                0,
                nullptr,
                CREATE_ALWAYS,
                FILE_FLAG_NO_BUFFERING,
                nullptr
        );

        auto begin = chrono::steady_clock::now();

        WriteFile(
                file,
                data,
                FILE_SECTORS * SECTOR_SIZE,
                &trash,
                nullptr
                );

        auto end = chrono::steady_clock::now();
        write_delays.push_back(chrono::duration_cast<chrono::microseconds>(end - begin).count());

        CloseHandle(file);

        file = CreateFile(
                file_name,
                GENERIC_READ,
                0,
                nullptr,
                OPEN_ALWAYS,
                FILE_FLAG_NO_BUFFERING,
                nullptr
        );

        begin = chrono::steady_clock::now();

        ReadFile(
                file,
                data,
                FILE_SECTORS * SECTOR_SIZE,
                &trash,
                nullptr
        );

        end = chrono::steady_clock::now();
        read_delays.push_back(chrono::duration_cast<chrono::microseconds>(end - begin).count());

        CloseHandle(file);

        DeleteFile(file_name);
        if ((i + 1) * 100 / REPEATS > i * 100 / REPEATS) {
            cout << "Repeat #" << i + 1 << " out of " << REPEATS << " done!" << endl;
        }
    }

    cout << endl;

    sort(write_delays.begin(), write_delays.end());
    sort(read_delays.begin(), read_delays.end());

    long long cnt = 0, write_delay_sum = 0, read_delay_sum = 0;
    double write_speed_sum = 0, read_speed_sum = 0, size = (double) (FILE_SECTORS * SECTOR_SIZE);

    unsigned int left_border = REPEATS / 10, right_border = REPEATS - REPEATS / 10;

    for (unsigned int i = left_border; i < right_border; ++i) {
        cnt += 1;
        write_delay_sum += write_delays[i];
        write_speed_sum += size / (double) write_delays[i];
        read_delay_sum += read_delays[i];
        read_speed_sum += size / (double) read_delays[i];
    }

    long long write_delay_mid = write_delay_sum / cnt, read_delay_mid = read_delay_sum / cnt;
    long long write_delay_dev = 0, read_delay_dev = 0;
    double write_speed_mid = write_speed_sum / (double) cnt, read_speed_mid = read_speed_sum / (double) cnt;
    double write_speed_dev = 0, read_speed_dev = 0;

    for (unsigned int i = left_border; i < right_border; ++i) {
        write_delay_dev += (write_delays[i] - write_delay_mid) * (write_delays[i] - write_delay_mid);
        write_speed_dev += pow(size / (double) write_delays[i] - write_speed_mid, 2);
        read_delay_dev += (read_delays[i] - read_delay_mid) * (read_delays[i] - read_delay_mid);
        read_speed_dev +=  pow(size / (double) read_delays[i] - read_speed_mid, 2);
    }

    double MIL = 1'000'000;

    cout << "Average write delay:            " << (double) write_delay_mid / MIL << " s" << endl;
    cout << "Write delay standard deviation: " << sqrt(write_delay_dev) / (double) cnt / MIL << " s" << endl << endl;

    cout << "Average write speed:            " << (double) write_speed_mid << " MB/s" << endl;
    cout << "Write speed standard deviation: " << sqrt(write_speed_dev) / (double) cnt << " MB/s" << endl << endl;

    cout << "Average read delay:             " << (double) read_delay_mid / MIL << " s" << endl;
    cout << "Read delay standard deviation:  " << sqrt(read_delay_dev) / (double) cnt / MIL << " s" << endl << endl;

    cout << "Average read speed:             " << (double) read_speed_mid << " MB/s" << endl;
    cout << "Read speed standard deviation:  " << sqrt(read_speed_dev) / (double) cnt << " MB/s" << endl;

    return 0;
}
