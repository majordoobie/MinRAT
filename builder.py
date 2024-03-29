import argparse
import shutil
import subprocess
from multiprocessing import cpu_count
from pathlib import Path
from typing import Union, List

TEST_BIN_PATH = "test_bin"
PROJ_BIN = "gtest_server"


def main() -> None:
    args = _get_args()
    test_binaries = _get_test_names(args.build)

    if args.list_all:
        if not test_binaries:
            exit("[!] Nothing to list. Please build the project first with "
                 "--build")
        test_binaries.sort()
        for test_name in test_binaries:
            print(test_name.name)
        exit()

    if args.run_all:
        _build(args)

        for binary in _get_test_names(args.build):
            print("Bin: ", binary)
            if binary.name == PROJ_BIN:
                subprocess.run(binary.as_posix())
                exit()

    elif args.type:
        _build(args)
        for binary in _get_test_names(args.build):
            if binary.name == args.type:
                subprocess.run(binary.as_posix())
                exit()

        exit("Could not find the test name you supplied. Use the "
             "--list-test to see all available tests")

    else:
        _build(args)


def _build(args: argparse.Namespace) -> None:
    subprocess.run(f"cmake -DCMAKE_BUILD_TYPE=Debug -S . -B {args.build}", shell=True)
    subprocess.run(f"cmake --build {args.build} -j {cpu_count()}", shell=True)


def _get_test_names(build_path: Path) -> List[Path]:
    test_path = build_path / TEST_BIN_PATH
    if not test_path.exists() or not test_path.is_dir():
        return []

    return [proj_dir for proj_dir in test_path.iterdir()]


def _get_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="%(prog)s runs either all unit tests in the project or"
                    "the tests for a specific data structure"
    )

    parser.add_argument(
        "-l", "--list-tests",
        help="List the name of the tests you can choose",
        action="store_true",
        dest="list_all",
        default=False
    )

    parser.add_argument(
        "-b", "--build",
        dest="build",
        type=Path,
        metavar="build path",
        nargs="?",
        const=Path("./build"),
        default=Path("./build"),
        help="Build and compile the project into the build directory specified "
             "(Default: %(default)s)"
    )

    group = parser.add_mutually_exclusive_group()

    group.add_argument(
        "-r", "--run-all",
        help="Run all tests available",
        dest="run_all",
        action="store_true",
        default=False
    )
    group.add_argument(
        "-t", "--test-name",
        help="Run a specific data structures tests. Use the \"--list-tests\" "
             "to get a listing of the available test names",
        type=str,
        dest="type"
    )

    return parser.parse_args()


if __name__ == "__main__":
    main()