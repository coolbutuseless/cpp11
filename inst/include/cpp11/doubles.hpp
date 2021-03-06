#pragma once

#include <algorithm>              // for min
#include <array>                  // for array
#include <initializer_list>       // for initializer_list
#include <stdexcept>              // for out_of_range
#include "cpp11/as.hpp"         // for as_sexp
#include "cpp11/named_arg.hpp"  // for named_arg
#include "cpp11/protect.hpp"    // for SEXP, SEXPREC, REAL_ELT, R_Preserve...
#include "cpp11/vector.hpp"     // for vector, vector<>::proxy, vector<>::...
// Specializations for doubles

namespace cpp11 {

template <>
inline SEXP vector<double>::valid_type(SEXP data) {
  if (TYPEOF(data) != REALSXP) {
    throw type_error(REALSXP, TYPEOF(data));
  }
  return data;
}

template <>
inline double vector<double>::operator[](const R_xlen_t pos) const {
  // NOPROTECT: likely too costly to unwind protect every elt
  return is_altrep_ ? REAL_ELT(data_, pos) : data_p_[pos];
}

template <>
inline double vector<double>::at(const R_xlen_t pos) const {
  if (pos < 0 || pos >= length_) {
    throw std::out_of_range("doubles");
  }
  // NOPROTECT: likely too costly to unwind protect every elt
  return is_altrep_ ? REAL_ELT(data_, pos) : data_p_[pos];
}

template <>
inline double* vector<double>::get_p(bool is_altrep, SEXP data) {
  if (is_altrep) {
    return nullptr;
  } else {
    return REAL(data);
  }
}

template <>
inline void vector<double>::const_iterator::fill_buf(R_xlen_t pos) {
  length_ = std::min(static_cast<R_xlen_t>(64L), data_.size() - pos);
  unwind_protect([&] { REAL_GET_REGION(data_.data_, pos, length_, buf_.data()); });
  block_start_ = pos;
}

typedef vector<double> doubles;

namespace writable {

template <>
inline typename vector<double>::proxy& vector<double>::proxy::operator=(double rhs) {
  if (is_altrep_) {
    // NOPROTECT: likely too costly to unwind protect every set elt
    SET_REAL_ELT(data_, index_, rhs);
  } else {
    *p_ = rhs;
  }
  return *this;
}

template <>
inline vector<double>::proxy::operator double() const {
  if (p_ == nullptr) {
    // NOPROTECT: likely too costly to unwind protect every elt
    return REAL_ELT(data_, index_);
  } else {
    return *p_;
  }
}

template <>
inline vector<double>::vector(std::initializer_list<double> il)
    : cpp11::vector<double>(as_sexp(il)), capacity_(il.size()) {}

template <>
inline vector<double>::vector(std::initializer_list<named_arg> il)
    : cpp11::vector<double>(safe[Rf_allocVector](REALSXP, il.size())),
      capacity_(il.size()) {
  try {
    unwind_protect([&] {
      protect_ = protect_sexp(data_);
      Rf_setAttrib(data_, R_NamesSymbol, Rf_allocVector(STRSXP, capacity_));
      SEXP names(Rf_getAttrib(data_, R_NamesSymbol));
      auto it = il.begin();
      for (R_xlen_t i = 0; i < capacity_; ++i, ++it) {
        data_p_[i] = doubles(it->value())[0];
        SET_STRING_ELT(names, i, Rf_mkCharCE(it->name(), CE_UTF8));
      }
    });
  } catch (const unwind_exception& e) {
    release_protect(protect_);
    throw e;
  }
}

template <>
inline void vector<double>::reserve(R_xlen_t new_capacity) {
  data_ = data_ == R_NilValue ? safe[Rf_allocVector](REALSXP, new_capacity)
                              : safe[Rf_xlengthgets](data_, new_capacity);
  SEXP old_protect = protect_;
  protect_ = protect_sexp(data_);
  release_protect(old_protect);

  data_p_ = REAL(data_);
  capacity_ = new_capacity;
}

template <>
inline void vector<double>::push_back(double value) {
  while (length_ >= capacity_) {
    reserve(capacity_ == 0 ? 1 : capacity_ *= 2);
  }
  if (is_altrep_) {
    SET_REAL_ELT(data_, length_, value);
  } else {
    data_p_[length_] = value;
  }
  ++length_;
}

typedef vector<double> doubles;

}  // namespace writable

}  // namespace cpp11
