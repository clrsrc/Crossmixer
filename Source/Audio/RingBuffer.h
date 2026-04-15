#pragma once
#include <atomic>
#include <vector>
#include <cstring>

// Lock-free Single-Producer Single-Consumer ring buffer for audio data.
// Stores interleaved float samples (numChannels samples per frame).
template <typename T = float>
class RingBuffer
{
public:
    RingBuffer(int numFrames, int numChannels)
        : capacity(numFrames),
          channels(numChannels),
          buffer(static_cast<size_t>(numFrames) * numChannels, T{}),
          readPos(0),
          writePos(0)
    {
    }

    // Returns number of frames available for reading
    int getAvailableFrames() const
    {
        int w = writePos.load(std::memory_order_acquire);
        int r = readPos.load(std::memory_order_relaxed);
        int avail = w - r;
        if (avail < 0) avail += capacity;
        return avail;
    }

    // Returns number of frames that can be written
    int getFreeFrames() const
    {
        return capacity - 1 - getAvailableFrames();
    }

    // Write interleaved frames. Returns number of frames actually written.
    int write(const T* data, int numFrames)
    {
        int available = getFreeFrames();
        int toWrite = (numFrames < available) ? numFrames : available;
        if (toWrite <= 0) return 0;

        int w = writePos.load(std::memory_order_relaxed);

        int firstPart = capacity - w;
        if (firstPart > toWrite) firstPart = toWrite;
        int secondPart = toWrite - firstPart;

        std::memcpy(buffer.data() + static_cast<size_t>(w) * channels,
                     data,
                     static_cast<size_t>(firstPart) * channels * sizeof(T));

        if (secondPart > 0)
        {
            std::memcpy(buffer.data(),
                         data + static_cast<size_t>(firstPart) * channels,
                         static_cast<size_t>(secondPart) * channels * sizeof(T));
        }

        int newW = (w + toWrite) % capacity;
        writePos.store(newW, std::memory_order_release);

        return toWrite;
    }

    // Read interleaved frames. Returns number of frames actually read.
    int read(T* data, int numFrames)
    {
        int available = getAvailableFrames();
        int toRead = (numFrames < available) ? numFrames : available;
        if (toRead <= 0) return 0;

        int r = readPos.load(std::memory_order_relaxed);

        int firstPart = capacity - r;
        if (firstPart > toRead) firstPart = toRead;
        int secondPart = toRead - firstPart;

        std::memcpy(data,
                     buffer.data() + static_cast<size_t>(r) * channels,
                     static_cast<size_t>(firstPart) * channels * sizeof(T));

        if (secondPart > 0)
        {
            std::memcpy(data + static_cast<size_t>(firstPart) * channels,
                         buffer.data(),
                         static_cast<size_t>(secondPart) * channels * sizeof(T));
        }

        int newR = (r + toRead) % capacity;
        readPos.store(newR, std::memory_order_release);

        return toRead;
    }

    void reset()
    {
        readPos.store(0, std::memory_order_relaxed);
        writePos.store(0, std::memory_order_relaxed);
    }

    int getNumChannels() const { return channels; }
    int getCapacity() const { return capacity; }

private:
    int capacity;
    int channels;
    std::vector<T> buffer;
    std::atomic<int> readPos;
    std::atomic<int> writePos;
};
