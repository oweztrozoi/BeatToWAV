#include <iostream>
#include <conio.h>
#include <chrono>
#include <thread>
#include <cstdint>
#include <vector>
#include <fstream>
#include <cmath>
#include <string>

// clicks: number of clicks recorded (used to compute baseBeatDuration = time / clicks)
// time: total elapsed time (in seconds) during which clicks occurred
// baseBeats: the number of beats for the base file (e.g. 189)
// subBeatFactor: how many sub-beats per base beat (default = 1 for the base file)
void generateWAV(int clicks, double time, int baseBeats, int subBeatFactor = 1) {
    // Use red for error messages.
    if (clicks <= 0 || time <= 0 || baseBeats <= 0 || subBeatFactor <= 0) {
        std::cerr << "\033[1;31mInvalid parameters.\033[0m" << std::endl;
        return;
    }

    int numBeats = baseBeats * subBeatFactor;
    double baseBeatDuration = time / clicks;              // Duration of one base beat
    double totalDuration = baseBeatDuration * baseBeats;      // Total duration remains the same as the base file
    double effectiveBeatDuration = baseBeatDuration / subBeatFactor; // Time interval between each sub-beat

    const uint32_t sampleRate = 44100;
    const uint16_t numChannels = 1;   // Mono
    const uint16_t bitsPerSample = 16;
    uint32_t numSamples = static_cast<uint32_t>(totalDuration * sampleRate);

    // Create a PCM buffer filled with silence.
    std::vector<int16_t> audioData(numSamples, 0);

    // Define click parameters:
    // A "click" lasting for 10 samples with a fixed amplitude.
    const uint32_t clickLengthSamples = 10;
    const int16_t clickAmplitude = 30000;

    // Insert a click at each beat time.
    for (int i = 0; i < numBeats; i++) {
        // Compute the starting sample index for this sub-beat.
        uint32_t clickStartIndex = static_cast<uint32_t>(i * effectiveBeatDuration * sampleRate);
        // Ensure we don't write beyond the buffer.
        if (clickStartIndex + clickLengthSamples < numSamples) {
            for (uint32_t j = 0; j < clickLengthSamples; j++) {
                audioData[clickStartIndex + j] = clickAmplitude;
            }
        }
    }

    // Generate a unique filename.
    static int fileCount = 0;
    fileCount++;
    std::string filename = "output_" + std::to_string(fileCount) + ".wav";

    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "\033[1;31mError opening file for output.\033[0m" << std::endl;
        return;
    }

    // Write the WAV header.
    uint32_t subChunk2Size = numSamples * numChannels * bitsPerSample / 8;
    uint32_t chunkSize = 36 + subChunk2Size;

    outFile.write("RIFF", 4);
    outFile.write(reinterpret_cast<const char*>(&chunkSize), 4);
    outFile.write("WAVE", 4);

    // Write the fmt subchunk.
    outFile.write("fmt ", 4);
    uint32_t subChunk1Size = 16;  // PCM
    outFile.write(reinterpret_cast<const char*>(&subChunk1Size), 4);
    uint16_t audioFormat = 1;     // PCM format
    outFile.write(reinterpret_cast<const char*>(&audioFormat), 2);
    outFile.write(reinterpret_cast<const char*>(&numChannels), 2);
    outFile.write(reinterpret_cast<const char*>(&sampleRate), 4);
    uint32_t byteRate = sampleRate * numChannels * bitsPerSample / 8;
    outFile.write(reinterpret_cast<const char*>(&byteRate), 4);
    uint16_t blockAlign = numChannels * bitsPerSample / 8;
    outFile.write(reinterpret_cast<const char*>(&blockAlign), 2);
    outFile.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // Write the data subchunk.
    outFile.write("data", 4);
    outFile.write(reinterpret_cast<const char*>(&subChunk2Size), 4);

    // Write PCM data.
    outFile.write(reinterpret_cast<const char*>(audioData.data()), subChunk2Size);
    outFile.close();

    // Print success message in green.
    std::cout << "\033[1;32mWAV file generated successfully: " << filename << "\033[0m" << std::endl;
}


