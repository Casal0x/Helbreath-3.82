// ConcurrentMsgQueue.h: Thread-safe message queue for server
//
// Replaces the non-thread-safe CMsg* ring buffer (m_pMsgQuene[100000])
// with a mutex-protected deque. Supports push from I/O threads and
// pop from the game thread.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <mutex>
#include <deque>
#include <vector>
#include <cstdint>
#include <cstring>

namespace hb::shared::net {

struct QueuedMsg {
	char cFrom;
	std::vector<char> data;
	size_t dwSize;
	int iIndex;
	char cKey;

	QueuedMsg() : cFrom(0), dwSize(0), iIndex(0), cKey(0) {}
	QueuedMsg(char from, const char* pData, size_t size, int index, char key)
		: cFrom(from), dwSize(size), iIndex(index), cKey(key)
	{
		if (pData && size > 0) {
			data.assign(pData, pData + size);
		}
	}
};

class ConcurrentMsgQueue {
public:
	bool Push(char cFrom, const char* pData, size_t dwSize, int iIndex, char cKey)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_queue.size() >= MAX_QUEUE_SIZE) return false;
		m_queue.emplace_back(cFrom, pData, dwSize, iIndex, cKey);
		return true;
	}

	bool Pop(char* pFrom, char* pData, size_t* pSize, int* pIndex, char* pKey)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_queue.empty()) return false;

		auto& msg = m_queue.front();
		*pFrom = msg.cFrom;
		if (!msg.data.empty()) {
			std::memcpy(pData, msg.data.data(), msg.dwSize);
		}
		*pSize = msg.dwSize;
		*pIndex = msg.iIndex;
		*pKey = msg.cKey;

		m_queue.pop_front();
		return true;
	}

	size_t Size() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

private:
	mutable std::mutex m_mutex;
	std::deque<QueuedMsg> m_queue;
	static constexpr size_t MAX_QUEUE_SIZE = 100000;
};

// Thread-safe queue for accepted sockets
template<typename T>
class ConcurrentQueue {
public:
	void Push(T item)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_queue.push_back(std::move(item));
	}

	bool Pop(T& out)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_queue.empty()) return false;
		out = std::move(m_queue.front());
		m_queue.pop_front();
		return true;
	}

	bool Empty() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.empty();
	}

	size_t Size() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_queue.size();
	}

private:
	mutable std::mutex m_mutex;
	std::deque<T> m_queue;
};

// Error event from I/O threads
struct SocketErrorEvent {
	int iSocketIndex;
	int iErrorCode;
};

} // namespace hb::shared::net
