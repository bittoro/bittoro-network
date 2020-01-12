#include <util/thread/queue_manager.hpp>

#include <absl/types/optional.h>
#include <vector>
#include <gtest/gtest.h>

using namespace llarp::thread;

void
generation(QueueManager& manager, uint32_t pushIndex, uint32_t popIndex)
{
  ASSERT_GE(pushIndex, popIndex);
  ASSERT_LE(pushIndex - popIndex, manager.capacity());

  for(uint32_t i = 0; i < popIndex; ++i)
  {
    uint32_t gen   = 0;
    uint32_t index = 0;

    (void)manager.reservePushIndex(gen, index);
    manager.commitPushIndex(gen, index);

    auto result = manager.reservePopIndex(gen, index);

    ASSERT_EQ(result, QueueReturn::Success);
    ASSERT_EQ(index, i % manager.capacity());

    manager.commitPopIndex(gen, index);
  }

  for(uint32_t i = popIndex; i < pushIndex; ++i)
  {
    uint32_t gen   = 0;
    uint32_t index = 0;

    auto result = manager.reservePushIndex(gen, index);
    ASSERT_EQ(result, QueueReturn::Success);
    ASSERT_EQ(index, i % manager.capacity());

    manager.commitPushIndex(gen, index);
  }
}

class IntQueue
{
 private:
  QueueManager manager;

  std::vector< int > data;

 public:
  IntQueue(const IntQueue&) = delete;

  explicit IntQueue(size_t capacity) : manager(capacity), data(capacity, 0)
  {
  }

  ~IntQueue() = default;

  bool
  tryPushBack(int value)
  {
    uint32_t gen   = 0;
    uint32_t index = 0;

    if(manager.reservePushIndex(gen, index) == QueueReturn::Success)
    {
      data[index] = value;
      manager.commitPushIndex(gen, index);
      return true;
    }
    else
    {
      return false;
    }
  }

  absl::optional< int >
  tryPopFront()
  {
    uint32_t gen   = 0;
    uint32_t index = 0;

    if(manager.reservePopIndex(gen, index) == QueueReturn::Success)
    {
      int result = data[index];
      manager.commitPopIndex(gen, index);
      return result;
    }
    else
    {
      return {};
    }
  }

  size_t
  size() const
  {
    return manager.size();
  }

  size_t
  capacity() const
  {
    return manager.capacity();
  }
};

// This class exactly mirrors the data of the QueueManager, and is used for
// both debugging and whitebox testing.
struct QueueData
{
 public:
  QueueManager::AtomicIndex m_pushIndex;  // Index in the buffer that the next
                                          // element will be added to.

  char m_pushPadding[QueueManager::Alignment
                     - sizeof(QueueManager::AtomicIndex)];

  QueueManager::AtomicIndex m_popIndex;  // Index in the buffer that the next
                                         // element will be removed from.

  char
      m_popPadding[QueueManager::Alignment - sizeof(QueueManager::AtomicIndex)];

  const size_t m_capacity;  // max size of the manager.

  const uint32_t m_maxGeneration;  // Maximum generation for this object.

  const uint32_t m_maxCombinedIndex;  // Maximum combined value of index and
                                      // generation for this object.

  std::uint32_t* m_states;  // Array of index states.
};

static_assert(sizeof(QueueData) == sizeof(QueueManager),
              "QueueData not updated");

static constexpr uint32_t GENERATION_COUNT_SHIFT = 0x2;
static constexpr uint32_t ELEMENT_STATE_MASK     = 0x3;

struct QueueIntrospection
{
 private:
  const QueueData* data;

 public:
  QueueIntrospection(const QueueManager& manager)
      : data(reinterpret_cast< const QueueData* >(&manager))
  {
  }

  uint32_t
  pushIndex() const
  {
    return data->m_pushIndex % capacity();
  }

  uint32_t
  pushGeneration() const
  {
    return data->m_pushIndex / capacity();
  }

  uint32_t
  popIndex() const
  {
    return data->m_popIndex % capacity();
  }

  uint32_t
  popGeneration() const
  {
    return data->m_popIndex / capacity();
  }

  uint32_t
  elementGen(uint32_t index) const
  {
    return data->m_states[index] >> GENERATION_COUNT_SHIFT;
  }

