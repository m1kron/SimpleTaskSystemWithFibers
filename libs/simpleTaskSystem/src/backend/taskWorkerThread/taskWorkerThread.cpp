#include "precompiledHeader.h"
#include "taskWorkerThread.h"

NAMESPACE_STS_BEGIN
NAMESPACE_BACKEND_BEGIN

#define THREAD_LOG( ... ) LOG( "[WORKER_THREAD]: " __VA_ARGS__ );

///////////////////////////////////////////////////////////
void TaskWorkerThread::ThreadFunction()
{
	VERIFY_SUCCESS( m_workerInstance.ConvertToWorkerInstance() );
	THREAD_LOG( "Starting main loop." );

	MainWorkerThreadLoop();

	THREAD_LOG( "Main loop ended." );
	VERIFY_SUCCESS( m_workerInstance.ConvertToNormalThread() );
	VERIFY_SUCCESS( m_workerInstance.Deinitalize() );
}

///////////////////////////////////////////////////////////
void TaskWorkerThread::MainWorkerThreadLoop()
{
	while( true )
	{
		// Check if we have any new task to work on. If not, then wait for them.
		m_hasWorkToDoEvent.Wait();
		m_hasWorkToDoEvent.ResetEvent();

		THREAD_LOG( "Weaking up." );

		// Finish work if requested:
		if( m_shouldFinishWork )
		{
			THREAD_LOG( "Requested to finish work." );
			m_hasFinishWork = true;
			return;
		}

		// Do all the tasks:
		while( !m_shouldFinishWork )
		{
			bool hasMoreWork = m_workerInstance.PerformOneExecutionStep();
			if( !hasMoreWork )
				break;
		}

		THREAD_LOG( "Goes to sleep." );
	}
}

NAMESPACE_BACKEND_END
NAMESPACE_STS_END