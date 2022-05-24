//
// Created by elshaaer on 12/04/2021.
//

#ifndef LIB_TEST_NET_CONNECTION_H
#define LIB_TEST_NET_CONNECTION_H

#include "net_common.hpp"

namespace forecasting {
    namespace net {
	    template <typename T>
        class connection;
	    
        template<typename T>
        class connection : public std::enable_shared_from_this<connection<T>> {
        public:

        	enum class owner { server, client };

            // Constructor: Specify Owner, connect to context, transfer the socket
            //				Provide reference to incoming message queue
            connection(
            	owner _owner,
            	asio::io_context& asio_context,
            	asio::ip::tcp::socket socket,
            	tsqueue<owned_message<T>>& q_in
            ) : m_asio_context(asio_context),
                m_socket(std::move(socket)),
                m_q_messages_in(q_in)
            {
                m_owner = _owner;
            }

            virtual ~connection(){}

	        std::uint32_t get_id() const {
            	return id;
            }

	        void ConnectToClient(std::uint32_t uid = 0)
            {
                if (m_owner == owner::server)
                {
                    if (m_socket.is_open())
                    {
                        id = uid;
                        ReadHeader();
                    }
                }
            }

            void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
            {
                // Only clients can connect to servers
                if (m_owner == owner::client)
                {
                    // Request asio attempts to connect to an endpoint
                    asio::async_connect(
                    	m_socket,
                    	endpoints,
                        [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
                        {
                            if (!ec)
                            {
	                            ReadHeader();
                            }
                        }
                    );
                }
            }

            void Disconnect()
            {
                if (IsConnected())
                    asio::post(m_asio_context, [this]() { m_socket.close(); });
            }

            bool IsConnected() const
            {
                return m_socket.is_open();
            }

            // ASYNC - Send a message, connections are one-to-one so no need to specifiy
            // the target, for a client, the target is the server and vice versa
            void Send(const message<T>& msg)
            {
                asio::post(
                	m_asio_context,
                   [this, msg]() {
                       // If the queue has a message in it, then we must
                       // assume that it is in the process of asynchronously being written.
                       // Either way add the message to the queue to be output. If no messages
                       // were available to be written, then start the process of writing the
                       // message at the front of the queue.
                       bool bWritingMessage = !m_q_messages_out.empty();
	                   m_q_messages_out.push_back(msg);
                       if (!bWritingMessage)
                       {
	                       WriteHeader();
                       }
                   }
               );
            }

        private:
	        // ASYNC - Prime context to write a message header
            void WriteHeader() {
		        asio::async_write(
		        	m_socket,
		        	asio::buffer(&m_q_messages_out.front().header, sizeof(message_header<T>)),
		        	[this] (std::error_code ec, std::size_t length) {
		        		// return if an error occurred
		        		if (ec) {
					        std::cout<< "[" << id << "] Write header fail.\n";
					        m_socket.close();
					        return;
		        		}
		        		
		        		// If no body attached, pop the message and process the next if possible
		        		if (m_q_messages_out.front().header.size == 0) {
					        m_q_messages_out.pop_front();
		        			
		        			// check if there are other out messages to process
		        			if (!m_q_messages_out.empty()) {
						        WriteHeader();
		        			}
		        			
		        			return;
		        		}
		        		
		        		// In case there is a body to write
				        WriteBody();
		        	}
	            );
            }
	        // ASYNC - Prime context to write a message body
            void WriteBody() {
            	asio::async_write(
            		m_socket,
            		asio::buffer(m_q_messages_out.front().body.data(), m_q_messages_out.front().body.size()),
            		[this] (std::error_code ec, std::size_t length) {
            			if (ec) {
            			    std::cout << "[" << id << "] Write header fail.\n";
            			    m_socket.close();
            			}
            			
            			m_q_messages_out.pop_front();
            			
            			if (!m_q_messages_out.empty()) {
            				WriteHeader();
            			}
            		}
                );
            }
	        // ASYNC - Prime context ready to read a message header
            void ReadHeader() {
		        asio::async_read(
		        	m_socket,
		        	asio::buffer(&m_msg_temporary_in.header, sizeof(message_header<T>)),
		        	[this] (std::error_code ec, std::size_t length) {
		        		if (ec) {
		        			std::cout<< "[" << id << "] Read header fail.\n";
		        			m_socket.close();
		        		    return;
		        		}
		        		
		        		if (m_msg_temporary_in.header.size == 0) {
				            AddToIncomingMessageQueue();
		        			return;
		        		}
		        		
		        		m_msg_temporary_in.body.resize(m_msg_temporary_in.header.size);
		        		ReadBody();
		        	}
	            );
            }
            // ASYNC - Prime context ready to read a message body
            void ReadBody() {
	            asio::async_read(
            		m_socket,
            		asio::buffer(m_msg_temporary_in.body.data(), m_msg_temporary_in.body.size()),
            		[this] (std::error_code ec, std::size_t length) {
            			if (ec) {
				            // As above!
                            std::cout << "[" << id << "] Read Body Fail.\n";
                            m_socket.close();
                            return;
            			}
            			
			            // ...and they have! The message is now complete, so add
                        // the whole message to incoming queue
                        AddToIncomingMessageQueue();
            		}
                );
            }
            
	        void AddToIncomingMessageQueue() {
		        ReadHeader();
                if (m_owner == owner::server)
                {
                	m_q_messages_in.push_back({ this->shared_from_this(), m_msg_temporary_in });
                	return;
                }
                m_q_messages_in.push_back({nullptr, m_msg_temporary_in });
            }

        protected:
            // Each connection has a unique socket to a remote
            asio::ip::tcp::socket m_socket;

            // This context is shared with the whole asio instance
            asio::io_context& m_asio_context;

            // This queue holds all messages to be sent to the remote side
            // of this connection
            tsqueue<message<T>> m_q_messages_out;

            // This references the incoming queue of the parent object
            tsqueue<owned_message<T>>& m_q_messages_in;
            
			// Incoming messages are constructed asynchronously, so we will
			// store the part assembled message here, until it is ready
			message<T> m_msg_temporary_in;
			
            owner m_owner = owner::server;

	        std::uint32_t id = 0;
        };
    }
}
#endif //LIB_TEST_NET_CONNECTION_H