  ElementState
  elementState(uint32_t index) const
  {
    return static_cast< ElementState >(data->m_states[index]
                                       & ELEMENT_STATE_MASK);
  }

  uint32_t
  maxGen() const
  {
    return data->m_maxGeneration;
  }

  uint32_t
  maxCombinedIndex() const
  {
    return data->m_maxCombinedIndex;
  }

  uint32_t
  capacity() const
  {
    return data->m_capacity;
  }
};

void
adjustGeneration(QueueManager& manager, uint32_t gen)
{
  QueueData* data = reinterpret_cast< QueueData* >(&manager);

  auto capacity = manager.capacity();

  for(size_t i = 0; i < capacity; ++i)
  {
    data->m_states[i] = gen << GENERATION_COUNT_SHIFT;
  }

  *reinterpret_cast< QueueManager::AtomicIndex* >(&data->m_pushIndex) =
      (gen * capacity);
  *reinterpret_cast< QueueManager::AtomicIndex* >(&data->m_popIndex) =
      (gen * capacity);
}

void
dirtyGenerate(QueueManager& manager, uint32_t pushCombinedIndex,
              uint32_t popCombinedIndex)
{
  ASSERT_GE(pushCombinedIndex, popCombinedIndex);
  ASSERT_LE(pushCombinedIndex - popCombinedIndex, manager.capacity());

  uint32_t capacity = manager.capacity();

  uint32_t start =
      static_cast< uint32_t >(popCombinedIndex / manager.capacity());

  adjustGeneration(manager, start);
  generation(manager, pushCombinedIndex - (start * capacity),
             popCombinedIndex - (start * capacity));
}

TEST(TestQueueManager, SimpleUsage)
{
  IntQueue queue(2);

  bool rc = queue.tryPushBack(1);
  ASSERT_TRUE(rc);

  rc = queue.tryPushBack(2);
  ASSERT_TRUE(rc);

  rc = queue.tryPushBack(3);
  ASSERT_FALSE(rc);

  ASSERT_EQ(2u, queue.size());

  auto result = queue.tryPopFront();

  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(1, result.value());
}

class BasicFunctionality : public ::testing::TestWithParam< uint32_t >
{
};

TEST_P(BasicFunctionality, Push)
{
  uint32_t val = GetParam();

  QueueManager manager(val);

  ASSERT_EQ(0u, manager.size());

  uint32_t gen   = 0;
  uint32_t index = 0;

  for(uint32_t i = 0; i < val; ++i)
  {
    ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
    ASSERT_EQ(i, index);
    ASSERT_EQ(0u, gen);
    ASSERT_EQ(i, manager.size() - 1);
    manager.commitPushIndex(gen, index);
  }

  ASSERT_EQ(QueueReturn::QueueFull, manager.reservePushIndex(gen, index));
  ASSERT_EQ(val, manager.size());
}

TEST_P(BasicFunctionality, AcquiringPopIndex)
{
  uint32_t capacity = GetParam();

  QueueManager manager(capacity);

  ASSERT_EQ(0u, manager.size());

  uint32_t gen   = 0;
  uint32_t index = 0;

  for(uint32_t g = 0; g < 3; ++g)
  {
    for(uint32_t idx = 0; idx < capacity; ++idx)
    {
      ASSERT_EQ(QueueReturn::QueueEmpty, manager.reservePopIndex(gen, index));

      ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
      ASSERT_EQ(g, gen);
      ASSERT_EQ(index, idx);
      ASSERT_EQ(1u, manager.size());

      manager.commitPushIndex(gen, index);
      ASSERT_EQ(1u, manager.size());

      ASSERT_EQ(QueueReturn::Success, manager.reservePopIndex(gen, index));
      ASSERT_EQ(g, gen);
      ASSERT_EQ(index, idx);
      ASSERT_EQ(0u, manager.size());

      manager.commitPopIndex(gen, index);
      ASSERT_EQ(0u, manager.size());
    }
  }
}

