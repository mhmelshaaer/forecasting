#include <iostream>

#include <libnet.hpp>

enum class CustomMsgTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	ServerMessage,
};

class CustomServer : public olc::net::server_interface<CustomMsgTypes>
{
public:
	CustomServer(uint16_t nPort) : olc::net::server_interface<CustomMsgTypes>(nPort)
	{

	}
protected:
	virtual bool OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client){}
    virtual void OnClientDisconnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client){}
    virtual void OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client, olc::net::message<CustomMsgTypes>& msg){}
};

bool bQuit = false;

int main() {
	CustomServer server(60000);
	server.Start();

    while(!bQuit) {
        server.Update();
    }

	return 0;
}
