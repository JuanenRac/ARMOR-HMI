// ARMOR-HMI host tests - the one-line check every test file uses.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#pragma once
#include <cstdio>

static int failures = 0;
static int checks = 0;
#define CHECK(condition)                                                              \
  do {                                                                                \
    ++checks;                                                                         \
    if (!(condition)) {                                                               \
      ++failures;                                                                     \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);                \
    }                                                                                 \
  } while (0)
#define FINISH(name)                                                                  \
  do {                                                                                \
    std::printf("%s: %d checks, %d failed\n", name, checks, failures);                \
    return failures == 0 ? 0 : 1;                                                     \
  } while (0)