TEST_P(BasicFunctionality, pushIndex)
{
  uint32_t capacity = GetParam();

  QueueManager manager(capacity);

  ASSERT_EQ(0u, manager.size());

  uint32_t generation = 0;
  uint32_t index      = 0;

  // Fill the queue
  for(uint32_t idx = 0; idx < capacity; ++idx)
  {
    manager.reservePushIndex(generation, index);
    manager.commitPushIndex(generation, index);
  }

  ASSERT_EQ(capacity, manager.size());

  for(uint32_t gen = 0; gen < 3; ++gen)
  {
    for(uint32_t idx = 0; idx < capacity; ++idx)
    {
      ASSERT_EQ(QueueReturn::QueueFull,
                manager.reservePushIndex(generation, index));

      ASSERT_EQ(QueueReturn::Success,
                manager.reservePopIndex(generation, index));

      ASSERT_EQ(generation, gen);
      ASSERT_EQ(index, idx);
      ASSERT_EQ(capacity - 1, manager.size());

      manager.commitPopIndex(generation, index);
      ASSERT_EQ(capacity - 1, manager.size());

      ASSERT_EQ(QueueReturn::Success,
                manager.reservePushIndex(generation, index));

      ASSERT_EQ(generation, gen + 1);
      ASSERT_EQ(index, idx);
      ASSERT_EQ(manager.size(), capacity);

      manager.commitPushIndex(generation, index);
      ASSERT_EQ(manager.size(), capacity);
    }
  }
}

INSTANTIATE_TEST_CASE_P(TestQueueManagerBasic, BasicFunctionality,
                        ::testing::Range(1u, 100u), );

// Potential issues:
// - That pushing an element at the max combined index will push the next
// element at index 0
// - That popping an element at the max combined index will pop the next
// element at index 0
// - That size returns the correct size when the push index has gone past the
// max combined index
// - That reservePopIndexForClear and abortPushIndexReservation clear the
// correct element and increment push/pop

TEST(TestQueueManagerMaxCombinedIndex, PushAtMax)
{
  QueueManager manager(1);

  QueueIntrospection state{manager};

  const uint32_t MAX_COMBINED_INDEX =
      std::numeric_limits< uint32_t >::max() >> 2;
  const uint32_t MAX_GENERATION = std::numeric_limits< uint32_t >::max() >> 2;

  const uint32_t maxGeneration = QueueIntrospection(manager).maxGen();
  const uint32_t maxCombinedIndex =
      QueueIntrospection(manager).maxCombinedIndex();

  ASSERT_EQ(maxGeneration, MAX_GENERATION);
  ASSERT_EQ(maxCombinedIndex, MAX_COMBINED_INDEX);

  dirtyGenerate(manager, MAX_COMBINED_INDEX, MAX_COMBINED_INDEX);

  uint32_t gen   = 0;
  uint32_t index = 0;

  ASSERT_EQ(0u, manager.size());
  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
  ASSERT_EQ(MAX_GENERATION, gen);
  ASSERT_EQ(0u, index);
  manager.commitPushIndex(gen, index);

  ASSERT_EQ(QueueReturn::Success, manager.reservePopIndex(gen, index));
  ASSERT_EQ(MAX_GENERATION, gen);
  ASSERT_EQ(0u, index);
  manager.commitPopIndex(gen, index);
  ASSERT_EQ(0u, manager.size());

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
  ASSERT_EQ(0u, gen);
  ASSERT_EQ(0u, index);
  manager.commitPushIndex(gen, index);
  ASSERT_EQ(1u, manager.size());

  ASSERT_EQ(QueueReturn::Success, manager.reservePopIndex(gen, index));
  ASSERT_EQ(0u, gen);
  ASSERT_EQ(0u, index);
  manager.commitPopIndex(gen, index);
  ASSERT_EQ(0u, manager.size());
}

struct CombinedIndexData
{
  uint32_t capacity;
  uint32_t pushIndex;
  uint32_t popIndex;
};

std::ostream&
operator<<(std::ostream& os, CombinedIndexData d)
{
  os << "[ capacity = " << d.capacity << " pushIndex = " << d.pushIndex
     << " popIndex = " << d.popIndex << " ]";
  return os;
}

class PopAtMax : public ::testing::TestWithParam< CombinedIndexData >
{
};

