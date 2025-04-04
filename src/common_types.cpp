#include "common_types.hpp" // Include the header!

// Animation data map (전역 또는 AnimationManager 클래스 멤버로 이동 가능)
std::map<PlayerAnimState, AnimationData> animData = {
    {PlayerAnimState::Stand, {64, 1, 0.18f}}, // Stand: start at 64, 1 frame
    {PlayerAnimState::Walk, {32, 8, 0.1f}},   // Walk: start at 32, 8 frames
    {PlayerAnimState::Jump, {42, 6, 0.1f}},   // Jump and Fall: start at 42, 6 frames
    {PlayerAnimState::Stance, {0, 4, 0.18f}}  // Stance: start at 0, 4 frames (unused for now)
};

// --- Packet operator overloads ---
// (별도 packet_operators.cpp 로 분리해도 좋음)

sf::Packet &operator<<(sf::Packet &packet, const PlayerInputState &input)
{
    return packet << input.up << input.down << input.left << input.right << input.jump;
}

sf::Packet &operator>>(sf::Packet &packet, PlayerInputState &input)
{
    return packet >> input.up >> input.down >> input.left >> input.right >> input.jump;
}

sf::Packet &operator<<(sf::Packet &packet, PacketType type)
{
    return packet << static_cast<uint8_t>(type);
}

sf::Packet &operator>>(sf::Packet &packet, PacketType &type)
{
    uint8_t value;
    packet >> value;
    type = static_cast<PacketType>(value);
    return packet;
}
