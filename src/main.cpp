#include "server_types.hpp"
#include "handle_client.hpp"
#include <iostream>
#include <thread>    // std::thread 사용
#include <vector>    // 클라이언트 목록 관리
#include <mutex>     // 공유 데이터 접근 제어
#include <memory>    // 스마트 포인터 사용
#include <atomic>    // 스레드 안전한 플래그 등 (선택 사항)
#include <list>      // 클라이언트 목록 변경 용이성 위해 list 사용 가능
#include <map>       // ID 기반 클라이언트 관리 용이성 위해 map 사용 가능


int main()
{
    // --- 리스너 설정 ---
    sf::TcpListener listener;
    unsigned short port = 53000;
    
    if (listener.listen(port) != sf::Socket::Status::Done)
    {
        std::cerr << "Failed to bind server to port " << port << std::endl;
        return -1;
    }
    std::cout << "Server successfully bound to port " << port << std::endl;
    
    // --- 서버 메인 루프 ---
    while (true) {
        auto newSocket = std::make_unique<sf::TcpSocket>();
        if (listener.accept(*newSocket) == sf::Socket::Status::Done) {
            // 새 클라이언트 연결 성공
            std::cout << "New connection request accepted." << std::endl;

            // 새 클라이언트 정보 생성 및 목록 추가 (뮤텍스로 보호)
            std::shared_ptr<ClientInfo> newClient = nullptr;
            {
                std::lock_guard<std::mutex> lock(clientsMutex); // 뮤텍스 잠금
                uint32_t clientId = nextPlayerId++;
                newClient = std::make_shared<ClientInfo>(std::move(newSocket), clientId);
                clients[clientId] = newClient; // 맵에 추가
                std::cout << "Client assigned ID: " << clientId << std::endl;
            } // 뮤텍스 자동 해제

            // 새 클라이언트를 위한 처리 스레드 시작
            // handleClient 함수를 새 스레드에서 실행하도록 설정
            // newClient shared_ptr을 복사하여 스레드로 전달
            newClient->thread = std::thread(handleClient, newClient);
            newClient->thread.detach(); // 스레드를 분리하여 메인 스레드와 독립적으로 실행 (종료 처리 필요)
                                        // 또는 joinable 상태로 두고 나중에 관리

        } else {
            // 연결 수락 실패 (리스너 소켓 문제?)
            std::cerr << "Warning: Failed to accept new connection." << std::endl;
            sf::sleep(sf::milliseconds(100)); // 잠시 대기 후 재시도
        }
    }
    return 0; // 실제로는 도달 X
}
