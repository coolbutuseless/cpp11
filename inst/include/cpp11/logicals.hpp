#pragma once

#include <algorithm>         // for min
#include <array>             // for array
#include <initializer_list>  // for initializer_list
#include <stdexcept>         // for out_of_range
#include "cpp11/R.hpp"
#include "cpp11/named_arg.hpp"  // for named_arg
#include "cpp11/protect.hpp"    // for protect_sexp, Rf_allocVector
#include "cpp11/vector.hpp"     // for vector, vector<>::proxy, vector<>::...

// Specializations for logicals

namespace cpp11 {

template <>
inline SEXP vector<Rboolean>::valid_type(SEXP data) {
  if (TYPEOF(data) != LGLSXP) {
    throw type_error(LGLSXP, TYPEOF(data));
  }
  return data;
}

template <>
inline Rboolean vector<Rboolean>::operator[](const R_xlen_t pos) const {
  return is_altrep_ ? static_cast<Rboolean>(LOGICAL_ELT(data_, pos)) : data_p_[pos];
}

template <>
inline Rboolean vector<Rboolean>::at(const R_xlen_t pos) const {
  if (pos < 0 || pos >= length_) {
    throw std::out_of_range("logicals");
  }
  return is_altrep_ ? static_cast<Rboolean>(LOGICAL_ELT(data_, pos)) : data_p_[pos];
}

template <>
inline Rboolean* vector<Rboolean>::get_p(bool is_altrep, SEXP data) {
  if (is_altrep) {
    return nullptr;
  } else {
    return reinterpret_cast<Rboolean*>(LOGICAL(data));
  }
}

template <>
inline void vector<Rboolean>::const_iterator::fill_buf(R_xlen_t pos) {
  length_ = std::min(static_cast<R_xlen_t>(64L), data_.size() - pos);
  LOGICAL_GET_REGION(data_.data_, pos, length_, reinterpret_cast<int*>(buf_.data()));
  block_start_ = pos;
}

typedef vector<Rboolean> logicals;

namespace writable {

template <>
inline typename vector<Rboolean>::proxy& vector<Rboolean>::proxy::operator=(
    Rboolean rhs) {
  if (is_altrep_) {
    SET_LOGICAL_ELT(data_, index_, rhs);
  } else {
    *p_ = rhs;
  }
  return *this;
}

template <>
inline vector<Rboolean>::proxy::operator Rboolean() const {
  if (p_ == nullptr) {
    return static_cast<Rboolean>(LOGICAL_ELT(data_, index_));
  } else {
    return *p_;
  }
}

template <>
inline vector<Rboolean>::vector(std::initializer_list<Rboolean> il)
    : cpp11::vector<Rboolean>(Rf_allocVector(LGLSXP, il.size())), capacity_(il.size()) {
  protect_ = protect_sexp(data_);
  auto it = il.begin();
  for (R_xlen_t i = 0; i < capacity_; ++i, ++it) {
    SET_LOGICAL_ELT(data_, i, *it);
  }
}

template <>
inline vector<Rboolean>::vector(std::initializer_list<named_arg> il)
    : cpp11::vector<Rboolean>(safe[Rf_allocVector](LGLSXP, il.size())),
      capacity_(il.size()) {
  try {
    unwind_protect([&] {
      protect_ = protect_sexp(data_);
      attr("names") = Rf_allocVector(STRSXP, capacity_);
      SEXP names = attr("names");
      auto it = il.begin();
      for (R_xlen_t i = 0; i < capacity_; ++i, ++it) {
        data_p_[i] = logicals(it->value())[0];
        SET_STRING_ELT(names, i, Rf_mkCharCE(it->name(), CE_UTF8));
      }
    });
  } catch (const unwind_exception& e) {
    release_protect(protect_);
    throw e;
  }
}

template <>
inline void vector<Rboolean>::reserve(R_xlen_t new_capacity) {
  data_ = data_ == R_NilValue ? safe[Rf_allocVector](LGLSXP, new_capacity)
                              : safe[Rf_xlengthgets](data_, new_capacity);
  SEXP old_protect = protect_;
  protect_ = protect_sexp(data_);

  release_protect(old_protect);

  data_p_ = reinterpret_cast<Rboolean*>(LOGICAL(data_));
  capacity_ = new_capacity;
}

template <>
inline void vector<Rboolean>::push_back(Rboolean value) {
  while (length_ >= capacity_) {
    reserve(capacity_ == 0 ? 1 : capacity_ *= 2);
  }
  if (is_altrep_) {
    SET_LOGICAL_ELT(data_, length_, value);
  } else {
    data_p_[length_] = value;
  }
  ++length_;
}

typedef vector<Rboolean> logicals;

}  // namespace writable

}  // namespace cpp11
