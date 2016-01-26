#include <exception>
#include "scalapack/Context.h"
#include "scalapack/InitExit.h"
#include "scalapack/Blacs.h"

namespace optimet {
namespace scalapack {

void Context::delete_context(Context::Impl *const impl) {
  if(impl->row >= 0 and impl->col >= 0) {
    if(not finalized())
      OPTIMET_FC_GLOBAL_(blacs_gridexit, BLACS_GRIDEXIT)(&impl->context);
    decrement_ref();
  }
  delete impl;
}

Context::Context(t_uint rows, t_uint cols) : impl(nullptr) {
  if(rows * cols == 0)
    throw std::runtime_error("Context of size 0");
  int nrows = static_cast<t_uint>(rows);
  int ncols = static_cast<t_uint>(cols);
  int this_row = -1, this_col = -1;
  int context;
  int rank, size = 0;
  OPTIMET_FC_GLOBAL_(blacs_pinfo, BLACS_PINFO)(&rank, &size);
  if(size == 0) {
    size = static_cast<int>(rows * cols);
    OPTIMET_FC_GLOBAL_(blacs_setup, BLACS_SETUP)(&rank, &size);
  }

  this_row = -1;
  this_col = 0;
  OPTIMET_FC_GLOBAL_(blacs_get, BLACS_GET)(&this_row, &this_col, &context);
  char order = 'R';
  OPTIMET_FC_GLOBAL_(blacs_gridinit, BLACS_GRIDINIT)(&context, &order, &nrows, &ncols);
  OPTIMET_FC_GLOBAL_(
      blacs_gridinfo, BLACS_GRIDINFO)(&context, &nrows, &ncols, &this_row, &this_col);
  Impl const data{context, static_cast<t_int>(nrows), static_cast<t_int>(ncols),
                  static_cast<t_int>(this_row), static_cast<t_int>(this_col)};
  impl = std::shared_ptr<Impl const>(new Impl(data), &Context::delete_context);
  if(impl and this_row >= 0 and this_row <= 0)
    increment_ref();
}

} /* scalapack  */
} /* optimet  */