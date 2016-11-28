#ifndef OPTIMET_MPI_FAST_MATRIX_MULTIPLY_H

#include "../FastMatrixMultiply.h"
#include <tuple>
#include <utility>
#include <vector>

namespace optimet {
namespace mpi {
namespace details {
//! \brief Splits interactions into local and non-local processes
//! \details If true, then that element will be computed locally.
Matrix<bool> local_interactions(t_int nscatterers);
//! \brief Distribution of input/output vector
Vector<t_int> vector_distribution(t_int nscatterers, t_int nprocs);
//! \brief Splits interactions into local and non-local processes
inline Matrix<t_int> matrix_distribution(Matrix<bool> const &local, Vector<t_int> const &columns) {
  return local.select(columns.transpose().replicate(columns.size(), 1),
                      columns.replicate(1, columns.size()));
}
//! Combines splitting alongst local-non-local and alongs columns
inline Matrix<t_int> matrix_distribution(t_int nscatterers, t_int nprocs) {
  return matrix_distribution(local_interactions(nscatterers),
                             vector_distribution(nscatterers, nprocs));
}

//! Figures out graph connectivity for given distribution
std::vector<std::set<t_int>>
local_graph_edges(Matrix<bool> const &locals, Vector<t_int> const &vector_distribution);
//! Figures out graph connectivity for given distribution
std::vector<std::set<t_int>>
non_local_graph_edges(Matrix<bool> const &nonlocals, Vector<t_int> const &vector_distribution);
}

class FastMatrixMultiply {

public:
  FastMatrixMultiply(ElectroMagnetic const &em_background, t_real wavenumber,
                     std::vector<Scatterer> const &scatterers, Matrix<bool> const local_nonlocal,
                     Vector<t_int> const vector_distribution,
                     mpi::Communicator const &comm = mpi::Communicator())
      : local_fmm_(em_background, wavenumber, scatterers,
                   local_dist(local_nonlocal, vector_distribution, comm.rank())),
        nonlocal_fmm_(em_background, wavenumber, scatterers,
                      nonlocal_dist(local_nonlocal, vector_distribution, comm.rank())) {}
  FastMatrixMultiply(ElectroMagnetic const &em_background, t_real wavenumber,
                     std::vector<Scatterer> const &scatterers,
                     mpi::Communicator const &comm = mpi::Communicator())
      : FastMatrixMultiply(em_background, wavenumber, scatterers,
                           details::local_interactions(scatterers.size()),
                           details::vector_distribution(scatterers.size(), comm.size()), comm) {}
  FastMatrixMultiply(t_real wavenumber, std::vector<Scatterer> const &scatterers,
                     Matrix<bool> const matrix_distribution,
                     Vector<t_int> const vector_distribution,
                     mpi::Communicator const &comm = mpi::Communicator())
      : FastMatrixMultiply(ElectroMagnetic(), wavenumber, scatterers, matrix_distribution,
                           vector_distribution, comm) {}
  FastMatrixMultiply(t_real wavenumber, std::vector<Scatterer> const &scatterers,
                     mpi::Communicator const &comm = mpi::Communicator())
      : FastMatrixMultiply(ElectroMagnetic(), wavenumber, scatterers, comm) {}

  FastMatrixMultiply &communicator(mpi::Communicator const &comm) {
    communicator_ = comm;
    return *this;
  }
  mpi::Communicator const &communicator() const { return communicator_; }

private:
  optimet::FastMatrixMultiply local_fmm_;
  optimet::FastMatrixMultiply nonlocal_fmm_;
  //! MPI communicator over which to perform calculation
  mpi::Communicator communicator_;

  template <class T0, class T1>
  Matrix<bool> local_dist(Eigen::MatrixBase<T0> const &locals, Eigen::MatrixBase<T1> const &vecdist,
                          t_uint rank) {
    return locals.array() && (vecdist.transpose().array() == rank).replicate(vecdist.size(), 1);
  }
  template <class T0, class T1>
  Matrix<bool> nonlocal_dist(Eigen::MatrixBase<T0> const &locals,
                             Eigen::MatrixBase<T1> const &vecdist, t_uint rank) {
    return (locals.array() == false) && (vecdist.array() == rank).replicate(1, vecdist.size());
  }
};
}
}

#endif