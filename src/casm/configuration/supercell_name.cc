#include "casm/configuration/supercell_name.hh"

#include "casm/casm_io/container/stream_io.hh"
#include "casm/crystallography/Lattice.hh"
#include "casm/misc/CASM_Eigen_math.hh"
#include "casm/misc/string_algorithm.hh"

namespace CASM {
namespace config {

/// \brief Return a string representing the HNF of a matrix
///
/// String format is: SCELV_A_B_C_D_E_F, where:
/// - V: H(0, 0) * H(1, 1) * H(2, 2)
/// - A: H(0,0),
/// - B: H(1,1),
/// - C: H(2,2),
/// - D: H(1,2),
/// - E: H(0,2),
/// - F: H(0,1),
/// - H: hermite_normal_form(matrix)
std::string hermite_normal_form_name(const Eigen::Matrix3l &matrix) {
  std::string name_str;

  Eigen::Matrix3i H = hermite_normal_form(matrix.cast<int>()).first;
  name_str = "SCEL";
  std::stringstream tname;
  // Consider using a for loop with HermiteCounter_impl::_canonical_unroll here
  tname << H(0, 0) * H(1, 1) * H(2, 2) << "_" << H(0, 0) << "_" << H(1, 1)
        << "_" << H(2, 2) << "_" << H(1, 2) << "_" << H(0, 2) << "_" << H(0, 1);
  name_str.append(tname.str());

  return name_str;
}

/// \brief Inverse function of `hermite_normal_form_name`
Eigen::Matrix3l make_hermite_normal_form(std::string hermite_normal_form_name) {
  std::vector<std::string> tokens;
  try {
    char_separator sep("SCEL_");
    tokenizer tok(hermite_normal_form_name, sep);
    std::copy_if(tok.begin(), tok.end(), std::back_inserter(tokens),
                 [](const std::string &val) { return !val.empty(); });
    if (tokens.size() != 7) {
      throw std::invalid_argument(
          "Error in make_supercell: supercell name format error");
    }
    Eigen::Matrix3l T;
    auto cast = [](std::string val) { return std::stol(val); };
    T << cast(tokens[1]), cast(tokens[6]), cast(tokens[5]), 0, cast(tokens[2]),
        cast(tokens[4]), 0, 0, cast(tokens[3]);
    return T;
  } catch (std::exception &e) {
    std::string format = "SCELV_T00_T11_T22_T12_T02_T01";
    std::stringstream ss;
    ss << "Error in make_hermite_normal_form: "
       << "expected format: " << format << ", "
       << "name: |" << hermite_normal_form_name << "|"
       << ", "
       << "tokens: " << tokens << ", "
       << "tokens.size(): " << tokens.size() << ", "
       << "error: " << e.what();
    throw std::runtime_error(ss.str());
  }
}

/// \brief Make the supercell name of a superlattice
///
/// The supercell name is a string generated from the hermite
/// normal form, H, of the transformation matrix, T, that generates
/// `superlattice` from `prim_lattice`:
///
/// \[
///     H = \mathrm{hermite_normal_form}(T)
///     S = L * T
/// \]
///
/// where S is the lattice vectors, as columns of a matrix, of
/// the `superlattice`, and L is the lattice vectors, as columns of
/// a matrix, of `prim_lattice`.
///
/// String format is: SCELV_A_B_C_D_E_F, where:
/// - V: H(0, 0) * H(1, 1) * H(2, 2)
/// - A: H(0,0),
/// - B: H(1,1),
/// - C: H(2,2),
/// - D: H(1,2),
/// - E: H(0,2),
/// - F: H(0,1),
/// - H: hermite_normal_form(T)
///
/// \param prim_lattice The primitive lattice
/// \param superlattice The superlattice whose name is to be generated
///
/// \returns A name for the superlattice
///
std::string make_supercell_name(xtal::Lattice const &prim_lattice,
                                xtal::Lattice const &superlattice) {
  Eigen::Matrix3l T = xtal::make_transformation_matrix_to_super(
      prim_lattice, superlattice, prim_lattice.tol());
  return hermite_normal_form_name(T);
}

/// \brief Construct a superlattice from the supercell name
///
/// Constructs a superlattice, S = L * H, where H is read from supercell_name
/// as generated by `make_supercell_name`.
///
/// \param prim_lattice The primitive lattice
/// \param supercell_name The name of the super lattice
///
/// \returns The superlattice with the specified supercell name. The result
///     may not be in canonical form.
///
xtal::Lattice make_superlattice_from_supercell_name(
    xtal::Lattice const &prim_lattice, std::string supercell_name) {
  Eigen::Matrix3l H = make_hermite_normal_form(supercell_name);
  return make_superlattice(prim_lattice, H);
}

}  // namespace config
}  // namespace CASM
