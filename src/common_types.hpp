#pragma once
#include <SFML/System.hpp>  // sf::Time 등 사용
#include <SFML/Network.hpp> // sf::Packet 사용
#include <cstdint>
#include <map>

// --- Animation Constants ---
const int FRAME_WIDTH = 64;
const int FRAME_HEIGHT = 64;
const int FRAMES_PER_ROW = 8;
const float STATE_CHANGE_COOLDOWN = 0.1f;

// Animation state definitions
enum class PlayerAnimState : uint8_t
{ // 명시적 타입 지정 권장
    Stand,
    Walk,
    Jump,
    Stance // Combat stance (currently unused)
};

// Animation data structure
struct AnimationData
{
    int startFrameIndex;
    int frameCount;
    float timePerFrame;
};

extern std::map<PlayerAnimState, AnimationData> animData;

// Player input state structure
struct PlayerInputState
{
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool jump = false;
};

// Packet type enumeration
enum class PacketType : uint8_t
{
    Welcome,
    PlayerState,
    PlayerInput,
    PlayerJoined,
    PlayerLeft,
    MapData
};

sf::Packet &operator<<(sf::Packet &packet, const PlayerInputState &input);
sf::Packet &operator>>(sf::Packet &packet, PlayerInputState &input);
sf::Packet &operator<<(sf::Packet &packet, PacketType type);
sf::Packet &operator>>(sf::Packet &packet, PacketType &type);

// --- Map Constants ---
const float CLIENT_TILE_SIZE = 40.f;