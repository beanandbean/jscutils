#ifndef jscodegen_source_loc_hpp
#define jscodegen_source_loc_hpp

#include <cstddef>

namespace jscodegen {

struct source_loc {
  size_t line;
  size_t column;

  inline source_loc(size_t _line, size_t _column) noexcept
      : line{_line}, column{_column} {}

  inline bool operator==(const source_loc& other) const noexcept {
    return cmp(other) == 0;
  }
  inline bool operator!=(const source_loc& other) const noexcept {
    return cmp(other) != 0;
  }
  inline bool operator<(const source_loc& other) const noexcept {
    return cmp(other) < 0;
  }
  inline bool operator>(const source_loc& other) const noexcept {
    return cmp(other) > 0;
  }
  inline bool operator<=(const source_loc& other) const noexcept {
    return cmp(other) <= 0;
  }
  inline bool operator>=(const source_loc& other) const noexcept {
    return cmp(other) >= 0;
  }

 private:
  // TODO : Use c++ 20 std::strong_ordering
  // TODO: Use c++20 3-way compare operator (and make this public)
  inline int cmp(const source_loc& other) const noexcept {
    if (line < other.line) {
      return -1;
    } else if (line > other.line) {
      return 1;
    } else if (column < other.column) {
      return -1;
    } else if (column > other.column) {
      return 1;
    } else {
      return 0;
    }
  }
};

struct source_range {
  source_loc begin;
  source_loc end;

  inline source_range(source_loc _begin, source_loc _end) noexcept
      : begin{_begin}, end{_end} {}

  inline bool contains(const source_loc& loc) const noexcept {
    return loc >= begin && loc < end;
  }
};

}  // namespace jscodegen

#endif  // jscodegen_source_loc_hpp