#pragma once

template <typename T>
class Connection : public std::enable_shared_from_this<Connection<T>>
{
public:
	Connection(uint32_t id, asio::io_context& asio_context, asio::ip::tcp::socket socket, tsque<Sent_message<T>>& incoming_messages, std::function<void(uint32_t, Sent_message<T>&)> messages_handler)
		:m_ID(id), m_context(asio_context), m_socket(std::move(socket)), m_inMsg(incoming_messages), m_handledMsg(T(0)), m_ownedByServer(true)
	{
		m_serverMsgHandler = messages_handler;
	}
	
	Connection(asio::io_context& asio_context, asio::ip::tcp::socket socket, tsque<Sent_message<T>>& incomming_messages, std::function<void(Sent_message<T>&)> messages_handler)
		:m_context(asio_context), m_socket(std::move(socket)), m_inMsg(incomming_messages), m_handledMsg(T(0)), m_ownedByServer(false)
	{
		m_clientMsgHandler = messages_handler;
	}

	inline bool IsConnected() const { return m_socket.is_open(); }



	void ConnectToClient()
	{
		if (m_ownedByServer)
		{
			if (IsConnected())
			{
				ReadID();
			}
		}
	}

	void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
	{
		if (!m_ownedByServer)
		{
			asio::async_connect(m_socket, endpoints,
				[this](asio::error_code ec, asio::ip::tcp::endpoint endpoint)
				{
					if (!ec)
					{
						std::cout << "Connected to server\n";
						ReadID();
					}
					else
					{
						std::cout << "Error " << ec << " occured while trying to connect client to " << endpoint << std::endl;
					}
				});

		}
	}

	void Disconnect()
	{
		std::cout << "Disconnected\n";
		if(IsConnected())
			asio::post(m_context, [this]() { m_socket.close(); });
	}


	void Send(const Sent_message<T>& msg)
	{
		asio::post(m_context, [this, msg]() {
			bool not_writing = m_outMsg.empty();
			m_outMsg.push_back(msg);
			if (not_writing)
			{
				WriteID();
			}
			});
	}

private:
	void ReadID()
	{
		asio::async_read(m_socket, asio::buffer(&m_handledMsg.ID, sizeof(uint32_t)),
			[this](asio::error_code ec, size_t size)
			{
				if (ec)
				{
					std::cout << "Error occured while trying to read ID: " << ec.message() << "\n";
					m_socket.close();
				}
				else
				{
					ReadHeader();
				}
			});
	}

	void ReadHeader()
	{
		asio::async_read(m_socket, asio::buffer(&m_handledMsg.message.header, Message<T>::SizeOfHeader()), 
			[this](asio::error_code ec, size_t size) 
			{
				if (ec)
				{
					std::cout << "Error occured while trying to read header: "<<ec.message()<<"\n";
					m_socket.close();
				}
				else
				{
					if (m_handledMsg.message.header.size)
					{
						m_handledMsg.message.data.resize(m_handledMsg.message.header.size);
						ReadBody();
					}
					else
					{
						std::cout << "Received an empty message\n";
						AddIncomingMsg();
					}
				}
			});
	}

	void ReadBody()
	{
		asio::async_read(m_socket, asio::buffer(m_handledMsg.message.data.data(), m_handledMsg.message.data.size()),
			[this](asio::error_code ec, size_t size)
			{
				if (ec)
				{
					std::cout << "Error occured while trying to read body: " << ec.message() << "\n";
					m_socket.close();
				}
				else
				{
					AddIncomingMsg();
				}
			});
	}


	void WriteID()
	{
		asio::async_write(m_socket, asio::buffer(&m_outMsg.front().ID, sizeof(uint32_t)),
			[this](asio::error_code ec, size_t size)
			{
				if (ec)
				{
					std::cout << "Error occured while trying to write header: " << ec.message() << "\n";
					m_socket.close();
				}
				else
				{
					WriteHeader();
				}
			});
	}

	void WriteHeader()
	{
		asio::async_write(m_socket, asio::buffer(&m_outMsg.front().message.header, Message<T>::SizeOfHeader()),
			[this](asio::error_code ec, size_t size)
			{
				if (ec)
				{
					std::cout << "Error occured while trying to write header: " << ec.message() << "\n";
					m_socket.close();
				}
				else
				{
					if (m_outMsg.front().message.data.size())
					{
						WriteBody();
					}
					else
					{
						PopOutcomingMsg();
					}
				}
			});
	}

	void WriteBody()
	{
		if (m_outMsg.front().message.data.size() != m_outMsg.front().message.header.size)
			std::cout << "Not fine\n";
		asio::async_write(m_socket, asio::buffer(m_outMsg.front().message.data.data(), m_outMsg.front().message.header.size),
			[this](asio::error_code ec, size_t size)
			{
				if (ec)
				{
					std::cout << "Error occured while trying to write body: " << ec.message() << "\n";
					m_socket.close();
				}
				else
				{
					PopOutcomingMsg();
				}
			});
	}


	void AddIncomingMsg()
	{
		if (m_ownedByServer)
			m_serverMsgHandler(m_ID, m_handledMsg);
		else
			m_clientMsgHandler(m_handledMsg);


		ReadID();
	}

	void PopOutcomingMsg()
	{
		m_outMsg.pop_front();
		if (!m_outMsg.empty())
		{
			WriteID();
		}
	}

public:
	const bool					m_ownedByServer = true;

	asio::ip::tcp::socket		m_socket;
	asio::io_context&			m_context;

	tsque<Sent_message<T>>		m_outMsg;
	tsque<Sent_message<T>>&		m_inMsg;
	Sent_message<T>				m_handledMsg;

	uint32_t					m_ID;

	std::function<void(uint32_t, Sent_message<T>&)> m_serverMsgHandler;

	std::function<void(Sent_message<T>&)>			m_clientMsgHandler;
};