TEST_P(PopAtMax, PopAtMax)
{
  const auto& d = GetParam();

  QueueManager manager(d.capacity);

  const uint32_t NUM_GEN = QueueManager::numGenerations(d.capacity);
  const uint32_t MAX_GEN = NUM_GEN - 1;

  adjustGeneration(manager, MAX_GEN);

  uint32_t gen   = 0;
  uint32_t index = 0;

  // Push and pop elements up until the pop-index.

  for(size_t j = 0; j < d.popIndex; ++j)
  {
    uint32_t INDEX = j % d.capacity;
    uint32_t GEN   = (MAX_GEN + j / d.capacity) % NUM_GEN;

    ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
    ASSERT_EQ(INDEX, index);
    ASSERT_EQ(GEN, gen);
    manager.commitPushIndex(gen, index);
    ASSERT_EQ(1u, manager.size());

    ASSERT_EQ(QueueReturn::Success, manager.reservePopIndex(gen, index));

    ASSERT_EQ(INDEX, index);
    ASSERT_EQ(GEN, gen);
    manager.commitPopIndex(gen, index);
    ASSERT_EQ(0u, manager.size());
  }

  // Push elements up to the push index

  for(size_t j = d.popIndex; j < d.pushIndex; ++j)
  {
    uint32_t INDEX = j % d.capacity;
    uint32_t GEN   = (MAX_GEN + j / d.capacity) % NUM_GEN;

    ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));

    ASSERT_EQ(INDEX, index);
    ASSERT_EQ(GEN, gen);
    manager.commitPushIndex(gen, index);
    ASSERT_EQ(j - d.popIndex + 1, manager.size());
  }

  // Pop elements until the queue is empty.

  for(size_t j = d.popIndex; j < d.pushIndex; ++j)
  {
    uint32_t INDEX = j % d.capacity;
    uint32_t GEN   = (MAX_GEN + j / d.capacity) % NUM_GEN;

    ASSERT_EQ(QueueReturn::Success, manager.reservePopIndex(gen, index));

    ASSERT_EQ(INDEX, index);
    ASSERT_EQ(GEN, gen);
    manager.commitPopIndex(gen, index);
    ASSERT_EQ(d.pushIndex - j - 1, manager.size());
  }
}

CombinedIndexData PopAtMaxData[] =
    {  // Capacity 2 queues for a couple generations
        {2, 1, 0},
        {2, 2, 0},
        {2, 2, 1},
        {2, 2, 2},
        {2, 3, 1},
        {2, 3, 2},
        {2, 3, 3},
        {2, 4, 2},
        {2, 4, 3},
        {2, 4, 4},

        // Capacity 3 queues for a couple generations
        {3, 2, 0},
        {3, 3, 0},
        {3, 3, 1},
        {3, 3, 2},
        {3, 3, 3},
        {3, 4, 1},
        {3, 4, 2},
        {3, 4, 3},
        {3, 4, 4},
        {3, 5, 2},
        {3, 5, 3},
        {3, 5, 4},
        {3, 5, 5},

        // Capacity 7 queue
        {7, 6, 0},
        {7, 7, 0},
        {7, 7, 6},
        {7, 13, 7},
        {7, 14, 7}};

INSTANTIATE_TEST_CASE_P(TestQueueManagerMaxCombinedIndex, PopAtMax,
                        ::testing::ValuesIn(PopAtMaxData), );

class ReservePop : public ::testing::TestWithParam< CombinedIndexData >
{
};

TEST_P(ReservePop, ReservePopIndexForClear)
{
  const auto& d = GetParam();

  QueueManager manager(d.capacity);
  const uint32_t NUM_GEN = QueueManager::numGenerations(d.capacity);
  const uint32_t MAX_GEN = NUM_GEN - 1;

  adjustGeneration(manager, MAX_GEN);

  generation(manager, d.pushIndex, d.popIndex);

  // Pop elements until the queue is empty

  uint32_t endGeneration = 0;
  uint32_t endIndex      = 0;
  uint32_t gen           = 0;
  uint32_t index         = 0;

  ASSERT_EQ(QueueReturn::Success,
            manager.reservePushIndex(endGeneration, endIndex));

  for(uint32_t j = d.popIndex; j < d.pushIndex; ++j)
  {
    uint32_t INDEX = j % d.capacity;
    uint32_t GEN   = (MAX_GEN + j / d.capacity) % NUM_GEN;

    ASSERT_TRUE(
        manager.reservePopForClear(gen, index, endGeneration, endIndex));

    ASSERT_EQ(INDEX, index);
    ASSERT_EQ(GEN, gen);
    manager.commitPopIndex(gen, index);
  }

  ASSERT_FALSE(manager.reservePopForClear(gen, index, endGeneration, endIndex));
  manager.abortPushIndexReservation(endGeneration, endIndex);
  ASSERT_EQ(0u, manager.size());
}

