#include <gtest\gtest.h>
// HACKY TEST, CUZ queus won't be a part of public interface of sts!

#include "..\..\libs\simpleTaskSystem\src\common\common.h"
#include "..\..\libs\simpleTaskSystem\src\structures\lockbased\lockBasedPtrQueue.h"
#include "..\..\libs\simpleTaskSystem\src\structures\lockfree\lockfreePtrQueue.h"

#include "..\..\libs\basicThreadingLib\include\thread\functorThread.h"
#include "..\..\libs\basicThreadingLib\include\synchro\mutex.h"

namespace helpers
{
	static const int QUEUE_SIZE = 2048;
	static float SOME_VALUE = 12345;
	static float ONE_VALUE = 1;
	static float TWO_VALUE = 2;
	static float THREE_VALUE = 3;

	//////////////////////////////////////////////////////////////////////////////////
	template< class TQueue >
	void QueueAdder( TQueue& queue, unsigned elements_to_push )
	{
		for( unsigned i = 0; i < elements_to_push; ++i )
		{
			float* data = &SOME_VALUE;
			while( !queue.PushBack( data ) ) { btl::this_thread::YieldThread(); }
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	template< class TQueue >
	void QueuePoper( TQueue& queue, unsigned elements_to_pop )
	{
		for( unsigned i = 0; i < elements_to_pop; ++i )
		{
			float* ptr = nullptr;
			while( !( ptr = queue.PopFront() ) ) { btl::this_thread::YieldThread(); }

			ASSERT_TRUE( ptr != nullptr );
			ASSERT_TRUE( *ptr == SOME_VALUE );
		}
	}

	//////////////////////////////////////////////////////////////////////////////////
	template< class TQueue >
	void QueueBackPoper( TQueue& queue, unsigned elements_to_pop )
	{
		for( unsigned i = 0; i < elements_to_pop; ++i )
		{
			float* ptr = nullptr;
			while( !( ptr = queue.PopBack() ) ) { btl::this_thread::YieldThread(); }

			ASSERT_TRUE( ptr != nullptr );
			ASSERT_TRUE( *ptr == SOME_VALUE );
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////
	template< class TQueue >
	void TestSingleThreadedPopBack()
	{
		TQueue queue;
		for( int i = 0; i < 100; ++i )
		{
			ASSERT_TRUE( queue.PushBack( &ONE_VALUE ) );
			ASSERT_TRUE( queue.PushBack( &TWO_VALUE ) );
			ASSERT_TRUE( queue.PushBack( &THREE_VALUE ) );

			ASSERT_TRUE( *queue.PopBack() == THREE_VALUE );
			ASSERT_TRUE( *queue.PopBack() == TWO_VALUE );
			ASSERT_TRUE( *queue.PopBack() == ONE_VALUE );

			helpers::QueueAdder( queue, queue.GetMaxSize() );
			helpers::QueueBackPoper( queue, queue.GetMaxSize() );

			ASSERT_TRUE( queue.GetCurrentSize() == 0 );

			ASSERT_TRUE( queue.PopBack() == nullptr );
			ASSERT_TRUE( queue.PopBack() == nullptr );
			ASSERT_TRUE( queue.PopBack() == nullptr );
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////
	template< class TQueue >
	void TestSingleThreaded()
	{
		TQueue queue;

		for( int i = 0; i < 100; ++i )
		{
			helpers::QueueAdder( queue, queue.GetMaxSize() );
			helpers::QueuePoper( queue, queue.GetMaxSize() / 2 );
			ASSERT_TRUE( queue.GetCurrentSize() == queue.GetMaxSize() / 2 );

			helpers::QueuePoper( queue, queue.GetMaxSize() / 2 );
			ASSERT_TRUE( queue.GetCurrentSize() == 0 );
			ASSERT_TRUE( queue.PopFront() == nullptr );
			ASSERT_TRUE( queue.PopFront() == nullptr );
			ASSERT_TRUE( queue.PopFront() == nullptr );

			helpers::QueueAdder( queue, queue.GetMaxSize() );
			ASSERT_TRUE( queue.GetCurrentSize() == queue.GetMaxSize() );
			ASSERT_TRUE( queue.PushBack( &SOME_VALUE ) == false );
			ASSERT_TRUE( queue.PushBack( &SOME_VALUE ) == false );
			ASSERT_TRUE( queue.PushBack( &SOME_VALUE ) == false );

			helpers::QueuePoper( queue, queue.GetMaxSize() );
			ASSERT_TRUE( queue.GetCurrentSize() == 0 );
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////
	template< class TQueue >
	void TestSingleProducerAndConsumer()
	{
		TQueue queue;

		const unsigned elemnts_to_process = 100 * QUEUE_SIZE;
		for( int i = 0; i < 10; ++i )
		{
			btl::FunctorThread producer;
			producer.SetFunctorAndStartThread( [ &queue, elemnts_to_process ] { helpers::QueueAdder( queue, elemnts_to_process ); } );

			btl::FunctorThread consumer;
			consumer.SetFunctorAndStartThread( [ &queue, elemnts_to_process ] { helpers::QueuePoper( queue, elemnts_to_process ); } );

			producer.Join();
			consumer.Join();

			ASSERT_TRUE( queue.GetCurrentSize() == 0 );
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////
	template< class TQueue >
	void TestSingleProducerAndConsumer_PopBack()
	{
		TQueue queue;

		const unsigned elemnts_to_process = 100 * QUEUE_SIZE;
		for( int i = 0; i < 10; ++i )
		{
			btl::FunctorThread producer;
			producer.SetFunctorAndStartThread( [ &queue, elemnts_to_process ] { helpers::QueueAdder( queue, elemnts_to_process ); } );

			btl::this_thread::SleepFor( 10 );

			btl::FunctorThread consumer;
			consumer.SetFunctorAndStartThread( [ &queue, elemnts_to_process ] { helpers::QueueBackPoper( queue, elemnts_to_process ); } );

			producer.Join();
			consumer.Join();

			ASSERT_TRUE( queue.GetCurrentSize() == 0 );
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////
	template< class TQueue >
	void TestMultipleProducersAndConsumers()
	{
		TQueue thread_queue;

		static const unsigned NUM_OF_ADDING_THREADS = 4;

		unsigned num_to_pop = NUM_OF_ADDING_THREADS * QUEUE_SIZE + QUEUE_SIZE;
		unsigned max_size = ( num_to_pop ) / NUM_OF_ADDING_THREADS;

		for( int i = 0; i < 50; ++i )
		{
			btl::FunctorThread thread1;
			thread1.SetFunctorAndStartThread( [ &thread_queue, max_size ] { helpers::QueueAdder( thread_queue, max_size ); } );
			btl::FunctorThread thread2;
			thread2.SetFunctorAndStartThread( [ &thread_queue, max_size ] { helpers::QueueAdder( thread_queue, max_size ); } );
			btl::FunctorThread thread3;
			thread3.SetFunctorAndStartThread( [ &thread_queue, max_size ] { helpers::QueueAdder( thread_queue, max_size ); } );

			unsigned num_to_pop_per_thread = ( num_to_pop - thread_queue.GetMaxSize() ) / 2;
			btl::FunctorThread thread4;
			thread4.SetFunctorAndStartThread( [ &thread_queue, num_to_pop_per_thread ] { helpers::QueuePoper( thread_queue, num_to_pop_per_thread ); } );
			btl::FunctorThread thread5;
			thread5.SetFunctorAndStartThread( [ &thread_queue, num_to_pop_per_thread ] { helpers::QueuePoper( thread_queue, num_to_pop_per_thread ); } );

			helpers::QueueAdder( thread_queue, max_size + ( num_to_pop - NUM_OF_ADDING_THREADS * max_size ) );

			thread1.Join();
			thread2.Join();
			thread3.Join();
			thread4.Join();
			thread5.Join();

			unsigned size2 = thread_queue.GetCurrentSize();
			ASSERT_TRUE( size2 == thread_queue.GetMaxSize() );

			helpers::QueuePoper( thread_queue, thread_queue.GetMaxSize() );
			ASSERT_TRUE( thread_queue.GetCurrentSize() == 0 );
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleThreaded_PopBack_Lockbased_SpinLock )
{
	helpers::TestSingleThreadedPopBack< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::SpinLock > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleThreaded_PopBack_Lockbased_Mutex )
{
	helpers::TestSingleThreadedPopBack< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::Mutex > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleThreaded_Lockfree )
{
	helpers::TestSingleThreaded< sts::LockFreePtrQueue< float, helpers::QUEUE_SIZE >  >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleThreaded_Lockbased_SpinLock )
{
	helpers::TestSingleThreaded< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::SpinLock > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleThreaded_Lockbased_Mutex )
{
	helpers::TestSingleThreaded< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::Mutex > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleProducerAndConsumer_Lockfree )
{
	helpers::TestSingleProducerAndConsumer< sts::LockFreePtrQueue< float, helpers::QUEUE_SIZE >  >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleProducerAndConsumer_Lockbased_SpinLock )
{
	helpers::TestSingleProducerAndConsumer< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::SpinLock > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleProducerAndConsumer_Lockbased_Mutex )
{
	helpers::TestSingleProducerAndConsumer< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::Mutex > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleProducerAndConsumer_PopBack_Lockbased_SpinLock )
{
	helpers::TestSingleProducerAndConsumer_PopBack< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::SpinLock > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, SingleProducerAndConsumer_PopBack_Lockbased_Mutex )
{
	helpers::TestSingleProducerAndConsumer_PopBack< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::Mutex > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, MultipleProducersAndConsumers_Lockfree )
{
	helpers::TestMultipleProducersAndConsumers< sts::LockFreePtrQueue< float, helpers::QUEUE_SIZE > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, MultipleProducersAndConsumers_LockBased_SpinLock )
{
	helpers::TestMultipleProducersAndConsumers< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::SpinLock > >();
}

////////////////////////////////////////////////////////////////////////////////
TEST( QueueTests, MultipleProducersAndConsumers_LockBased_Mutex )
{
	helpers::TestMultipleProducersAndConsumers< sts::LockBasedPtrQueue< float, helpers::QUEUE_SIZE, btl::Mutex > >();
}