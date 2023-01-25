#include <pybind11/eigen.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// nlohmann::json binding
#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include "pybind11_json/pybind11_json.hpp"

// CASM
#include "casm/casm_io/Log.hh"
#include "casm/casm_io/json/InputParser_impl.hh"
#include "casm/casm_io/json/jsonParser.hh"
#include "casm/configuration/clusterography/IntegralCluster.hh"
#include "casm/configuration/group/Group.hh"
#include "casm/configuration/occ_events/OccEvent.hh"
#include "casm/configuration/occ_events/OccEventRep.hh"
#include "casm/configuration/occ_events/OccSystem.hh"
#include "casm/configuration/occ_events/io/json/OccEventCounter_json_io.hh"
#include "casm/configuration/occ_events/io/json/OccEvent_json_io.hh"
#include "casm/configuration/occ_events/io/json/OccSystem_json_io.hh"
#include "casm/configuration/occ_events/orbits.hh"
#include "casm/configuration/sym_info/factor_group.hh"
#include "casm/crystallography/BasicStructure.hh"
#include "casm/crystallography/BasicStructureTools.hh"

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace py = pybind11;

/// CASM - Python binding code
namespace CASMpy {

using namespace CASM;

std::shared_ptr<occ_events::OccSystem> make_system(
    std::shared_ptr<xtal::BasicStructure const> const &prim,
    std::optional<std::vector<std::string>> chemical_name_list = std::nullopt,
    std::optional<std::vector<std::string>> vacancy_name_list = std::nullopt) {
  if (!chemical_name_list.has_value()) {
    chemical_name_list =
        occ_events::make_chemical_name_list(*prim, make_factor_group(*prim));
  }

  std::set<std::string> vacancy_name_set = {"Va", "VA", "va"};
  if (vacancy_name_list.has_value()) {
    vacancy_name_set.clear();
    for (auto const &x : vacancy_name_list.value()) {
      vacancy_name_set.insert(x);
    }
  }

  return std::make_shared<occ_events::OccSystem>(prim, *chemical_name_list,
                                                 vacancy_name_set);
}

occ_events::OccEventRep make_occ_event_rep(
    xtal::UnitCellCoordRep const &_unitcellcoord_rep,
    sym_info::OccSymOpRep const &_occupant_rep,
    sym_info::AtomPositionSymOpRep const &_atom_position_rep) {
  return occ_events::OccEventRep(_unitcellcoord_rep, _occupant_rep,
                                 _atom_position_rep);
}

occ_events::OccEvent make_occ_event(
    std::vector<std::vector<occ_events::OccPosition>> trajectories) {
  std::vector<occ_events::OccTrajectory> _trajectories;
  for (auto const &traj : trajectories) {
    _trajectories.emplace_back(traj);
  }
  return occ_events::OccEvent(_trajectories);
}

occ_events::OccPosition make_default_occ_position() {
  return occ_events::OccPosition::molecule(xtal::UnitCellCoord(0, 0, 0, 0), 0);
}

}  // namespace CASMpy

PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);