CombinedIndexData ReservePopIndexForClearData[] = {
    // Capacity 2 queues for a couple generations
    {2, 1, 0},
    {2, 2, 1},
    {2, 2, 2},
    {2, 3, 2},
    {2, 3, 3},
    {2, 4, 3},
    {2, 4, 4},

    // Capacity 3 queues for a couple generations
    {3, 2, 0},
    {3, 3, 1},
    {3, 3, 2},
    {3, 3, 3},
    {3, 4, 2},
    {3, 4, 3},
    {3, 4, 4},
    {3, 5, 3},
    {3, 5, 4},
    {3, 5, 5},

    // Capacity 7 queue
    {7, 6, 0},
    {7, 7, 6},
    {7, 13, 7},
};

INSTANTIATE_TEST_CASE_P(TestQueueManagerMaxCombinedIndex, ReservePop,
                        ::testing::ValuesIn(ReservePopIndexForClearData), );

struct CircularDifferenceData
{
  uint32_t minuend;
  uint32_t subtrahend;
  uint32_t maxSize;
  int32_t expectedValue;
};

std::ostream&
operator<<(std::ostream& os, CircularDifferenceData d)
{
  os << "[ minuend = " << d.minuend << " subtrahend = " << d.subtrahend
     << " maxSize = " << d.maxSize << " expectedValue = " << d.expectedValue
     << " ]";
  return os;
}

class CircularDifference
    : public ::testing::TestWithParam< CircularDifferenceData >
{
};

TEST_P(CircularDifference, difference)
{
  const auto& data = GetParam();

  ASSERT_EQ(data.expectedValue,
            QueueManager::circularDifference(data.minuend, data.subtrahend,
                                             data.maxSize));
}

constexpr uint32_t OUR_INT32_MAX    = std::numeric_limits< int32_t >::max();
constexpr uint32_t OUR_INT32_MAX_1  = OUR_INT32_MAX + 1;
constexpr int32_t OUR_INT32_MAX_DIV = OUR_INT32_MAX_1 / 2;

CircularDifferenceData circularDifferenceData[] = {
    // capacity 1
    {0, 0, 1, 0},

    // capacity 2
    {1, 1, 2, 0},
    {1, 0, 2, 1},
    {0, 1, 2, -1},

    // capacity 3
    {2, 0, 3, -1},
    {2, 1, 3, 1},
    {2, 2, 3, 0},
    {1, 0, 3, 1},
    {1, 1, 3, 0},
    {1, 2, 3, -1},
    {0, 0, 3, 0},
    {0, 1, 3, -1},
    {0, 2, 3, 1},

    // capacity 4
    {3, 0, 4, -1},
    {3, 1, 4, 2},
    {3, 2, 4, 1},
    {3, 3, 4, 0},
    {0, 3, 4, 1},
    {1, 3, 4, -2},
    {2, 3, 4, -1},
    {3, 3, 4, 0},

    // capacity INT_MAX
    {OUR_INT32_MAX, 0, OUR_INT32_MAX_1, -1},
    {0, OUR_INT32_MAX, OUR_INT32_MAX_1, 1},
    {OUR_INT32_MAX_DIV, 0, OUR_INT32_MAX_1, OUR_INT32_MAX_DIV},
    {0, OUR_INT32_MAX_DIV, OUR_INT32_MAX_1, -OUR_INT32_MAX_DIV},

    // Examples circularDifference( 0, 359, 360) == 1
    // circularDifference( 359, 0, 360) == -1 circularDifference(
    // 180, 0, 360) == 180 circularDifference( 0, 180, 360) == -180

    {0, 359, 360, 1},
    {359, 0, 360, -1},
    {180, 0, 360, 180},
    {0, 180, 360, -180},
};

INSTANTIATE_TEST_CASE_P(TestQueueManagerMaxCombinedIndex, CircularDifference,
                        ::testing::ValuesIn(circularDifferenceData), );

class NumGenerations : public ::testing::TestWithParam< uint32_t >
{
};

