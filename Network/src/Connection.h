#pragma once

//T is enum of messages types
template <typename T>
class Connection : public std::enable_shared_from_this<Connection<T>>
{
public:
	/// <summary>
	/// Constructs connection used by server
	/// </summary>
	/// <param name="id">:					id of the connection in server</param>
	/// <param name="asio_context">:		server's asio context</param>
	/// <param name="socket">:				client's socket (where messages come from and are sent to)</param>
	/// <param name="messages_handler">:	function that is called when the connection (thus server) receives a message</param>
	Connection(uint32_t id, asio::io_context& asio_context, asio::ip::tcp::socket socket, std::function<void(uint32_t, Sent_message<T>&)> messages_handler)
		:m_ID(id), m_context(asio_context), m_socket(std::move(socket)), m_handledMsg(T(0)), m_ownedByServer(true)
	{
		m_serverMsgHandler = messages_handler;
	}
	
	/// <summary>
	/// Constructs connection used by client
	/// </summary>
	/// <param name="asio_context">:		client's asio context</param>
	/// <param name="socket">:				server's socket (where messages come from and are sent to)</param>
	/// <param name="messages_handler">:	function that is called when the connection (thus client) receives a message</param>
	Connection(asio::io_context& asio_context, asio::ip::tcp::socket socket, std::function<void(Sent_message<T>&)> messages_handler)
		:m_context(asio_context), m_socket(std::move(socket)), m_handledMsg(T(0)), m_ownedByServer(false)
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
				//Basically server just starts to wait for messages
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
						Debug::Message("Connected to server");
						ReadID();
					}
					else
					{
						Debug::Message("Error occured while trying to connect to server: ", ec.message());
					}
				});

		}
	}

	void Disconnect()
	{
		Debug::Message("Disconnected");
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
					Debug::Message("Error occured while trying to read ID: ", ec.message());
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
					Debug::Message("Error occured while trying to read header: ", ec.message());
					m_socket.close();
				}
				else
				{
					if (m_handledMsg.message.header.size)
					{
						m_handledMsg.message.body.resize(m_handledMsg.message.header.size);
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
		asio::async_read(m_socket, asio::buffer(m_handledMsg.message.body.data(), m_handledMsg.message.body.size()),
			[this](asio::error_code ec, size_t size)
			{
				if (ec)
				{
					Debug::Message("Error occured while trying to read body: ", ec.message());
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
					Debug::Message("Error occured while trying to write ID: ", ec.message());
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
					Debug::Message("Error occured while trying to write header: ", ec.message());
					m_socket.close();
				}
				else
				{
					if (m_outMsg.front().message.body.size())
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
		if (m_outMsg.front().message.body.size() != m_outMsg.front().message.header.size)
			Debug::Message("Size mismatch");
		asio::async_write(m_socket, asio::buffer(m_outMsg.front().message.body.data(), m_outMsg.front().message.header.size),
			[this](asio::error_code ec, size_t size)
			{
				if (ec)
				{
					Debug::Message("Error occured while trying to write body: ", ec.message());
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
	const uint32_t				m_ID = -1;

private:
	const bool					m_ownedByServer = true;

	asio::ip::tcp::socket		m_socket;
	asio::io_context&			m_context;

	tsque<Sent_message<T>>		m_outMsg;
	Sent_message<T>				m_handledMsg;

	std::function<void(uint32_t, Sent_message<T>&)> m_serverMsgHandler;

	std::function<void(Sent_message<T>&)>			m_clientMsgHandler;
};