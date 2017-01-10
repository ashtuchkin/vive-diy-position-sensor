// Author: mierle@gmail.com (Keir Mierle)

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "ring_buffer.h"

TEST_CASE("Pushing elements until full", "[ring_buffer]") {
  RingBuffer<int, 4> buffer;

  // Zero.
  REQUIRE(buffer.IsFull() == false);
  REQUIRE(buffer.IsEmpty() == true);

  // One.
  REQUIRE(buffer.PushFront(10) == true);
  REQUIRE(buffer.IsFull() == false);
  REQUIRE(buffer.IsEmpty() == false);

  // Two.
  REQUIRE(buffer.PushFront(11) == true);
  REQUIRE(buffer.IsFull() == false);
  REQUIRE(buffer.IsEmpty() == false);

  // Three.
  REQUIRE(buffer.PushFront(12) == true);
  REQUIRE(buffer.IsFull() == false);
  REQUIRE(buffer.IsEmpty() == false);

  // Four. It's full after this; next one should overwrite last element.
  REQUIRE(buffer.PushFront(13) == true);
  REQUIRE(buffer.IsFull() == true);
  REQUIRE(buffer.IsEmpty() == false);

  // Four again; last element was forced out.
  REQUIRE(buffer.PushFront(14) == false);  // False since this clobbered.
  REQUIRE(buffer.IsFull() == true);
  REQUIRE(buffer.IsEmpty() == false);
}

TEST_CASE("Popping from a queue", "[ring_buffer]") {
  RingBuffer<int, 4> buffer;
  int value = -1;

  // Nothing pushed yet; should not be possible to pop.
  REQUIRE(buffer.PopBack(&value) == false);

  // Push a value, pop it out. Should indicate an available value with true.
  buffer.PushFront(12);
  REQUIRE(buffer.PopBack(&value) == true);
  REQUIRE(buffer.IsEmpty() == true);
  REQUIRE(value == 12);

  // Push three, pop them off.
  buffer.PushFront(13);
  buffer.PushFront(14);
  buffer.PushFront(15);
  REQUIRE(buffer.PopBack(&value) == true);
  REQUIRE(value == 13);

  REQUIRE(buffer.PopBack(&value) == true);
  REQUIRE(value == 14);

  REQUIRE(buffer.PopBack(&value) == true);
  REQUIRE(value == 15);
  REQUIRE(buffer.IsEmpty() == true);

  // Can't pop anymore off; it's empty now.
  REQUIRE(buffer.PopBack(&value) == false);
  REQUIRE(buffer.IsEmpty() == true);

  // Now push six on and pop four off.
  buffer.PushFront(23);
  buffer.PushFront(24);
  buffer.PushFront(25);
  buffer.PushFront(26);
  buffer.PushFront(27);
  buffer.PushFront(28);
  // Four.
  REQUIRE(buffer.PopBack(&value) == true);
  REQUIRE(value == 25);
  REQUIRE(buffer.IsEmpty() == false);

  // Three.
  REQUIRE(buffer.PopBack(&value) == true);
  REQUIRE(value == 26);
  REQUIRE(buffer.IsEmpty() == false);

  // Two.
  REQUIRE(buffer.PopBack(&value) == true);
  REQUIRE(value == 27);
  REQUIRE(buffer.IsEmpty() == false);

  // One.
  REQUIRE(buffer.PopBack(&value) == true);
  REQUIRE(value == 28);
  REQUIRE(buffer.IsEmpty() == true);

  // Zero.
  REQUIRE(buffer.PopBack(&value) == false);
}

struct MyStructy {
  int a;
  int b;
  short c;
};

TEST_CASE("Using a struct as values", "[ring_buffer]") {
  RingBuffer<MyStructy, 4> buffer;
  {
    MyStructy my_structy = {1, 2, 3};
    buffer.PushFront(my_structy);
  }
  {
    MyStructy my_structy = {4, 5, 6};
    buffer.PushFront(my_structy);
  }
  MyStructy popped;

  REQUIRE(buffer.PopBack(&popped) == true);
  REQUIRE(popped.a == 1);
  REQUIRE(popped.b == 2);
  REQUIRE(popped.c == 3);

  REQUIRE(buffer.PopBack(&popped) == true);
  REQUIRE(popped.a == 4);
  REQUIRE(popped.b == 5);
  REQUIRE(popped.c == 6);
}