TEST_P(NumGenerations, generations)
{
  uint32_t capacity = GetParam();
  uint32_t numGen   = QueueManager::numGenerations(capacity);

  static const uint32_t MAX_ELEMENT_STATE_GEN =
      std::numeric_limits< uint32_t >::max() >> 2;

  static const uint32_t MAX_COMBINED_INDEX =
      std::numeric_limits< uint32_t >::max() >> 1;

  ASSERT_GE(numGen, 2u);
  ASSERT_TRUE(MAX_ELEMENT_STATE_GEN == numGen - 1
              || ((numGen * capacity - 1 <= MAX_COMBINED_INDEX)
                  && ((numGen + 1) * capacity - 1 > MAX_COMBINED_INDEX)));
}

uint32_t GenerationData[] = {1,
                             2,
                             3,
                             4,
                             15,
                             16,
                             17,
                             QueueManager::MAX_CAPACITY - 1,
                             QueueManager::MAX_CAPACITY};

INSTANTIATE_TEST_CASE_P(TestQueueManagerMaxCombinedIndex, NumGenerations,
                        ::testing::ValuesIn(GenerationData), );

TEST(TestQueueManager, abortPushIndexReservation)
{
  uint32_t genA   = 0;
  uint32_t genB   = 0;
  uint32_t indexA = 0;
  uint32_t indexB = 0;

  QueueManager manager(1);

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(genA, indexA));
  ASSERT_NE(QueueReturn::Success, manager.reservePushIndex(genA, indexA));

  manager.abortPushIndexReservation(genA, indexA);

  ASSERT_EQ(0u, manager.size());

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(genB, indexB));
  ASSERT_EQ(genA + 1, genB);
  ASSERT_EQ(indexA, indexB);
}

struct AbortData
{
  uint32_t capacity;
  uint32_t pushIndex;
  uint32_t popIndex;
  uint32_t expectedClears;
};

std::ostream&
operator<<(std::ostream& os, AbortData d)
{
  os << "[ capacity = " << d.capacity << " pushIndex = " << d.pushIndex
     << " popIndex = " << d.popIndex << " expectedClears = " << d.expectedClears
     << " ]";
  return os;
}

class AbortPush : public ::testing::TestWithParam< AbortData >
{
};

TEST_P(AbortPush, abortPush)
{
  const auto& data = GetParam();

  QueueManager manager(data.capacity);

  generation(manager, data.pushIndex, data.popIndex);

  const uint32_t END_GENERATION = data.pushIndex / data.capacity;
  const uint32_t END_INDEX      = data.pushIndex % data.capacity;

  uint32_t gen   = 0;
  uint32_t index = 0;

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
  ASSERT_EQ(END_GENERATION, gen);
  ASSERT_EQ(END_INDEX, index);

  for(uint32_t i = 0; i < data.expectedClears; ++i)
  {
    ASSERT_TRUE(
        manager.reservePopForClear(gen, index, END_GENERATION, END_INDEX));

    ASSERT_EQ((data.popIndex + i) / data.capacity, gen);
    ASSERT_EQ((data.popIndex + i) % data.capacity, index);

    manager.commitPopIndex(gen, index);
  }

  ASSERT_FALSE(
      manager.reservePopForClear(gen, index, END_GENERATION, END_INDEX));

  manager.abortPushIndexReservation(END_GENERATION, END_INDEX);

  // Verify the queue is now empty, and the current push index has changed

  ASSERT_EQ(0u, manager.size());
  for(uint32_t i = 0; i < data.capacity; ++i)
  {
    ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
    ASSERT_EQ(i + 1, manager.size());

    ASSERT_EQ(END_GENERATION * data.capacity + END_INDEX + i + 1,
              gen * data.capacity + index);
  }

  ASSERT_NE(QueueReturn::Success, manager.reservePushIndex(gen, index));
  ASSERT_EQ(data.capacity, manager.size());
}

AbortData abortData[] = {
    {1, 0, 0, 0},

    // Capacity 2 queues for a couple generations
    {2, 0, 0, 0},
    {2, 1, 0, 1},
    {2, 1, 1, 0},
    {2, 2, 1, 1},
    {2, 2, 2, 0},
    {2, 3, 2, 1},
    {2, 3, 3, 0},

    // Capacity 3 queues for a couple generations
    {3, 0, 0, 0},
    {3, 1, 0, 1},
    {3, 1, 1, 0},
    {3, 2, 0, 2},
    {3, 2, 1, 1},
    {3, 2, 2, 0},
    {3, 3, 1, 2},
    {3, 3, 2, 1},
    {3, 3, 3, 0},
    {3, 4, 2, 2},
    {3, 4, 3, 1},
    {3, 4, 4, 0},

    // Capacity 7 queue
    {7, 14, 14, 0},
    {7, 15, 14, 1},
    {7, 20, 14, 6},
    {7, 18, 18, 0},
    {7, 19, 18, 1},
    {7, 24, 18, 6},
};

