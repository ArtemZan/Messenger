#pragma once

//Message will have type, which defines how it will be handled
//T is enum with messages types
template <typename T>
class Message
{
	struct Header
	{
		T		type;
		size_t	size = 0;

		Header(T type)
			:type(type)
		{

		}
	};
public:
	static constexpr int SizeOfHeader() { return sizeof(Header); }

	Message(T type)
		:header(type)
	{
	}

	void Write(const void* source, size_t size)
	{
		body.resize(body.size() + size);
		memcpy(body.data() + header.size, source, size);
		header.size += size;
	}

	size_t Read(void* buffer, size_t size)
	{
		size_t size_to_read = (size >= header.size ? header.size : size);

		memcpy(buffer, body.data() + body.size() - size_to_read, size_to_read);

		return size_to_read;
	}

	template <typename T>
	size_t Read(T* buffer)
	{
		return Read(buffer, sizeof(T));
	}

	template <typename T>
	size_t Read(T& buffer)
	{
		return Read(&buffer, sizeof(T));
	}

	size_t TakeData(void* buffer, size_t size)
	{
		size = Read(buffer, size);

		body.resize(body.size() - size);

		header.size -= size;

		return size;
	}

	template <typename T>
	size_t TakeData(T* buffer)
	{
		return TakeData(buffer, sizeof(T));
	}

	template <typename T>
	size_t TakeData(T& buffer)
	{
		return TakeData(&buffer, sizeof(T));
	}

	Header header;
	std::vector<uint8_t> body;
};


template <typename T, typename DataT>
Message<T>& operator<<(Message<T>& message, const DataT& data)
{
	static_assert(std::is_standard_layout<DataT>(), "Not standart layout");

	message.body.resize(message.body.size() + sizeof(data));

	memcpy(message.body.data() + message.body.size() - sizeof(data), &data, sizeof(data));

	message.header.size += sizeof(data);

	return message;
}

template <typename T>
Message<T>& operator<<(Message<T>& message, const char* data)
{
	message.body.resize(message.body.size() + strlen(data) + 1);

	memcpy(message.body.data() + message.body.size() - strlen(data) - 1, data, strlen(data) + 1);

	message.header.size += strlen(data) + 1;

	return message;
}

template <typename T, typename DataT>
Message<T>& operator<<(Message<T>& message, const DataT&& data)
{
	static_assert(std::is_standard_layout<DataT>(), "Not standart layout");


	message.body.resize(message.body.size() + sizeof(data));

	memcpy(message.body.data() + message.body.size() - sizeof(data), &data, sizeof(data));

	message.header.size += sizeof(data);

	return message;
}

template <typename T, typename DataT>
Message<T>& operator>>(Message<T>& message, DataT& buffer)
{
	message.TakeData(buffer);

	return message;
}

template <typename T>
struct Sent_message 
{
	Sent_message(Message<T> message, uint32_t ID = 0)
		: message(message), ID(ID)
	{

	}
	uint32_t	ID;
	Message<T>	message;
};

