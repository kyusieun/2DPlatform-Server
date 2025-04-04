#pragma once
#include <SFML/Network.hpp>
#include <memory>
#include <mutex>
#include <map>
#include <thread>

// --- 애니메이션 상수 정의 ---
const int FRAME_WIDTH = 64;   // 프레임 가로 크기 (픽셀)
const int FRAME_HEIGHT = 64;  // 프레임 세로 크기 (픽셀)
const int FRAMES_PER_ROW = 8; // !!! 스프라이트 시트 가로 프레임 수 - 실제 값 확인 필수 !!!

// 각 애니메이션 상태 정의
enum class PlayerAnimState
{
    Stand,  // Stand 애니메이션 사용
    Walk,   // Walk 애니메이션 사용
    Jump,   // Jump and Fall 애니메이션 사용
    Stance, // 전투 대기모션
};

// 패킷 타입 정의
enum class PacketType : uint8_t
{
    Welcome = 0,      // 서버 -> 클라이언트: 클라이언트 ID 전달
    PlayerState = 1,  // 서버 -> 클라이언트: 특정 플레이어 상태 전달
    PlayerInput = 2,  // 클라이언트 -> 서버: 플레이어 입력 전달
    PlayerJoined = 3, // 서버 -> 클라이언트: 새 플레이어 접속 알림
    PlayerLeft = 4,   // 서버 -> 클라이언트: 플레이어 접속 종료 알림
    MapData = 5,      // 맵 데이터 전송용
};

// 각 애니메이션의 정보 (시작 인덱스, 프레임 수, 프레임당 시간)
struct AnimationData
{
    int startFrameIndex; // 전체 시트 기준 시작 프레임 인덱스 (0부터 시작)
    int frameCount;      // 해당 애니메이션의 총 프레임 수
    float timePerFrame;  // 각 프레임의 재생 시간 (초)
};

extern std::map<PlayerAnimState, AnimationData> animData;

struct PlayerInputState
{
    bool up = false; // 점프 입력으로 사용
    bool down = false;
    bool left = false;
    bool right = false;
    bool jump = false; // 점프 입력용
};

// 연산자 선언
sf::Packet &operator<<(sf::Packet &packet, const PlayerInputState &input);
sf::Packet &operator>>(sf::Packet &packet, PlayerInputState &input);
sf::Packet &operator<<(sf::Packet &packet, PacketType type);
sf::Packet &operator>>(sf::Packet &packet, PacketType &type);

// ------------------------------------------------------

// 물리 상수
const float GRAVITY = 980.f;
const float JUMP_STRENGTH = -500.f;

// 맵 관련 상수
extern const int MAP_WIDTH;
extern const int MAP_HEIGHT;
constexpr float TILE_SIZE = 40.f;

// 맵 데이터
extern const std::vector<std::vector<int>> tileMap;

// 맵 관련 헬퍼 함수
bool isSolid(int x, int y);

// 서버 측 플레이어 정보
struct ServerPlayer
{
    uint32_t id = 0;            // 플레이어 ID (지금은 0)
    float x = 400.f;            // 초기 위치 X
    float y = 300.f;            // 초기 위치 Y
    float speed = 200.f;        // 플레이어 속도
    PlayerInputState lastInput; // 마지막으로 받은 입력 상태
    float velocityY = 0.f;      // 수직 속도
    bool isOnGround = false;    // 땅에 닿아 있는지 여부
    bool needsUpdate = true;    // 상태 변경 여부 플래그 (브로드캐스팅 최적화용)
};

// 연결된 클라이언트 정보를 담을 구조체
struct ClientInfo
{
    std::unique_ptr<sf::TcpSocket> socket; // 클라이언트 소켓 (unique_ptr로 소유권 관리)
    ServerPlayer player;                   // 해당 클라이언트의 플레이어 데이터
    std::thread thread;                    // 클라이언트 처리 스레드
    bool connected = true;                 // 연결 상태 플래그
    // std::atomic<bool> connected = true; // 스레드 안전하게 사용하려면 atomic

    // 생성자 선언
    ClientInfo(std::unique_ptr<sf::TcpSocket> s, uint32_t playerId);

    // 이동 생성자/대입 연산자
    ClientInfo(ClientInfo &&other) noexcept = default;
    ClientInfo &operator=(ClientInfo &&other) noexcept = default;

    // 복사 생성자/대입 연산자 삭제
    ClientInfo(const ClientInfo &) = delete;
    ClientInfo &operator=(const ClientInfo &) = delete;
};

// 애니메이션 데이터 초기화 (계산된 값 사용)

extern std::map<uint32_t, std::shared_ptr<ClientInfo>> clients;
extern std::mutex clientsMutex;
extern uint32_t nextPlayerId;