INSTANTIATE_TEST_CASE_P(TestQueueManagerMaxCombinedIndex, AbortPush,
                        ::testing::ValuesIn(abortData), );

// Testing reservePopForClear
// - Failure is returned when the head of the queue is the same as the given end
// generation and index
// - Success is returned and clears the queue head when the current pop index is
// not the given end generation and index
// - We do not clear an index reserved for popping

TEST(TestQueueManagerReserve, Capacity1)
{
  // It is not possible to clear a pop index when the capacity is 1.

  uint32_t gen   = 0;
  uint32_t index = 0;

  // Random values to verify we didn't change them.
  uint32_t resultGen   = 1024;
  uint32_t resultIndex = 1023;

  QueueManager manager(1);

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));

  ASSERT_FALSE(manager.reservePopForClear(resultGen, resultIndex, gen, index));

  ASSERT_EQ(1024u, resultGen);
  ASSERT_EQ(1023u, resultIndex);

  ASSERT_EQ(1u, manager.size());
}

TEST(TestQueueManagerReserve, Capacity2)
{
  uint32_t gen   = 0;
  uint32_t index = 0;

  // Random values to verify we didn't change them.
  uint32_t resultGen   = 1024;
  uint32_t resultIndex = 1023;

  QueueManager manager(2);

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));

  ASSERT_FALSE(manager.reservePopForClear(resultGen, resultIndex, gen, index));

  ASSERT_EQ(1024u, resultGen);
  ASSERT_EQ(1023u, resultIndex);
  manager.commitPushIndex(gen, index);

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));

  ASSERT_TRUE(manager.reservePopForClear(resultGen, resultIndex, gen, index));
  ASSERT_EQ(0u, resultGen);
  ASSERT_EQ(0u, resultIndex);
  manager.commitPopIndex(resultGen, resultIndex);

  manager.commitPushIndex(gen, index);
  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));

  ASSERT_TRUE(manager.reservePopForClear(resultGen, resultIndex, gen, index));
  ASSERT_EQ(0u, resultGen);
  ASSERT_EQ(1u, resultIndex);
  manager.commitPopIndex(resultGen, resultIndex);
  manager.commitPushIndex(gen, index);

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));

  ASSERT_TRUE(manager.reservePopForClear(resultGen, resultIndex, gen, index));
  ASSERT_EQ(1u, resultGen);
  ASSERT_EQ(0u, resultIndex);
  manager.commitPopIndex(resultGen, resultIndex);
  manager.commitPushIndex(gen, index);

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));

  ASSERT_TRUE(manager.reservePopForClear(resultGen, resultIndex, gen, index));
  ASSERT_EQ(1u, resultGen);
  ASSERT_EQ(1u, resultIndex);
  manager.commitPopIndex(resultGen, resultIndex);
  manager.commitPushIndex(gen, index);

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));

  ASSERT_TRUE(manager.reservePopForClear(resultGen, resultIndex, gen, index));
  ASSERT_EQ(2u, resultGen);
  ASSERT_EQ(0u, resultIndex);
  manager.commitPopIndex(resultGen, resultIndex);
  manager.commitPushIndex(gen, index);
}

struct ReserveData
{
  uint32_t capacity;
  uint32_t pushIndex;
  uint32_t popIndex;
  uint32_t expectedClears;
};

std::ostream&
operator<<(std::ostream& os, ReserveData d)
{
  os << "[ capacity = " << d.capacity << " pushIndex = " << d.pushIndex
     << " popIndex = " << d.popIndex << " expectedClears = " << d.expectedClears
     << " ]";
  return os;
}
class Reserve : public ::testing::TestWithParam< ReserveData >
{
};

