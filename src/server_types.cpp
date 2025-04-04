#include "server_types.hpp"

// 전역 변수 정의
std::map<uint32_t, std::shared_ptr<ClientInfo>> clients;
std::mutex clientsMutex;
uint32_t nextPlayerId = 0;

// 맵 데이터 초기화 (0: 빈 공간, 1: 벽)
const int MAP_WIDTH = 50;  // 맵 가로 크기 증가
const int MAP_HEIGHT = 30; // 맵 세로 크기 증가
const std::vector<std::vector<int>> tileMap = []() {
    std::vector<std::vector<int>> map(MAP_HEIGHT, std::vector<int>(MAP_WIDTH, 0));

    // 바닥 생성
    for (int x = 0; x < MAP_WIDTH; ++x) {
        map[MAP_HEIGHT - 1][x] = 1;  // 맨 아래 바닥
    }

    for (int x = 5; x < 15; ++x) {
        map[MAP_HEIGHT - 4][x] = 1;
    }


    for (int x = 17; x < 28; ++x) {
        map[MAP_HEIGHT - 6][x] = 1;
    }


    for (int x = 30; x < 45; ++x) {
        map[MAP_HEIGHT - 8][x] = 1;
    }

    // 왼쪽 벽
    for (int y = MAP_HEIGHT - 10; y < MAP_HEIGHT - 1; ++y) {
        map[y][0] = 1;
    }

    // 오른쪽 벽
    for (int y = MAP_HEIGHT - 10; y < MAP_HEIGHT - 1; ++y) {
        map[y][MAP_WIDTH - 1] = 1;
    }

    return map;
}();

// 헬퍼 함수 구현
bool isSolid(int x, int y) {
    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) {
        return true; // 맵 밖은 벽으로 간주
    }
    return tileMap[y][x] == 1;
}

// ClientInfo 생성자 구현
ClientInfo::ClientInfo(std::unique_ptr<sf::TcpSocket> s, uint32_t playerId)
    : socket(std::move(s)) {
    player.id = playerId;
}

// Packet 연산자 구현
sf::Packet& operator <<(sf::Packet& packet, const PlayerInputState& input) {
    return packet << input.up << input.down << input.left << input.right << input.jump;
}


sf::Packet& operator >>(sf::Packet& packet, PlayerInputState& input) {
    return packet >> input.up >> input.down >> input.left >> input.right >> input.jump;
}

sf::Packet& operator <<(sf::Packet& packet, PacketType type) {
    return packet << static_cast<uint8_t>(type);
}

sf::Packet& operator >>(sf::Packet& packet, PacketType& type) {
    uint8_t value;
    packet >> value;
    type = static_cast<PacketType>(value);
    return packet;
} 