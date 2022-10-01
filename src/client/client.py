import struct
from dataclasses import dataclass
import socket
from time import sleep

from cli_parser import get_args, ActionType, ClientRequest


@dataclass
class DirList:
    f_type: str
    _size: int
    f_name: str

    def __str__(self):
        return f"{self.f_type} {self.f_size} {self.f_name}"

    @property
    def f_size(self) -> str:
        """Print the value of the size as "human-readable" """
        suffix = "B"
        for unit in ["", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"]:
            if abs(self._size) < 1024.0:
                return f"{self._size:0.0f}{unit}{suffix}"
            self._size /= 1024.0


def _do_list_ldir(args: ClientRequest) -> None:
    contents = ""
    for file in args.src.iterdir():
        # Only ready file and dirs
        if file.is_file() or file.is_dir():
            _type = "[F]" if file.is_file() else "[D]"
            contents += f"{_type}:{file.stat().st_size}:{file.name}\n"

    _parse_dir(contents)


def _parse_dir(array: str) -> None:
    """
    Break the stream of characters that represents the directory listing
    and format it to print to the screen.

    :param array: String array in the format of `f_type:f_size:f_name\n`
    """
    dir_objs = []
    for file in array.split("\n"):
        if "" == file:
            break

        _type, size, name = file.split(":")
        if not size.isdigit():
            raise ValueError("File size is not of type int")
        dir_objs.append(DirList(_type, int(size), name))

    dir_objs.sort(key=lambda x: (x.f_type, -x._size))
    for i in dir_objs:
        print(f"{i.f_type} {i.f_size:>4} {i.f_name}")


def _do_lmkdir(args: ClientRequest) -> None:
    try:
        args.src.mkdir()
        print("[+] Create directory")
    except FileNotFoundError:
        print("[!] Path is missing parent directories. Make those directories "
              "first")
    except FileExistsError:
        print("[!] The directory you are attempting to create already exists")
    except Exception as error:
        print(f"[!] {error}")


def _do_ldelete(args: ClientRequest) -> None:
    try:
        if args.src.is_file():
            args.src.unlink()
            print("[!] Deleted file")
        else:
            args.src.rmdir()
            print("[!] Deleted directory")

    except FileNotFoundError:
        print("[!] File not found")
    except Exception as error:
        print(f"[!] {error}")


def _socket_timedout(conn: socket.socket):
    i = conn.recv(1, socket.MSG_PEEK)
    payload = struct.unpack(">B", i)
    print("The timout func read ", payload)
    if i[0] == 2:
        return True
    return False


def main() -> None:
    args = None
    try:
        args = get_args()
    except Exception as error:
        exit(error)

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as conn:
            payload = struct.pack(">B", 1)
            print(args.socket)
            conn.connect(args.socket)
            sleep(7)
            if _socket_timedout(conn):
                exit("[!] Session timeout")

            i = conn.send(payload)
            print("got ", i)

    if ActionType.L_LS == args.action:
        _do_list_ldir(args)

    elif ActionType.L_MKDIR == args.action:
        _do_lmkdir(args)

    elif ActionType.L_DELETE == args.action:
        _do_ldelete(args)

        # print(f"[Sent to server] Client TCP port: {tcp_port}")
        # print(f"[Sent to server] Client will send: {payload_size} bytes")


if __name__ == "__main__":
    main()
