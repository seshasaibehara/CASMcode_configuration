import os

__version__ = "2.0.dev1"

# Available at setup time due to pyproject.toml
from pybind11.setup_helpers import (
    ParallelCompile,
    Pybind11Extension,
    build_ext,
    naive_recompile,
)
from setuptools import find_namespace_packages, setup

casm_num_build_jobs = os.getenv("CASM_NUM_BUILD_JOBS")
if casm_num_build_jobs is None:
    raise Exception("CASM_NUM_BUILD_JOBS not set")
ParallelCompile("NPY_NUM_BUILD_JOBS", needs_recompile=naive_recompile).install()

casm_prefix = os.getenv("CASM_PREFIX")
if casm_prefix is None:
    raise Exception("CASM_PREFIX not set")

with open(os.path.join("README.md"), encoding="utf-8") as f:
    long_description = f.read()

# The main interface is through Pybind11Extension.
# * You can add cxx_std=11/14/17, and then build_ext can be removed.
# * You can set include_pybind11=false to add the include directory yourself,
#   say from a submodule.
#
# Note:
#   Sort input source files if you glob sources to ensure bit-for-bit
#   reproducible builds (https://github.com/pybind/python_example/pull/53)

ext_modules_params = {
    "define_macros": [
        ("VERSION_INFO", __version__),
    ],
    "cxx_std": 17,
    "library_dirs": [
        os.path.join(casm_prefix, "lib"),
    ],
    "include_dirs": [
        os.path.join(casm_prefix, "include/casm/external"),
        os.path.join(casm_prefix, "include"),
    ],
    "extra_compile_args": [
        "-D_LIBCPP_DISABLE_AVAILABILITY",
        "--std=c++17",
    ],
    "extra_link_args": [
        "-lcasm_global",
        "-lcasm_crystallography",
        "-lcasm_clexulator",
        "-lcasm_configuration",
    ],
}

ext_modules = [
    Pybind11Extension(
        "libcasm.sym_info._sym_info", ["src/sym_info.cpp"], **ext_modules_params
    ),
    Pybind11Extension(
        "libcasm.clusterography._clusterography",
        ["src/clusterography.cpp"],
        **ext_modules_params
    ),
    Pybind11Extension(
        "libcasm.configuration._configuration",
        ["src/configuration.cpp"],
        **ext_modules_params
    ),
    Pybind11Extension(
        "libcasm.irreps._irreps", ["src/irreps.cpp"], **ext_modules_params
    ),
    Pybind11Extension(
        "libcasm.occ_events._occ_events", ["src/occ_events.cpp"], **ext_modules_params
    ),
    Pybind11Extension(
        "libcasm.enumerate._enumerate", ["src/enumerate.cpp"], **ext_modules_params
    ),
]

setup(
    name="libcasm-configuration",
    version=__version__,
    url="https://github.com/prisms-center/CASMcode_configuration",
    description="CASM configuration library Python interface",
    long_description=long_description,
    long_description_content_type="text/markdown",
    author="CASM developers",
    author_email="casm-developers@lists.engr.ucsb.edu",
    license="LGPL2.1+",
    packages=find_namespace_packages(include=["libcasm.*"]),
    install_requires=["pybind11", "numpy", "sortedcontainers"],
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "License :: OSI Approved :: "
        "GNU Lesser General Public License v2 or later (LGPLv2+)",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Topic :: Scientific/Engineering",
    ],
    data_files=[("", ["LICENSE"])],
    ext_modules=ext_modules,
    extras_require={"test": "pytest"},
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.8",
)