#include <iostream>
#include <random>

// For listening for Ctrl + c
#include <cstdio>

#include <libnet.hpp>

enum class MessageTypes : uint32_t
{
	ServerAccept,
	ServerDeny,
	ServerPing,
	NodeTemperature,
};

namespace forecasting::net
{
    typedef connection<MessageTypes> Connection;
    typedef message<MessageTypes> Message;
    typedef server_interface<MessageTypes> Server;
}

namespace fnet = forecasting::net;

class SensoryNode : public forecasting::net::Server
{
public:
	SensoryNode(uint16_t nPort) : fnet::Server(nPort)
	{

	}
protected:
	virtual bool OnClientConnect(std::shared_ptr<fnet::Connection> client)
    {
		fnet::Message msg;
		msg.header.id = MessageTypes::ServerAccept;
		client->Send(msg);
		return true;
    }

    virtual void OnClientDisconnect(std::shared_ptr<fnet::Connection> client)
    {
		std::cout << "Removing client [" << client->get_id() << "]\n";
    }

    virtual void OnMessage(std::shared_ptr<fnet::Connection> client, fnet::Message& msg)
    {
		switch (msg.header.id)
		{
			case MessageTypes::ServerPing:
			{
				std::cout << "[" << client->get_id() << "]: Server Ping\n";
	
				// Simply bounce message back to client
				client->Send(msg);
			}
			break;
            default:
            break;
		}
    }
};

bool bQuit = false;
typedef float temperature_t;
temperature_t curr_temp;
std::mutex mu;
std::condition_variable read_temperature;

void temperature_sensor() {
	std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_real_distribution<temperature_t> generate_temp(-30.0f, 50.0f);

	while (!bQuit) {
		std::unique_lock<std::mutex> locker(mu);
		curr_temp = generate_temp(rng);
		locker.unlock();
		read_temperature.notify_one();
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
	read_temperature.notify_all();
	std::cout << "Sensor exit\n";
}

void temperature_reader(fnet::Server& server) {
	while(!bQuit) {
		std::unique_lock<std::mutex> locker(mu);
		read_temperature.wait(locker);
		locker.unlock();
		fnet::Message message;
		message.header.id = MessageTypes::NodeTemperature;
		message << curr_temp;
		server.MessageAllClients(message);
		std::cout << "Current temperature: " << curr_temp << std::endl;
	}
	std::cout << "Reader exit\n";
}

/**
 * Signal handler.
 *
 * @param s signal
 */
void handle_exit_signal(int s){
  std::cout << "[SERVER] terminating...\n";
  bQuit = true;
}

/**
 * When hitting ctrl + c while running the client, the program will exist
 * expectedly.
 */
void listen_for_ctrl_c() {
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = handle_exit_signal;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
}

int main() {
    listen_for_ctrl_c();
    
	SensoryNode server(60000);
	server.Start();
	
	std::thread reader(temperature_reader, std::ref(server));
	std::thread thermometer(temperature_sensor);
	
	thermometer.join();
	reader.join();

	return 0;
}
