#include "handle_client.hpp"
#include "server_types.hpp"
#include <SFML/Network.hpp>
#include <iostream>

// --- 브로드캐스트 함수 (모든 클라이언트에게 패킷 전송) ---
void broadcastPacket(sf::Packet& packet, uint32_t senderId = -1) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto const& [id, client] : clients) {
        if (client && client->connected && id != senderId) {
            client->socket->send(packet);
        }
    }
}

// --- 클라이언트 처리 함수 ---
void handleClient(std::shared_ptr<ClientInfo> clientInfo) {
    uint32_t clientId = clientInfo->player.id;
    std::cout << "Client " << clientId << " connected." << std::endl;

    // 클라이언트 소켓 논블로킹 설정
    clientInfo->socket->setBlocking(false);

    //"Welcome" 메시지 전송
    sf::Packet welcomePacket;
    welcomePacket << PacketType::Welcome << clientId;
    clientInfo->socket->send(welcomePacket);

    //기존 다른 플레이어 정보 전송
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for(auto const& [id, otherClient] : clients) {
            if (id != clientId && otherClient && otherClient->connected) {
                sf::Packet existingPlayerPacket;
                existingPlayerPacket << PacketType::PlayerState 
                                   << otherClient->player.id 
                                   << otherClient->player.x 
                                   << otherClient->player.y;
                clientInfo->socket->send(existingPlayerPacket);
            }
        }
    }

    //자신의 접속 사실을 다른 플레이어에게 브로드캐스트
    sf::Packet joinedPacket;
    joinedPacket << PacketType::PlayerJoined 
                 << clientInfo->player.id 
                 << clientInfo->player.x 
                 << clientInfo->player.y;
    broadcastPacket(joinedPacket, clientId);


    //맵 데이터 전송
    sf::Packet mapPacket;
    mapPacket << PacketType::MapData << uint32_t(MAP_WIDTH) << uint32_t(MAP_HEIGHT);
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            mapPacket << int(tileMap[y][x]); // int -> sf::Int32
        }
    }
    clientInfo->socket->send(mapPacket);
    std::cout << "[Thread " << clientId << "] Sent Map Data." << std::endl;


    sf::Clock gameClock;

    // --- 클라이언트 처리 루프 ---
    while (clientInfo->connected) {
        sf::Time dt = gameClock.restart();
        float dtSeconds = dt.asSeconds();

        // --- 입력 수신 ---
        sf::Packet inputPacket;
        sf::Socket::Status receiveStatus = clientInfo->socket->receive(inputPacket);

        if (receiveStatus == sf::Socket::Status::Done) {
            PacketType packetType;
            PlayerInputState input;
            if (inputPacket >> packetType && packetType == PacketType::PlayerInput && inputPacket >> input) {
                clientInfo->player.lastInput = input;
                clientInfo->player.needsUpdate = true;
            }
        } else if (receiveStatus == sf::Socket::Status::Disconnected) {
            std::cout << "Client " << clientId << " disconnected." << std::endl;
            clientInfo->connected = false;
            break;
        } else if (receiveStatus != sf::Socket::Status::NotReady) {
            clientInfo->connected = false;
            break;
        }

        // --- 서버 측 상태 업데이트 (물리 & 충돌) ---
        ServerPlayer& player = clientInfo->player; // 편의상 레퍼런스 사용

        // ** 이전 프레임 상태 저장 및 현재 프레임 플래그 초기화 **
        bool wasOnGround = player.isOnGround;
        //player.isOnGround = false; //이부분 삭제
        
        float moveX = 0.f;
        float moveY = 0.f;
        // 플레이어 크기 (Shape 원점이 중앙이므로 절반 크기 사용)
        float playerHalfWidth = FRAME_WIDTH / 2.f;
        float playerHalfHeight = FRAME_HEIGHT / 2.f;

        // 여전히 땅에 닿아 있는지 확인
        if(player.isOnGround){
            // 플레이어의 발 밑 타일 확인
            int tileX_Left = static_cast<int>((player.x - playerHalfWidth) / TILE_SIZE);
            int tileX_Right = static_cast<int>((player.x + playerHalfWidth - 1) / TILE_SIZE);
            int tileY_Bottom = static_cast<int>((player.y + playerHalfHeight) / TILE_SIZE);

            if (!isSolid(tileX_Left, tileY_Bottom) && !isSolid(tileX_Right, tileY_Bottom)) {
                player.isOnGround = false;
            }
        }
        
        // 중력 적용
        if (!player.isOnGround) { // isOnGround가 false일때만 중력 적용
            player.velocityY += GRAVITY * dtSeconds;
        }

        // 점프 처리 - isOnGround를 사용하여 현재 프레임의 상태를 기준으로 점프 가능 여부 결정
        if (player.lastInput.jump && player.isOnGround) {
            player.velocityY = JUMP_STRENGTH;
            player.isOnGround = false;  // 점프 시작 시 땅에서 떨어짐
            player.needsUpdate = true;
        }

        // 좌우 이동량 계산
        if (player.lastInput.left)  moveX -= player.speed * dtSeconds;
        if (player.lastInput.right) moveX += player.speed * dtSeconds;

        // 수직 이동량 계산
        moveY = player.velocityY * dtSeconds;

        float nextX = player.x + moveX;
        float nextY = player.y + moveY;



        // 수평 충돌 확인 및 위치 조정
        int currentTileX = static_cast<int>((player.x) / TILE_SIZE);
        int nextTileX_Left = static_cast<int>((nextX - playerHalfWidth) / TILE_SIZE);
        int nextTileX_Right = static_cast<int>((nextX + playerHalfWidth - 1) / TILE_SIZE); // -1 for edge cases
        int tileY_Top = static_cast<int>((player.y - playerHalfHeight) / TILE_SIZE);
        int tileY_Bottom = static_cast<int>((player.y + playerHalfHeight - 1) / TILE_SIZE);
        int tileY_Mid = static_cast<int>(player.y / TILE_SIZE); // 중앙점 추가 확인

        if (moveX > 0) { // 오른쪽 이동 시
            if (isSolid(nextTileX_Right, tileY_Top) || isSolid(nextTileX_Right, tileY_Bottom) || isSolid(nextTileX_Right, tileY_Mid)) {
                nextX = (nextTileX_Right * TILE_SIZE) - playerHalfWidth; // 벽 오른쪽에 붙임
                moveX = 0; // 이동량 0
            }
        } else if (moveX < 0) { // 왼쪽 이동 시
             if (isSolid(nextTileX_Left, tileY_Top) || isSolid(nextTileX_Left, tileY_Bottom) || isSolid(nextTileX_Left, tileY_Mid)) {
                nextX = ((nextTileX_Left + 1) * TILE_SIZE) + playerHalfWidth; // 벽 왼쪽에 붙임
                moveX = 0;
            }
        }
        player.x = nextX; // 수평 위치 확정 (먼저 계산)


        // 수직 충돌 확인 및 위치 조정
        int currentTileY = static_cast<int>((player.y) / TILE_SIZE);
        int nextTileY_Top = static_cast<int>((nextY - playerHalfHeight) / TILE_SIZE);
        int nextTileY_Bottom = static_cast<int>((nextY + playerHalfHeight - 1) / TILE_SIZE);
         // x축 위치는 위에서 확정된 player.x 사용
        int tileX_Left = static_cast<int>((player.x - playerHalfWidth) / TILE_SIZE);
        int tileX_Right = static_cast<int>((player.x + playerHalfWidth - 1) / TILE_SIZE);

        if (moveY > 0) { // 아래로 이동 시 (낙하)
             if (isSolid(tileX_Left, nextTileY_Bottom) || isSolid(tileX_Right, nextTileY_Bottom)) {
                nextY = (nextTileY_Bottom * TILE_SIZE) - playerHalfHeight; // 땅 위에 붙임
                player.velocityY = 0;    // 속도 리셋
                player.isOnGround = true; // 땅에 닿음
                std::cout << " [Thread " << clientId << "] Player landed." << std::endl;
            }
        } else if (moveY < 0) { // 위로 이동 시 (점프)
             if (isSolid(tileX_Left, nextTileY_Top) || isSolid(tileX_Right, nextTileY_Top)) {
                nextY = ((nextTileY_Top + 1) * TILE_SIZE) + playerHalfHeight; // 천장에 붙임
                player.velocityY = 0; // 속도 리셋 (튕겨나오도록 할 수도 있음)
            }
        }

        // 최종 위치 업데이트
        if (player.x != nextX || player.y != nextY || player.isOnGround != wasOnGround) { // 실제 위치나 상태 변경이 있을 때만 업데이트 및 플래그
             player.needsUpdate = true;
        }
        player.x = nextX; // 수평 위치는 위에서 이미 확정
        player.y = nextY; // 수직 위치 확정


        // --- 상태 브로드캐스트 ---
        if (clientInfo->player.needsUpdate) {
            sf::Packet statePacket;
            statePacket << PacketType::PlayerState
                       << clientInfo->player.id
                       << clientInfo->player.x
                       << clientInfo->player.y
                       << clientInfo->player.isOnGround;
            
            // 모든 클라이언트에게 전송
            broadcastPacket(statePacket);
            
            clientInfo->player.needsUpdate = false;
        }

        sf::sleep(sf::milliseconds(10));
    }

    // --- 연결 종료 처리 ---
    sf::Packet leftPacket;
    leftPacket << PacketType::PlayerLeft << clientId;
    broadcastPacket(leftPacket, clientId);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(clientId);
        std::cout << "Client " << clientId << " removed. Active clients: " << clients.size() << std::endl;
    }
}