TEST_P(Reserve, clear)
{
  const auto& data = GetParam();
  QueueManager manager(data.capacity);

  generation(manager, data.pushIndex, data.popIndex);

  const uint32_t endGen = data.pushIndex / data.capacity;
  const uint32_t endIdx = data.pushIndex % data.capacity;

  uint32_t gen   = 0;
  uint32_t index = 0;
  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
  ASSERT_EQ(endGen, gen);
  ASSERT_EQ(endIdx, index);

  for(unsigned int j = 0; j < data.expectedClears; ++j)
  {
    ASSERT_TRUE(manager.reservePopForClear(gen, index, endGen, endIdx));
    ASSERT_EQ((data.popIndex + j) / data.capacity, gen);
    ASSERT_EQ((data.popIndex + j) % data.capacity, index);
    manager.commitPopIndex(gen, index);
  }
  ASSERT_FALSE(manager.reservePopForClear(gen, index, endGen, endIdx));
  manager.commitPushIndex(endGen, endIdx);
  ASSERT_EQ(1u, manager.size());
}

ReserveData reserveData[] = {
    {1, 0, 0, 0},

    // Capacity 2 queues for a couple generations
    {2, 0, 0, 0},
    {2, 1, 0, 1},
    {2, 1, 1, 0},
    {2, 2, 1, 1},
    {2, 2, 2, 0},
    {2, 3, 2, 1},
    {2, 3, 3, 0},

    // Capacity 3 queues for a couple generations
    {3, 0, 0, 0},
    {3, 1, 0, 1},
    {3, 1, 1, 0},
    {3, 2, 0, 2},
    {3, 2, 1, 1},
    {3, 2, 2, 0},
    {3, 3, 1, 2},
    {3, 3, 2, 1},
    {3, 3, 3, 0},
    {3, 4, 2, 2},
    {3, 4, 3, 1},
    {3, 4, 4, 0},

    // Capacity 7 queue
    {7, 14, 14, 0},
    {7, 15, 14, 1},
    {7, 20, 14, 6},
    {7, 18, 18, 0},
    {7, 19, 18, 1},
    {7, 24, 18, 6},
};

INSTANTIATE_TEST_CASE_P(TestQueueManagerReserve, Reserve,
                        ::testing::ValuesIn(reserveData), );

TEST(TestQueueManager, Enabled)
{
  QueueManager manager(3);

  ASSERT_TRUE(manager.enabled());

  uint32_t gen   = 0;
  uint32_t index = 0;

  // Insert 2 elements.
  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
  manager.commitPushIndex(gen, index);
  ASSERT_EQ(1u, manager.size());

  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
  manager.commitPushIndex(gen, index);
  ASSERT_EQ(2u, manager.size());

  // Disable the queue.
  manager.disable();
  ASSERT_FALSE(manager.enabled());

  // Test that attempting to push fails.
  ASSERT_EQ(QueueReturn::QueueDisabled, manager.reservePushIndex(gen, index));
  ASSERT_EQ(2u, manager.size());

  // Test that attempting to pop succeeds.
  ASSERT_EQ(QueueReturn::Success, manager.reservePopIndex(gen, index));
  manager.commitPopIndex(gen, index);
  ASSERT_EQ(1u, manager.size());

  // Test that attempting to push still fails.
  ASSERT_EQ(QueueReturn::QueueDisabled, manager.reservePushIndex(gen, index));
  ASSERT_EQ(1u, manager.size());

  // Disable the queue a second time, and verify that has no effect.
  manager.disable();
  ASSERT_FALSE(manager.enabled());

  // Test that attempting to push still fails.
  ASSERT_EQ(QueueReturn::QueueDisabled, manager.reservePushIndex(gen, index));
  ASSERT_EQ(1u, manager.size());

  // Enable the queue.
  manager.enable();
  ASSERT_TRUE(manager.enabled());

  // Test that attempting to push succeeds.
  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
  manager.commitPushIndex(gen, index);
  ASSERT_EQ(2u, manager.size());

  // Test that attempting to pop succeeds.
  ASSERT_EQ(QueueReturn::Success, manager.reservePopIndex(gen, index));
  manager.commitPopIndex(gen, index);
  ASSERT_EQ(1u, manager.size());

  // Enable the queue a second time, and verify that has no effect.
  manager.enable();
  ASSERT_TRUE(manager.enabled());

  // Test that attempting to push succeeds.
  ASSERT_EQ(QueueReturn::Success, manager.reservePushIndex(gen, index));
  manager.commitPushIndex(gen, index);
  ASSERT_EQ(2u, manager.size());
}
