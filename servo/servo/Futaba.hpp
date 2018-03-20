#ifndef __Futaba__
#define __Futaba__

#include "Servo.hpp"
#include "ByteUnion.hpp"

template <uint8_t id>
class Futaba : public Servo<id> {
    template<size_t length>
    static constexpr uint8_t calcChecksum(const std::array<uint8_t, length> data) {
        uint8_t ret = data[0];
        for (int i = 1; i < length; ++i) {
            ret ^= data[i];
        }
        return ret;
    }
    template <uint8_t address, uint8_t value>
    void writeMemory(bool *success) {
        constexpr std::array<uint8_t, 6> packetForChecksum({
            id,
            0, /* No return packet */
            address,
            1,
            1, /* Count */
            value
        });
        constexpr uint8_t checksum = calcChecksum(packetForChecksum);
        constexpr std::array<uint8_t, 9> packet({
            0xFA, 0xAF, id, 0, address, 1, 1, value, checksum
        });
        if (success) {
            *success = this->serial->transfer(packet);
        } else {
            this->serial->transfer(packet);
        }
    }
    template <uint8_t address, typename T>
    void writeMemory(T data, bool *success) {
        constexpr std::array<uint8_t, 2> header({
            0xFA, 0xAF, /* Header */
        });
        constexpr std::array<uint8_t, 5> packetForChecksum({
            id,
            0, /* No return packet */
            address,
            sizeof(data),
            1, /* Count */
        });
        constexpr uint8_t checksum = calcChecksum(packetForChecksum);
        uint8_t checksumVar = checksum;
        ByteUnion<T> dataByte(data);
        for (uint8_t v : dataByte.array) {
            checksumVar ^= v;
        }
        auto dataArray = dataByte.arrayObj();
        if (success) {
            *success = this->serial->transfer(header, packetForChecksum,
                                              dataArray, std::array<uint8_t, 1>({checksumVar}));
        } else {
            this->serial->transfer(header, packetForChecksum, dataArray, std::array<uint8_t, 1>({checksumVar}));
        }
    }
    template <uint8_t address, typename T>
    T readMemory(bool *success) {
        constexpr std::array<uint8_t, 8> packet({
            0xFA, 0xAF, /* Header */
            id,
            0x0F, /* Flag */
            address,
            sizeof(T),
            0, /* Count */
            id ^ 0x0F ^ address ^ sizeof(T) /* Checksum */
        });
        std::array<uint8_t, 8 + sizeof(T)> response;
        bool ret = this->serial->transfer(response, packet);
        union ReturnPacket {
            uint8_t raw[8 + sizeof(T)];
            struct __attribute__((packed)) {
                uint16_t header;
                uint8_t identifier;
                uint8_t flags;
                uint8_t registerAddress;
                uint8_t length;
                uint8_t count;
                T data;
                uint8_t checksum;
            };
        };
        ReturnPacket rePacket;
        std::copy(response.begin(), response.end(), std::begin(rePacket.raw));
        if (success) {
            *success = ret;
        }
        return rePacket.data;
    }
    
public:
    Futaba(Serial *_serial) : Servo<id>(_serial) {}
    void setTorque(bool enable, bool *success = nullptr) {
        if (enable) {
            writeMemory<0x24, 1>(success);
        } else {
            writeMemory<0x24, 0>(success);
        }
    }
    void setPosition(double position, bool *success = nullptr) {
        writeMemory<0x1E>(static_cast<int16_t>(position * 10), success);
    }
    void setPosition(int16_t position, bool *success = nullptr) {
        writeMemory<0x1E>(position, success);
    }
    int16_t intPosition(bool *success = nullptr) {
        return readMemory<0x2A, int16_t>(success);
    }
    double position(bool *success = nullptr) {
        return readMemory<0x2A, int16_t>(success) / 10;
    }
};

#endif