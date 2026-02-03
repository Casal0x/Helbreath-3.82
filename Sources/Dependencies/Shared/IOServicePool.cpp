// IOServicePool.cpp: Shared io_context + optional thread pool
//
//////////////////////////////////////////////////////////////////////

#include "IOServicePool.h"
#include <cstdio>

IOServicePool::IOServicePool(int threadCount)
	: m_ioContext()
	, m_workGuard(asio::make_work_guard(m_ioContext))
	, m_threadCount(threadCount)
{
}

IOServicePool::~IOServicePool()
{
	Stop();
}

void IOServicePool::Start()
{
	if (m_threadCount <= 0) return;
	if (!m_threads.empty()) return; // Already started

	printf("[IOServicePool] Starting %d I/O threads\n", m_threadCount);

	for (int i = 0; i < m_threadCount; i++) {
		m_threads.emplace_back([this, i]() {
			printf("[IOServicePool] I/O thread %d started\n", i);
			m_ioContext.run();
			printf("[IOServicePool] I/O thread %d exited\n", i);
		});
	}
}

void IOServicePool::Stop()
{
	// Release work guard so run() can return when idle
	m_workGuard.reset();

	// Stop accepting new handlers
	m_ioContext.stop();

	// Join all threads
	for (auto& t : m_threads) {
		if (t.joinable()) {
			t.join();
		}
	}
	m_threads.clear();
}
