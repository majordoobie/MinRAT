from __future__ import annotations

import struct
from enum import Enum, auto, unique
from pathlib import Path
from typing import Optional


@unique
class ActionType(Enum):
    NO_OP = 0
    USER_OP = 1
    DELETE = 2
    LS = 3
    GET = 4
    MKDIR = 5
    PUT = 6
    LOCAL_OP = 7

    CREATE_USER = 10
    DELETE_USER = 20

    SHELL = auto()
    L_LS = auto()
    L_DELETE = auto()
    L_MKDIR = auto()


class DependencyAction(Enum):
    """
    The action type Enums have a value that specifies the dependency of that
    action.
    0 - No dependency
    1 - Depends on --src
    2 - Depends on --dst
    3 - Depends on --src and --dst
    4 - Depends on --perm
    """
    SHELL = 0
    DELETE_USER = 0
    L_LS = 1
    L_DELETE = 1
    L_MKDIR = 1
    LS = 2
    MKDIR = 2
    DELETE = 2
    PUT = 3
    GET = 3
    CREATE_USER = 4


class UserPerm(Enum):
    READ = 1
    READ_WRITE = 2
    ADMIN = 3

    def __str__(self):
        """Provides an easier to read output when using -h"""
        return self.name

    @staticmethod
    def permission(perm: str):
        """Called from argparse.parse_args(). Takes the cli argument and
        attempts to return a UserPerm object if it exists."""
        try:
            return UserPerm[perm.upper()]
        except KeyError:
            raise ValueError()


class ClientRequest:
    def __init__(self, host: str,
                 port: int,
                 username: str,
                 src: Optional[Path],
                 dst: Optional[str],
                 perm: Optional[UserPerm] = UserPerm.READ,
                 session_id: Optional[int] = 0,
                 **kwargs) -> None:
        """
        Create a "dataclass" containing the configuration and action required
        to communicate with the server.

        Args:
            host (str): IP of the server
            port (int): Port of the server
            username (str): Username used to either authenticate or to action
                on when using commands such as "--create_user"
            src (:obj:`pathlib.Path`, optional): Path to the source file to
                reference
            dst (:obj:`pathlib.Path`, optional): Path to the server file to
                reference
            perm (:obj:`UserPerm`, optional): Permission to set to user when
                using "--create_user"
            **kwargs (dict): Remainder of optional arguments passed.
        """
        self._host = host
        self._port = port
        self._username = username
        self._src = src
        self._dst = dst
        self._perm = perm
        self._session_id = session_id
        self._password: str = ""

        self._action: [ActionType] = ActionType.NO_OP
        self._user_flag: [ActionType] = ActionType.NO_OP
        self._local_action: [ActionType] = ActionType.NO_OP

        self._other_username: str = ""
        self._other_password: str = ""

        self._parse_kwargs(kwargs)

    @property
    def socket(self) -> tuple[str, int]:
        return self._host, self._port

    @property
    def passwd(self) -> str:
        return self._password

    @passwd.setter
    def passwd(self, value: str) -> None:
        self._password = value

    @property
    def client_request(self):
        """
        0               1               2               3
        0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |     OPCODE    |   USER_FLAG   |           RESERVED            |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |        USERNAME_LEN           |        PASSWORD_LEN           |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                          SESSION_ID                           |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                    **USERNAME + PASSWORD**                    |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                          PAYLOAD_LEN ->                       |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                       <- PAYLOAD_LEN                          |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                ~user_payload || std_payload~                  |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        """
        request_header = bytearray(struct.pack("!BBHHHL",
                                               self._action.value,
                                               self._user_flag,
                                               0,
                                               len(self._username),
                                               len(self.passwd),
                                               self._session_id,
                                               ))
        request_header += self._username.encode(encoding="utf-8")
        request_header += self._password.encode(encoding="utf-8")

        """
        Create the user payload
           0               1               2               3   
           0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           |  USR_ACT_FLAG |   PERMISSION  |          USERNAME_LEN         |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           | **USERNAME**  |         PASSWORD_LEN          | **PASSWORD**  |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        """
        if self._action == ActionType.USER_OP:
            user_payload = struct.pack("!BBH",
                                       self._user_flag.value,
                                       self._perm.value,
                                       len(self._other_username)
                                       )

            user_payload += self._other_username.encode(encoding="utf-8")

            if 0 != len(self._other_password):
                user_payload += struct.pack("!H", len(self._other_password))
                user_payload += self._other_password.encode("utf-8")

            request_header += struct.pack("!Q", len(user_payload))
            request_header += user_payload

        return request_header



    def _parse_kwargs(self, kwargs) -> None:
        """
        Parse the remaining arguments and ensure that only a single action
        is specified. If more than a single argument is passed in, raise
        an error

        Args:
            kwargs (dict): Remainder of arguments passed in

        Raises:
            ValueError: If more than a single action is passed in or if the
                action is missing dependencies such as --src or --dst
        """
        action = None
        for key, value in kwargs.items():
            if value:
                if key in ("create_user", "delete_use"):
                    self._other_username = value
                if action is not None:
                    raise ValueError("[!] Only one command flag may be set")
                action = key

        if action is None:
            raise ValueError("[!] No command flag set")

        for name, member in ActionType.__members__.items():
            if name == action.upper():
                self._action: ActionType = member

        dep_value = 0
        for name, member in DependencyAction.__members__.items():
            if name == self._action.name:
                dep_value = member.value
                break

        if dep_value == 1:
            if self._src is None:
                raise ValueError(f"[!] Command \"--{self._action.name.lower()}\""
                                 f" requires \"--src\" argument")

        elif dep_value == 2:
            if self._dst is None:
                raise ValueError(f"[!] Command \"--{self._action.name.lower()}\""
                                 f" requires \"--dst\" argument")

        elif dep_value == 3:
            if self._dst is None or self._src is None:
                raise ValueError(f"[!] Command \"--{self._action.name.lower()}\""
                                 f" requires \"--src\" and \"--dst\" argument")

        elif dep_value == 4:
            if self._perm is None:
                raise ValueError(f"[!] Command \"--{self._action.name.lower()}\""
                                 f" requires \"--perm\" argument")

        # Is command is to either create or delete a user, set the action
        # flag to USER_OP and set the USER_FLAG to the type of user op
        if self._action in (ActionType.DELETE_USER, ActionType.CREATE_USER):
            self._user_flag = self._action
            self._action = ActionType.USER_OP

        # If command is a local operation, set the action type to the LOCAL_OP
        # this will instruct the server to only authenticate
        if self._action in (ActionType.L_LS, ActionType.L_MKDIR, ActionType.L_DELETE):
            self._user_flag = self._action
            self._action = ActionType.LOCAL_OP


