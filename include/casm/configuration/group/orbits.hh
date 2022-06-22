#ifndef CASM_group_orbits
#define CASM_group_orbits

#include <memory>
#include <set>

#include "casm/configuration/group/definitions.hh"

namespace CASM {
namespace group {

/// \brief Make an orbit by applying group elements to one element
///     of the orbit
///
/// \param orbit_element One element of the orbit
/// \param group_begin,group_end Group elements used to generate the
///     orbit
/// \param compare_f, Binary function used to compare orbit elements.
/// \param copy_apply_f Function used to apply group element to orbit
///     elements, according to `copy_apply_f(group_element, orbit_element)`
///     which returns a new orbit element.
///
/// \returns orbit, A set containing the unique orbit elements
///
template <typename OrbitElementType, typename GroupElementIt,
          typename CompareType, typename CopyApplyType>
std::set<OrbitElementType, CompareType> make_orbit(
    OrbitElementType const &orbit_element, GroupElementIt group_begin,
    GroupElementIt group_end, CompareType compare_f,
    CopyApplyType copy_apply_f) {
  std::set<ElementType, CompareElementType> orbit(compare_f);
  for (; group_begin != group_end; ++group_begin) {
    orbit.emplace(copy_apply_f(*group_begin, orbit_element));
  }
  return orbit;
);

/// \brief Make the orbit equivalence map
///
/// Generate a lookup table for which group elements applied to the
/// first element in an orbit generate each other element in the orbit.
///
/// \param orbit The orbit of unique elements generated by the group
/// \param group_begin,group_end Group elements used to generate the
///     orbit
/// \param copy_apply_f Function used to apply group element to orbit
///     elements, according to `copy_apply_f(group_element, orbit_element)`
///     which returns a new orbit element.
/// \returns equivalence_map The indices equivalence_map[i] are the
///     indices of the group elements which map orbit element 0 onto
///     orbit element i
/// \param group The group used to generate the orbit
///
/// \returns invariant_subgroups, The indices invariant_subgroups[i] are
///     the indices of the group elements which leave orbit element i
///     invariant.
///
template <typename OrbitElementType, typename GroupElementIt,
          typename CompareType, typename CopyApplyType>
std::vector<std::vector<Index>> make_equivalence_map(
    std::set<OrbitElementType, CompareType> const &orbit,
    GroupElementIt group_begin, GroupElementIt group_end,
    CopyApplyType copy_apply_f) {
  std::vector<std::vector<Index>> equivalence_map;
  equivalence_map.resize(orbit.size());
  Index i = 0;
  for (; group_begin != group_end; ++group_begin) {
    auto it = orbit.find(copy_apply_f(*group_begin, orbit_element));
    if (it == orbit.end()) {
      throw std::runtime_error("Error in make_equivalence_map: failed");
    }
    Index d = std::distance(orbit.begin(), it);
    equivalence_map[d].push_back(i);
    ++i;
  }
  return equivalence_map;
}

}  // namespace group
}  // namespace CASM

#endif