void generateWAVs(int clicks, double time) {
    // Prompt for base beats in cyan.
    std::cout << "\033[1;36m\nHow many beats do you want the WAV to include (p.e.: 4 will result in a WAV that is 4 beats long)? \033[0m" << std::endl;
    int baseBeats;
    std::cin >> baseBeats;

    // Inform base file creation in yellow.
    std::cout << "\033[1;33mCreating base .wav file...\033[0m" << std::endl;
    generateWAV(clicks, time, baseBeats, 1);  // Base file: no subdivision

    // Loop to allow multiple additional files.
    while (true) {
        std::cout << "\033[1;36mDo you want to create an additional .wav file with sub-beats? (y/n) \033[0m" << std::endl;
        char key = _getch();
        std::cout << key << std::endl;  // Echo the user's choice.
        if (key != 'y' && key != 'Y') {
            break;
        }
        std::cout << "\033[1;36mHow many sub-beats per beat do you want? \033[0m" << std::endl;
        int subBeats;
        std::cin >> subBeats;
        // For additional files, total beats = baseBeats * subBeats.
        generateWAV(clicks, time, baseBeats, subBeats);
    }
}


void useBPM() {
    system("cls");
    std::cout << "\033[1;36mPlease enter your desired BPM: \033[0m" << std::endl;
    int bpm;
    std::cin >> bpm;
    generateWAVs(bpm, 60);
}


void clickRecorder() {
    std::cout << "\033[1;33m"
        << "Press [SPACE] to start recording. Press any other key to stop recording (has to be on-beat).\nPress [m] to manually enter BPM."
        << "\033[0m" << std::endl;

    // Start click detection
    char key = _getch();
    if (key == ' ') {
        system("cls");
        std::cout << "\033[1;33mClick!\033[0m" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        system("cls");
    }
    else if (key == 'm') {
        useBPM();
        return;
    }
    else {
        system("cls");
        clickRecorder();
    }

    // Record first click time
    auto start = std::chrono::steady_clock::now();

    // Record first click time if SPACE pressed, if m pressed call useBPM() to enter BPM manually.
    int clicks;
    clicks = 0;
    while (true) {
        char key = _getch();
        if (key == ' ') {
            system("cls");
            std::cout << "\033[1;33mClick!\033[0m" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            system("cls");
            clicks++;
        }
        else {
            system("cls");
            clicks++;
            break;
        }
    }

    // Record last click time
    auto end = std::chrono::steady_clock::now();

    // Calculate the elapsed time in seconds
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "\033[1;36mRecorded \033[0m" << clicks << "\033[1;36m clicks in \033[0m" << elapsed.count() << "\033[1;36m seconds.\033[0m";

    // Call .wav generation
    generateWAVs(clicks, elapsed.count());
}


void printWelcome() {
    // Print Welcome Message
    system("cls");
    std::cout << "\033[1;34m"
        << "Hello! Welcome to my beat visualiser program!\n"
        << "\033[0m" << std::flush;

    std::cout << "\033[1;36m"
        << "-\nHow does it work?\n"
        << "\033[0m" << std::endl;

    std::cout << "\033[1;32m"
        << "-> Listen to the music of your choice. \n"
        << "The Program will record the amount of clicks you do in a certain time "
        << "and then generate a .wav with loud noises at every beat "
        << "for you to use in video editing software to sync video to music.\n\n" << std::endl;

    std::cout << "-> Alternatively find out the BPM of your music. \n"
        << "The Program will generate a .wav with loud noises at every beat"
        << "for you to use in video editing software to sync video to music.\n\n"
        << "\033[0m" << std::endl;

    // Call Click-Record-Function
    clickRecorder();
}


int main()
{
    printWelcome();
    return 0;
}