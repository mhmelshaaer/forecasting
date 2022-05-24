#include <iostream>
#include <numeric>

#include <libnet.hpp>

#define MAKE_STR(x) _MAKE_STR(x)
#define _MAKE_STR(x) #x

#ifndef SERVER_HOST_NAME
#define SERVER_HOST_NAME "server"
#endif

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
    typedef client_interface<MessageTypes> Client;
}

namespace fnet = forecasting::net;

class SensoryNodeClient : public fnet::Client
{
public:
};

bool bQuit = false;
typedef float temperature_t;
std::vector<temperature_t> received_temperatures;
std::vector<std::packaged_task<temperature_t()>> average_tasks;
std::vector<std::packaged_task<temperature_t()>> accumulation_tasks;
std::mutex temperatures_lock;
std::mutex average_tasks_lock;
std::mutex accumulation_tasks_lock;
std::condition_variable average_tasks_ready;
std::condition_variable accumulation_tasks_ready;

temperature_t calculate_average(const std::vector<temperature_t>& temperatures) {
	if (temperatures.empty()) return 0;
	temperature_t sum = std::accumulate(temperatures.begin(), temperatures.end(), 0.0f);
	return sum / static_cast<temperature_t>(temperatures.size());
}

temperature_t calculate_sum(const std::vector<temperature_t>& temperatures) {
	return std::accumulate(temperatures.begin(), temperatures.end(), 0.0f);
}

void task_generator() {
	while(!bQuit) {
		// every 5 seconds
		std::this_thread::sleep_for(std::chrono::seconds(5));
		
		{
			std::lock_guard<std::mutex> locker1(temperatures_lock);
			
			// adding an average task. Note, the received_temperatures should be copied.
			std::lock_guard<std::mutex> locker2(average_tasks_lock);
			average_tasks.emplace_back(
				std::packaged_task<temperature_t()> {
					std::bind(calculate_average, received_temperatures)
				}
			);
			std::cout << "average_tasks: " << average_tasks.size() << std::endl;
			average_tasks_ready.notify_one();
			
			// adding a sum task. Note, the received_temperatures should be copied.
			std::lock_guard<std::mutex> locker3(accumulation_tasks_lock);
			accumulation_tasks.emplace_back(
				std::packaged_task<temperature_t()> {
					std::bind(calculate_sum, received_temperatures)
				}
			);
			std::cout << "accumulation_tasks: " << accumulation_tasks.size() << std::endl;
			accumulation_tasks_ready.notify_one();
			
			// clearing the current values;
			received_temperatures.clear();
		}
		
	}
}

void consume_average_tasks() {
	while(!bQuit) {
		std::packaged_task<temperature_t()> t;
		{
			std::unique_lock<std::mutex> locker(average_tasks_lock);
			// wait for an average task to work on
			average_tasks_ready.wait(locker);
			t = std::move(average_tasks.front());
			average_tasks.pop_back();
		}
		auto result = t.get_future();
		t();
		std::cout<<"[TASK] Average: "<< result.get() <<std::endl;
	}
}

void consume_accumulation_tasks() {
	while(!bQuit) {
		std::packaged_task<temperature_t()> t;
		{
			std::unique_lock<std::mutex> locker(accumulation_tasks_lock);
			// wait for an accumulation task to work on
			accumulation_tasks_ready.wait(locker);
			t = std::move(accumulation_tasks.front());
			accumulation_tasks.pop_back();
		}
		auto result = t.get_future();
		t();
		std::cout<<"[TASK] Accumulation: "<< result.get() <<std::endl;
	}
}


/**
 * Signal handler.
 *
 * @param s signal
 */
void handle_exit_signal(int s){
  std::cout << "[CLIENT] terminating...\n";
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

	std::thread t1(task_generator);
	std::thread t2(consume_average_tasks);
	std::thread t3(consume_accumulation_tasks);

	SensoryNodeClient c;
	c.Connect(MAKE_STR(SERVER_HOST_NAME), 60000);

	while (!bQuit)
	{
		if (c.IsConnected())
		{
			while(!c.Incoming().empty())
			{
				auto msg = c.Incoming().pop_front().msg;

				switch (msg.header.id)
				{
                case MessageTypes::ServerAccept:
                {
                    // Server has responded to a ping request
                    std::cout << "Server Accepted Connection\n";
                }
                break;

                case MessageTypes::ServerDeny:
                {
                    // Server has responded to a ping request
                    std::cout << "Server Denied Connection\n";
			        bQuit = true;
                }
                break;

                case MessageTypes::NodeTemperature:
                {
                    // Server has responded to a ping request
                    temperature_t temperature;
                    msg >> temperature;
                    std::cout << "Client: " << temperature << std::endl;
                    
                    {
                        std::lock_guard<std::mutex> locker(temperatures_lock);
                        received_temperatures.emplace_back(temperature);
                    }
                }
                break;
                }
			}
		}
		else
		{
			std::cout << "Server Down\n";
			bQuit = true;
		}
	}
  

	t1.join();
	t2.join();
	t3.join();

    std::cout << "[CLIENT] Stopped successfully.\n";

    return 0;
}