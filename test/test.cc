#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../include/resizable_buffer.h"
#include "doctest.h"

using rb::ReleasedResizableBuffer;
using rb::ResizableBuffer;

TEST_CASE("Create with 0 length") {
  SUBCASE("with no parameter") {
    ResizableBuffer<int> buffer;
    CHECK_EQ(buffer.length(), 0);
    CHECK_EQ(buffer.buffer(), nullptr);
  }

  SUBCASE("with 0") {
    ResizableBuffer<int> buffer(0);
    CHECK_EQ(buffer.length(), 0);
    CHECK_EQ(buffer.byte_length(), 0);
    CHECK_EQ(buffer.buffer(), nullptr);

    ReleasedResizableBuffer<int> released_buffer = buffer.Release();
    CHECK_EQ(released_buffer.length, 0);
    CHECK_EQ(released_buffer.byte_length, 0);
    CHECK_EQ(released_buffer.buffer, nullptr);
    CHECK_EQ(buffer.length(), 0);
    CHECK_EQ(buffer.byte_length(), 0);
    CHECK_EQ(buffer.buffer(), nullptr);
  }
}

TEST_CASE("Create with length") {
  SUBCASE("char") {
    ResizableBuffer<char> buffer(10);
    CHECK_EQ(buffer.length(), 10);
    CHECK_EQ(buffer.byte_length(), 10 * sizeof(char));
    CHECK_NE(buffer.buffer(), nullptr);

    for (int i = 0; i < 10; ++i) {
      CHECK_EQ(buffer.buffer()[i], 0);
    }

    snprintf(buffer.buffer(), buffer.length(), "hellotest");
    CHECK_EQ(strcmp(buffer.buffer(), "hellotest"), 0);
  }

  SUBCASE("int") {
    ResizableBuffer<int> buffer(10);
    CHECK_EQ(buffer.length(), 10);
    CHECK_EQ(buffer.byte_length(), 10 * sizeof(int));
    CHECK_NE(buffer.buffer(), nullptr);

    for (int i = 0; i < 10; ++i) {
      CHECK_EQ(buffer.buffer()[i], 0);
    }

    for (size_t i = buffer.length() - 1, j = 0; i != 0xFFFFFFFFFFFFFFFF;
         --i, ++j) {
      buffer.buffer()[i] = j;
    }
    for (size_t i = buffer.length() - 1, j = 0; i != 0xFFFFFFFFFFFFFFFF;
         --i, ++j) {
      CHECK_EQ(buffer.buffer()[i], j);
    }

    char* temp = buffer.byte_buffer();
    for (size_t i = 0; i < buffer.byte_length(); ++i) {
      if (i % sizeof(int) == 0) {
        CHECK_EQ(temp[i], 9 - (i / sizeof(int)));
      } else {
        CHECK_EQ(temp[i], 0);
      }
    }
  }
}

TEST_CASE("Realloc") {
  SUBCASE("new is smaller") {
    ResizableBuffer<char> buffer(10);
    auto buf = buffer.buffer();
    snprintf(buffer.buffer(), buffer.length(), "hellotest");
    buffer.Realloc(5);

    CHECK_EQ(buffer.buffer(), buf);
    CHECK_EQ(buffer.length(), 5);
    CHECK_EQ(buffer.byte_length(), 5 * sizeof(char));

    // `Realloc` doesn't change the content of the buffer if the new buffer is
    // smaller.
    for (int i = 0; i < 5; ++i) {
      CHECK_EQ(buffer.buffer()[i], "hello"[i]);
    }
  }

  SUBCASE("new is the same") {
    ResizableBuffer<char> buffer(10);
    auto buf = buffer.buffer();
    snprintf(buffer.buffer(), buffer.length(), "hellotest");
    buffer.Realloc(10);

    CHECK_EQ(buffer.buffer(), buf);
    CHECK_EQ(buffer.length(), 10);
    CHECK_EQ(buffer.byte_length(), 10 * sizeof(char));

    // `Realloc` doesn't change the content of the buffer if the new buffer is
    // the same size.
    for (int i = 0; i < 10; ++i) {
      CHECK_EQ(buffer.buffer()[i], "hellotest"[i]);
    }
  }

  SUBCASE("new is larger") {
    ResizableBuffer<char> buffer(10);
    auto buf = buffer.buffer();
    snprintf(buffer.buffer(), buffer.length(), "hellotest");
    buffer.Realloc(getpagesize() * 2);

    WARN_NE(buffer.buffer(), buf);
    CHECK_EQ(buffer.length(), getpagesize() * 2);
    CHECK_EQ(buffer.byte_length(), getpagesize() * 2 * sizeof(char));

    // `Realloc` set all the bytes to 0 if the new buffer is larger.
    for (int i = 0; i < getpagesize() * 2; ++i) {
      CHECK_EQ(buffer.buffer()[i], 0);
    }
  }
}

TEST_CASE("Release") {
  SUBCASE("normal test") {
    ResizableBuffer<char> buffer(10);
    auto buf = buffer.buffer();
    snprintf(buffer.buffer(), buffer.length(), "hellotest");

    ReleasedResizableBuffer<char> released_buffer = buffer.Release();
    CHECK_EQ(released_buffer.length, 10);
    CHECK_EQ(released_buffer.byte_length, 10 * sizeof(char));
    CHECK_EQ(released_buffer.buffer, buf);
    CHECK_EQ(buffer.length(), 0);
    CHECK_EQ(buffer.byte_length(), 0);
    CHECK_EQ(buffer.buffer(), nullptr);

    // `Release` doesn't change the content of the buffer.
    for (int i = 0; i < 10; ++i) {
      CHECK_EQ(released_buffer.buffer[i], "hellotest"[i]);
    }

    released_buffer.Free();
  }

  SUBCASE("release nullptr") {
    ResizableBuffer<char> buffer(10);
    snprintf(buffer.buffer(), buffer.length(), "hellotest");

    ReleasedResizableBuffer<char> released_buffer = buffer.Release();
    released_buffer.Free();

    CHECK_EQ(buffer.length(), 0);
    CHECK_EQ(buffer.byte_length(), 0);
    CHECK_EQ(buffer.buffer(), nullptr);

    ReleasedResizableBuffer<char> released_buffer2 = buffer.Release();
    CHECK_EQ(released_buffer2.length, 0);
    CHECK_EQ(released_buffer2.byte_length, 0);
    CHECK_EQ(released_buffer2.buffer, nullptr);
  }

  SUBCASE("release realloced") {
    ResizableBuffer<char> buffer(10);
    auto buf = buffer.buffer();
    snprintf(buffer.buffer(), buffer.length(), "hellotest");
    buffer.Realloc(5);

    ReleasedResizableBuffer<char> released_buffer = buffer.Release();
    CHECK_EQ(released_buffer.length, 5);
    CHECK_EQ(released_buffer.byte_length, 5 * sizeof(char));
    CHECK_EQ(released_buffer.buffer, buf);
    CHECK_EQ(buffer.length(), 0);
    CHECK_EQ(buffer.byte_length(), 0);
    CHECK_EQ(buffer.buffer(), nullptr);

    // `Release` doesn't change the content of the buffer if the new buffer is
    // smaller.
    for (int i = 0; i < 5; ++i) {
      CHECK_EQ(released_buffer.buffer[i], "hello"[i]);
    }
    for (int i = 5; i < 10; ++i) {
      CHECK_EQ(released_buffer.buffer[i], "test"[i - 5]);
    }

    released_buffer.Free();
  }
}
