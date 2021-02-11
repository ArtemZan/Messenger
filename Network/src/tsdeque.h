#pragma once

//Class of thread-safe queue
//One instance of tsque can be accessed by several threads simutaneously
//This is achieved using std::mutex and std::scoped_lock
template <typename T>
class tsque
{
public:
	tsque() = default;
	tsque(const tsque<T>&) = delete;
	~tsque()
	{
		clear();
	}

	inline const T& front() const
	{
		//std::scoped_lock locks the queue until the function
		//returns (same for other functions)
		std::scoped_lock lock(m_mutex);
		return m_deque.front();
	}

	inline const T& back() const
	{
		std::scoped_lock lock(m_mutex);
		return m_deque.back();
	}

	inline void push_back(const T& value)
	{
		std::scoped_lock lock(m_mutex);
		m_deque.push_back(value);
	}

	inline void push_front(const T& value)
	{
		std::scoped_lock lock(m_mutex);
		m_deque.push_front(value);
	}

	template <class... Valty>
	inline void emplace_front(const Valty... val)
	{
		std::scoped_lock lock(m_mutex);
		m_deque.emplace_front(val...);
	}

	template <class... Valty>
	inline void emplace_back(const Valty... val)
	{
		std::scoped_lock lock(m_mutex);
		m_deque.emplace_back(val...);
	}

	inline void clear()
	{
		std::scoped_lock lock(m_mutex);
		m_deque.clear();
	}

	inline size_t size() const
	{
		std::scoped_lock lock(m_mutex);
		return m_deque.size();
	}

	inline T pop_front()
	{
		std::scoped_lock lock(m_mutex);
		T front = m_deque.front();
		m_deque.pop_front();
		return front;
	}
	
	inline T pop_back()
	{
		std::scoped_lock lock(m_mutex);
		T back = m_deque.back();
		m_deque.pop_back();
		return back;
	}
	
	inline bool empty() {
		std::scoped_lock lock(m_mutex);
		return m_deque.empty(); 
	}

private:
	mutable std::mutex		m_mutex;

	//using std::deque makes accessing data from both sides easy
	std::deque<T>			m_deque;
};