PYBIND11_MODULE(_occ_events, m) {
  using namespace CASMpy;

  m.doc() = R"pbdoc(
        Occupation events, such as diffusive hops or molecular re-orientation

        libcasm.occ_events
        ------------------

        The libcasm.occ_events package contains data structures and methods for specifying and enumerating occupation events, determining their symmetry, and generating orbits.

    )pbdoc";
  py::module::import("libcasm.clusterography");
  py::module::import("libcasm.xtal");
  py::module::import("libcasm.sym_info");

  py::class_<occ_events::OccSystem, std::shared_ptr<occ_events::OccSystem>>(
      m, "OccSystem",
      R"pbdoc(
      Holds index conversion tables used with occupation events
      )pbdoc")
      .def(py::init(&make_system), py::arg("prim"),
           py::arg("chemical_name_list") = std::nullopt,
           py::arg("vacancy_name_list") = std::nullopt,
           R"pbdoc(
      Construct an OccSystem

      Parameters
      ----------
      prim: libcasm.xtal.Prim
          A :class:`~libcasm.xtal.Prim`
      chemical_name_list: Optional[list[str]]=None
          Order of chemical name indices (i.e. :func:`~libcasm.xtal.Occupant.name`)
          to use in specifying OccEvents, performing Monte Carlo calculations, etc.
      vacancy_name_list: Optional[list[str]]=None
          Chemical names that should be recognized as vacancies.

      )pbdoc")
      .def(
          "chemical_name_list",
          [](occ_events::OccSystem const &m) { return m.chemical_name_list; },
          "Return the chemical name list.")
      .def(
          "is_vacancy_list",
          [](occ_events::OccSystem const &m) { return m.is_vacancy_list; },
          "Return a list[bool], where `is_vacancy_list[chemical_name_index]` "
          "indicates if the corresponding chemical is a vacancy.")
      .def(
          "orientation_name_list",
          [](occ_events::OccSystem const &m) {
            return m.orientation_name_list;
          },
          "Names of the unique molecular orientations, as determined from the "
          "keys of :func:`~libcasm.xtal.Prim.occupants`.")
      .def_static(
          "from_dict",
          [](const nlohmann::json &data,
             std::shared_ptr<xtal::BasicStructure const> const &prim)
              -> std::shared_ptr<occ_events::OccSystem> {
            jsonParser json{data};
            InputParser<occ_events::OccSystem> event_system_parser(json, prim);
            std::runtime_error error_if_invalid{
                "Error in libcasm.occ_events.OccSystem.from_dict"};
            report_and_throw_if_invalid(event_system_parser, CASM::log(),
                                        error_if_invalid);
            return std::make_shared<occ_events::OccSystem>(
                std::move(*event_system_parser.value));
          },
          R"pbdoc(
          Construct OccSystem from a Python dict

          Parameters
          ----------
          data : dict
              The serialized OccSystem

          prim : libcasm.xtal.Prim
              A :class:`~libcasm.xtal.Prim`

          Returns
          -------
          system : libcasm.occ_events.OccSystem
              The OccSystem
          )pbdoc"
          "Construct OccSystem from a Python dict.",
          py::arg("prim"), py::arg("data"))
      .def(
          "to_dict",
          [](occ_events::OccSystem const &m) -> nlohmann::json {
            jsonParser json;
            to_json(m, json);
            return static_cast<nlohmann::json>(json);
          },
          "Represent the OccSystem as a Python dict.");

  py::class_<occ_events::OccPosition>(m, "OccPosition",
                                      R"pbdoc(
      An atom or molecule position
      )pbdoc")
      .def(py::init(&make_default_occ_position),
           R"pbdoc(
      Construct a default OccPosition, equal to indicating the first occupant in the
      first basis site in the origin unit cell.

      )pbdoc")
      .def_static(
          "molecule",
          [](xtal::UnitCellCoord const &integral_site_coordinate,
             Index occupant_index) {
            return occ_events::OccPosition::molecule(integral_site_coordinate,
                                                     occupant_index);
          },
          R"pbdoc(
          Construct an OccPosition representing an entire molecule, whether
          single or multi-atom.

          This is equivalent to :func:`~libcasm.occ_events.occupant`.
          )pbdoc",
          py::arg("integral_site_coordinate"), py::arg("occupant_index"))
      .def_static(
          "occupant",
          [](xtal::UnitCellCoord const &integral_site_coordinate,
             Index occupant_index) {
            return occ_events::OccPosition::molecule(integral_site_coordinate,
                                                     occupant_index);
          },
          R"pbdoc(
          Construct an OccPosition representing the entire occupant.

          This is equivalent to :func:`~libcasm.occ_events.molecule`.
          )pbdoc",
          py::arg("integral_site_coordinate"), py::arg("occupant_index"))
      .def_static(
          "atom_component",
          [](xtal::UnitCellCoord const &integral_site_coordinate,
             Index occupant_index, Index atom_position_index) {
            return occ_events::OccPosition::atom(
                integral_site_coordinate, occupant_index, atom_position_index);
          },
          "Construct an OccPosition representing an atom component of a "
          "multi-atom molecule.",
          py::arg("integral_site_coordinate"), py::arg("occupant_index"),
          py::arg("atom_position_index"))
      .def(
          "is_in_resevoir",
          [](occ_events::OccPosition const &pos) { return pos.is_in_resevoir; },
          "If true, indicates molecule/atom in resevoir. If false, indicates a "
          "molecule/atom on integral_site_coordinate.")
      .def(
          "is_atom",
          [](occ_events::OccPosition const &pos) { return pos.is_atom; },
          "If true, indicates this tracks an atom component. If false, then "
          "this tracks a molecule position.")
      .def(
          "integral_site_coordinate",
          [](occ_events::OccPosition const &pos) {
            return pos.integral_site_coordinate;
          },
          "If is_in_resevoir() is False: Integral coordinates of site "
          "containing occupant; otherwise invalid.")
      .def(
          "occupant_index",
          [](occ_events::OccPosition const &pos) { return pos.occupant_index; },
          "If is_in_resevoir() is False: Index of occupant in "
          ":func:`~libcasm.xtal.Prim.occ_dof` for sublattice specified by "
          "`integral_site_coordinate`.  If is_in_resevoir() is True: Index "
          "into :func:`~libcasm.occ_events.OccSystem.chemical_name_list` of a "
          "molecule in the resevoir.")
      .def(
          "atom_position_index",
          [](occ_events::OccPosition const &pos) { return pos.occupant_index; },
          "If is_atom() is True and is_in_resevoir() is False: Index of atom "
          "position in the indicated occupant molecule.")
      .def_static(
          "from_dict",
          [](const nlohmann::json &data,
             occ_events::OccSystem const &system) -> occ_events::OccPosition {
            jsonParser json{data};
            return jsonConstructor<occ_events::OccPosition>::from_json(json,
                                                                       system);
          },
          R"pbdoc(
          Construct an OccPosition from a Python dict

          Parameters
          ----------
          data : dict
              The serialized OccPosition

          system : libcasm.occ_events.OccSystem
              A :class:`~libcasm.occ_events.OccSystem`

          Returns
          -------
          event : libcasm.occ_events.OccPosition
              The OccPosition
          )pbdoc",
          py::arg("data"), py::arg("system"))
      .def(
          "to_dict",
          [](occ_events::OccPosition const &pos,
             occ_events::OccSystem const &system) -> nlohmann::json {
            jsonParser json;
            to_json(pos, json, system);
            return static_cast<nlohmann::json>(json);
          },
          R"pbdoc(
          Represent the OccPosition as a Python dict

          Parameters
          ----------
          system : libcasm.occ_events.OccSystem
              A :class:`~libcasm.occ_events.OccSystem`

          Returns
          -------
          data : dict
              The OccEvent as a Python dict
          )pbdoc",
          py::arg("system"));

  py::class_<occ_events::OccEventRep> pyOccEventRep(m, "OccEventRep",
                                                    R"pbdoc(
      Symmetry representation for transformating an OccEvent.
    )pbdoc");

  py::class_<occ_events::OccEvent> pyOccEvent(m, "OccEvent",
                                              R"pbdoc(
      OccEvent represents an occupation event, for example the change in
      occupation due to a diffusive hop or molecular re-orientation. The
      occupation change is represented by occupant trajectories.

      Example, 1NN A-Va exchange in an FCC prim:

      .. code-block:: Python

          import libcasm.xtal as xtal
          from libcasm.xtal.prims import FCC as FCC_prim
          from libcasm.occ_events import OccPosition, OccEvent

          prim = FCC_prim(r=1.0, occ_dof=["A", "B", "Va"])

          site1 = xtal.IntegralSiteCoordinate(sublattice=0, unitcell=[0, 0, 0])
          site2 = xtal.IntegralSiteCoordinate(sublattice=0, unitcell=[1, 0, 0])

          A_occ_index = 0
          Va_occ_index = 2

          A_initial_pos = OccPosition.molecule(site1, A_occ_index)
          A_final_pos = OccPosition.molecule(site2, A_occ_index)
          Va_initial_pos = OccPosition.molecule(site2, Va_occ_index)
          Va_final_pos = OccPosition.molecule(site1, Va_occ_index)

          occ_event = OccEvent([
              [A_initial_pos, A_final_pos],
              [Va_initial_pos, Va_final_pos]
          ])

    )pbdoc");

  pyOccEventRep
      .def(py::init(&make_occ_event_rep),
           R"pbdoc(
          Construct an OccEventRep

          Parameters
          ----------
          integral_site_coordinate_rep: libcasm.xtal.IntegralSiteCoordinateRep
              Symmetry representation for transforming IntegralSiteCoordinate

          occupant_rep: list[list[int]]
              Permutations describe occupant index transformation under symmetry.
              Usage:

                  occupant_index_after =
                      occupant_rep[sublattice_index_before][occupant_index_before]

          atom_position_rep: list[list[list[int]]]
              Permutations describe atom position index transformation under
              symmetry.

              Usage:

                  atom_position_index_after =
                      atom_position_rep[sublattice_index_before][occupant_index_before][atom_position_index_before]

           )pbdoc",
           py::arg("integral_site_coordinate_rep"), py::arg("occupant_rep"),
           py::arg("atom_position_rep"))
      .def(
          "__mul__",
          [](occ_events::OccEventRep const &self,
             occ_events::OccEvent const &event) {
            return copy_apply(self, event);
          },
          py::arg("event"),
          "Creates a copy of the OccEvent and applies the symmetry operation "
          "represented by this OccEventRep.");

  m.def(
      "make_occevent_symgroup_rep",
      [](std::vector<xtal::SymOp> const &group_elements,
         xtal::BasicStructure const &xtal_prim) {
        return occ_events::make_occevent_symgroup_rep(group_elements,
                                                      xtal_prim);
      },
      R"pbdoc(
      Construct a group representation of OccEventRep

      Parameters
      ----------
      group_elements: list[libcasm.xtal.SymOp]
          Symmetry group elements

      xtal_prim: libcasm.xtal.Prim
          The Prim structure

      Returns
      -------
      occevent_symgroup_rep: list[OccEventRep]
          Group representation for transforming OccEvent
      )pbdoc",
      py::arg("group_elements"), py::arg("xtal_prim"));

  m.def(
      "make_occevent_symgroup_rep",
      [](std::vector<xtal::UnitCellCoordRep> const &unitcellcoord_symgroup_rep,
         std::vector<sym_info::OccSymOpRep> const &occ_symgroup_rep,
         std::vector<sym_info::AtomPositionSymOpRep> const
             &atom_position_symgroup_rep) {
        return occ_events::make_occevent_symgroup_rep(
            unitcellcoord_symgroup_rep, occ_symgroup_rep,
            atom_position_symgroup_rep);
      },
      R"pbdoc(
      Construct a group representation of OccEventRep

      Parameters
      ----------
      unitcellcoord_symgroup_rep: list[libcasm.xtal.IntegralSiteCoordinateRep]
          The symmetry group representation that describes how IntegralSiteCoordinate transform under symmetry.
      occ_symgroup_rep: list[list[list[int]]]
          Permutations describe occupant index transformation under symmetry. Indices are: group_element_index, sublattice_index_before, and occupant_index_before; and the resulting value is occupant_index_after.
      atom_position_symgroup_rep: list[list[list[list[int]]]]
          Permutations describe atom position index transformation under symmetry. Indices are: group_element_index, sublattice_index_before, occupant_index_before, atom_position_index_before; and the resulting value is atom_position_index_after.

      Returns
      -------
      occevent_symgroup_rep: list[OccEventRep]
          Group representation for transforming OccEvent
      )pbdoc",
      py::arg("unitcellcoord_symgroup_rep"), py::arg("occ_symgroup_rep"),
      py::arg("atom_position_symgroup_rep"));

  pyOccEvent
      .def(py::init(&make_occ_event),
           R"pbdoc(
      Construct an OccEvent

      Parameters
      ----------
      trajectories: list[list[OccPosition]]=[]
          The occupant trajectories. Usage: `trajectories[i_occupant][0]` is the initial position of the i-th occupant, and `trajectories[i_occupant][1]` is the final position of the i-th occupant. Most methods currently support trajectories of length 2 only (an initial position and a final position).

      )pbdoc",
           py::arg("trajectories") =
               std::vector<std::vector<occ_events::OccPosition>>{})
      .def(
          "size",
          [](occ_events::OccEvent const &event) { return event.size(); },
          "The number of trajectories.")
      .def(
          "trajectories",
          [](occ_events::OccEvent const &event) {
            std::vector<std::vector<occ_events::OccPosition>> trajectories(
                event.size());
            for (Index i_traj = 0; i_traj < event.size(); ++i_traj) {
              for (auto const &pos : event[i_traj].position) {
                trajectories[i_traj].push_back(pos);
              }
            }
            return trajectories;
          },
          R"pbdoc(
          Return the event trajectories

          Returns
          -------
          trajectories: list[list[OccPosition]]=[]
             The occupant trajectories. Usage: `trajectories[i_occupant][0]` is the initial position of the i-th occupant, and `trajectories[i_occupant][1]` is the final position of the i-th occupant. Most methods currently support trajectories of length 2 only (an initial position and a final position).

          )pbdoc")
      .def(
          "__add__",
          [](occ_events::OccEvent const &event,
             Eigen::Vector3l const &translation) {
            return event + translation;
          },
          "Translate the OccEvent by adding unit cell indices")
      .def(
          "__iadd__",
          [](occ_events::OccEvent &event, Eigen::Vector3l const &translation) {
            return event += translation;
          },
          "Translate the OccEvent by adding unit cell indices")
      .def(
          "__sub__",
          [](occ_events::OccEvent const &event,
             Eigen::Vector3l const &translation) {
            return event - translation;
          },
          "Translate the OccEvent by subtracting unit cell indices")
      .def(
          "__isub__",
          [](occ_events::OccEvent &event, Eigen::Vector3l const &translation) {
            return event -= translation;
          },
          "Translate the OccEvent by subtracting unit cell indices")
      .def(
          "sort", [](occ_events::OccEvent &event) { return sort(event); },
          "Sort event trajectories")
      .def(
          "copy_sort",
          [](occ_events::OccEvent const &event) { return copy_sort(event); },
          "Return a copy of the event with sorted trajectories")
      .def(
          "reverse", [](occ_events::OccEvent &event) { return reverse(event); },
          "Reverse event trajectories")
      .def(
          "copy_reverse",
          [](occ_events::OccEvent const &event) { return copy_sort(event); },
          "Return a copy of the event with reversed trajectories")
      .def(
          "standardize",
          [](occ_events::OccEvent &event) { return standardize(event); },
          "Put event into standardized form with regard to "
          "permutation/reversal")
      .def(
          "cluster",
          [](occ_events::OccEvent const &event) {
            return make_cluster_occupation(event).first;
          },
          "The cluster of sites involved in the OccEvent.")
      .def(
          "initial_occupation",
          [](occ_events::OccEvent const &event) {
            return make_cluster_occupation(event).second[0];
          },
          "Occupant indices on each site in the cluster, in the initial "
          "positions. Order of sites is consistent with self.cluster().")
      .def(
          "final_occupation",
          [](occ_events::OccEvent const &event) {
            return make_cluster_occupation(event).second[1];
          },
          "Occupant indices on each site in the cluster, in the final "
          "positions. Order of sites is consistent with self.cluster().")
      .def(py::self < py::self,
           "Compares via lexicographical order of trajectories")
      .def(py::self <= py::self,
           "Compares via lexicographical order of trajectories")
      .def(py::self > py::self,
           "Compares via lexicographical order of trajectories")
      .def(py::self >= py::self, "Compares via lexicographical order of sites")
      .def(py::self == py::self, "True if events are equal")
      .def(py::self != py::self, "True if events are not equal")
      .def_static(
          "from_dict",
          [](const nlohmann::json &data,
             occ_events::OccSystem const &system) -> occ_events::OccEvent {
            jsonParser json{data};
            return jsonConstructor<occ_events::OccEvent>::from_json(json,
                                                                    system);
          },
          R"pbdoc(
          Construct an OccEvent from a Python dict

          Parameters
          ----------
          data : dict
              The serialized OccEvent

          system : libcasm.occ_events.OccSystem
              A :class:`~libcasm.occ_events.OccSystem`

          Returns
          -------
          event : libcasm.occ_events.OccEvent
              The OccEvent
          )pbdoc",
          py::arg("data"), py::arg("system"))
      .def(
          "to_dict",
          [](occ_events::OccEvent const &event,
             occ_events::OccSystem const &system, bool include_cluster,
             bool include_cluster_occupation,
             bool include_event_invariants) -> nlohmann::json {
            occ_events::OccEventOutputOptions opt;
            opt.include_cluster = include_cluster;
            opt.include_cluster_occupation = include_cluster_occupation;
            opt.include_event_invariants = include_event_invariants;
            jsonParser json;
            to_json(event, json, system, opt);
            return static_cast<nlohmann::json>(json);
          },
          R"pbdoc(
          Represent the OccEvent as a Python dict

          Parameters
          ----------
          system : libcasm.occ_events.OccSystem
              A :class:`~libcasm.occ_events.OccSystem`

          include_cluster: bool = True
              If True, also include the cluster sites

          include_cluster_occupation: bool = True
              If True, also include the initial and final cluster occupation

          include_event_invariants: bool = True
              If True, also include event invariants: number of trajectories,
              number of each occupant type, and site distances

          Returns
          -------
          data : dict
              The OccEvent as a Python dict
          )pbdoc",
          py::arg("system"), py::arg("include_cluster") = true,
          py::arg("include_cluster_occupation") = true,
          py::arg("include_event_invariants") = true);

  m.def(
      "make_prim_periodic_orbit",
      [](occ_events::OccEvent const &orbit_element,
         std::vector<occ_events::OccEventRep> const &occevent_symgroup_rep) {
        std::set<occ_events::OccEvent> orbit =
            make_prim_periodic_orbit(orbit_element, occevent_symgroup_rep);
        return std::vector<occ_events::OccEvent>(orbit.begin(), orbit.end());
      },
      R"pbdoc(
      Construct an orbit of OccEvent

      The orbit of OccEvent is all distinct OccEvent that are equivalent
      under the provided symmetry group, including one element for all
      OccEvent that are equivalent according to prim translational symmetry.

      Parameters
      ----------
      orbit_element : OccEvent
          One OccEvent in the orbit

      occevent_symgroup_rep: list[OccEventRep]
          Symmetry group representation.

      Returns
      -------
      orbit : list[OccEvent]
          The orbit of OccEvent
      )pbdoc",
      py::arg("orbit_element"), py::arg("occevent_symgroup_rep"));

  m.def(
      "make_occevent_group",
      [](occ_events::OccEvent const &occ_event,
         std::shared_ptr<occ_events::SymGroup const> const &group,
         xtal::Lattice const &lattice,
         std::vector<occ_events::OccEventRep> const &occevent_symgroup_rep) {
        return occ_events::make_occevent_group(
            occ_event, group, lattice.lat_column_mat(), occevent_symgroup_rep);
      },
      R"pbdoc(
      Construct a subgroup which leaves an event invariant

      Parameters
      ----------
      occ_event : OccEvent
          The OccEvent that remains invariant after transformation by
          subgroup elements.

      group: list[libcasm.xtal.SymOp]
          The super group.

      lattice: xtal.Lattice
          The lattice.

      occevent_symgroup_rep: list[OccEventRep]
          Representation of `group` for transforming OccEventRep.

      Returns
      -------
      subgroup : libcasm.sym_info.SymGroup
          The subgroup which leaves the event invariant
      )pbdoc",
      py::arg("occ_event"), py::arg("group"), py::arg("lattice"),
      py::arg("occevent_symgroup_rep"));

#ifdef VERSION_INFO
  m.attr("__version__") = MACRO_STRINGIFY(VERSION_INFO);
#else
  m.attr("__version__") = "dev";
#endif
}
