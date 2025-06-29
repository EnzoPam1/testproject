from enum import Enum
from . import ai
from . import utile
from typing import Tuple
import subprocess
from itertools import cycle
from random import randint


class Orientation(Enum):
    UP = 1
    RIGHT = 2
    DOWN = 3
    LEFT = 4


class Position:
    def __init__(self, x: int, y: int, map_x: int, map_y: int):
        self.x = x
        self.y = y
        self.map_x = map_x
        self.map_y = map_y

    def __str__(self):
        return "x: " + str(self.x) + " y: " + str(self.y)

    def __eq__(self, other):
        if isinstance(other, Position):
            return self.x == other.x and self.y == other.y
        else:
            return False

    def __ne__(self, other):
        return not self.__eq__(other)

    def get_pos(self):
        return (self.x, self.y)

    def set_pos(self, x: int, y: int):
        self.x = x
        self.y = y

    def go_up(self):
        if self.y == 0:
            self.y = self.map_y - 1
        self.y -= 1

    def go_down(self):
        if self.y == self.map_y - 1:
            self.y = 0
        self.y += 1

    def go_left(self):
        if self.x == 0:
            self.x = self.map_x - 1
        self.x -= 1

    def go_right(self):
        if self.x == self.map_x - 1:
            self.x = 0
        self.x += 1


for_level = [
    # nb player, linemate, deraumere, sibur, mendiane, phiras, thystame
    [1, 1, 0, 0, 0, 0, 0],
    [2, 1, 1, 1, 0, 0, 0],
    [2, 2, 0, 1, 0, 2, 0],
    [4, 1, 1, 2, 0, 1, 0],
    [4, 1, 2, 1, 3, 0, 0],
    [6, 1, 2, 3, 0, 1, 0],
    [6, 2, 2, 2, 2, 2, 1],
]

vision = [
    [1, 3],
    [4, 8],
    [9, 15],
    [16, 24],
    [25, 35],
    [36, 48],
    [49, 63],
    [64, 80],
]
items = ["food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]


class Inventory:
    def __init__(self):
        self.food = 0
        self.linemate = 0
        self.deraumere = 0
        self.sibur = 0
        self.mendiane = 0
        self.phiras = 0
        self.thystame = 0

    def add_ressource(self, ressource: str, amount: int):
        if ressource == "food":
            self.food += amount
        elif ressource == "linemate":
            self.linemate += amount
        elif ressource == "deraumere":
            self.deraumere += amount
        elif ressource == "sibur":
            self.sibur += amount
        elif ressource == "mendiane":
            self.mendiane += amount
        elif ressource == "phiras":
            self.phiras += amount
        elif ressource == "thystame":
            self.thystame += amount
        else:
            raise Exception("Invalid ressource")

    def remove_ressource(self, ressource: str, amount: int):
        if ressource == "food":
            if self.food < amount:
                raise Exception("Not enough food")
            self.food -= amount
        elif ressource == "linemate":
            if self.linemate < amount:
                raise Exception("Not enough linemate")
            self.linemate -= amount
        elif ressource == "deraumere":
            if self.deraumere < amount:
                raise Exception("Not enough deraumere")
            self.deraumere -= amount
        elif ressource == "sibur":
            if self.sibur < amount:
                raise Exception("Not enough sibur")
            self.sibur -= amount
        elif ressource == "mendiane":
            if self.mendiane < amount:
                raise Exception("Not enough mendiane")
            self.mendiane -= amount
        elif ressource == "phiras":
            if self.phiras < amount:
                raise Exception("Not enough phiras")
            self.phiras -= amount
        elif ressource == "thystame":
            if self.thystame < amount:
                raise Exception("Not enough thystame")
            self.thystame -= amount
        else:
            raise Exception("Invalid ressource")

    def get_ressource(self, ressource: str):
        if ressource == "food":
            return self.food
        elif ressource == "linemate":
            return self.linemate
        elif ressource == "deraumere":
            return self.deraumere
        elif ressource == "sibur":
            return self.sibur
        elif ressource == "mendiane":
            return self.mendiane
        elif ressource == "phiras":
            return self.phiras
        elif ressource == "thystame":
            return self.thystame
        else:
            raise Exception("Invalid ressource")

    def print_inventory(self):
        message = "food " + str(self.food) + ","
        message += "linemate " + str(self.linemate) + ","
        message += "deraumere " + str(self.deraumere) + ","
        message += "sibur " + str(self.sibur) + ","
        message += "mendiane " + str(self.mendiane) + ","
        message += "phiras " + str(self.phiras) + ","
        message += "thystame " + str(self.thystame)
        return message

    def set_ressource(self, item: str, amount: int):
        if "food" in item:
            self.food = int(amount)
        elif "linemate" in item:
            self.linemate = int(amount)
        elif "deraumere" in item:
            self.deraumere = int(amount)
        elif "sibur" in item:
            self.sibur = int(amount)
        elif "mendiane" in item:
            self.mendiane = int(amount)
        elif "phiras" in item:
            self.phiras = int(amount)
        elif "thystame" in item:
            self.thystame = int(amount)
        else:
            raise Exception("Invalid ressource")

    def set_inventory(self, inventory: list):
        for i in range(7):
            self.set_ressource(items[i], inventory[i][1])


##class player:


class Player:
    def __init__(self, team_name: str, servport, servhost):
        self.id = 0
        self.level = 1
        self.inventory = Inventory()
        self.share_inventory = [
            Inventory(),
            Inventory(),
            Inventory(),
            Inventory(),
            Inventory(),
            Inventory(),
            Inventory(),
        ]
        self.orientation = Orientation.UP
        self.slot_open = 0

        self.command = []
        self.look_content = []
        self.inventory_content = []
        self.team_name = team_name
        self.servport = servport
        self.servhost = servhost
        self.new_object = False
        self.ressource_to_take = ""

        self.enough_player = False

        self.first_turn = True
        self.inventory_in_turn = False
        self.is_elevating = False
        self.move_in_turn = False

    def encode(self, message: str):
        return utile.xor_compressed_cipher(str(self.id) + "|" + message, self.team_name)

    def decode(self, message: str):
        return utile.xor_compressed_decipher(message, self.team_name)

    def add_memory(self, content: list):
        if self.orientation == Orientation.UP:
            self.add_memory_up(content)
        elif self.orientation == Orientation.DOWN:
            self.add_memory_down(content)
        elif self.orientation == Orientation.LEFT:
            self.add_memory_left(content)
        elif self.orientation == Orientation.RIGHT:
            self.add_memory_right(content)

    def look(self, content: str):
        content = content.split(",")
        content = self.add_memory(content)

    def broadcast_inventory(self):
        self.command.append(
            "Broadcast " + self.encode("inventory|" + self.inventory.print_inventory())
        )

    def elevation(self):
        pass

    def fork(self):
        pass

    def left(self):
        self.orientation = (self.orientation - 1) % 4
        pass

    def right(self):
        self.orientation = (self.orientation + 1) % 4
        pass

    def forward(self):
        if self.orientation == Orientation.UP:
            self.pos.go_up()
        elif self.orientation == Orientation.DOWN:
            self.pos.go_down()
        elif self.orientation == Orientation.LEFT:
            self.pos.go_left()
        elif self.orientation == Orientation.RIGHT:
            self.pos.go_right()
        pass

    def find_object(self, obj: str):
        # find the nearest object on the self.map

        pass

    def need_to_elevation(self) -> str:
        cpt = 1
        for i in items:
            if i == "food":
                continue
            if self.share_inventory[6].get_ressource(i) < for_level[self.level - 1][cpt]:
                return i
            cpt += 1
        return None

    def go_to(self, pos: int, item: str):
        cpt = 0
        if pos == 0:
            for i in range(len(self.look_content[0])):
                if item in self.look_content[0][i]:
                    self.command.append("Take " + item)
            self.inventory_in_turn = True
            return
        self.command.append("Forward")
        while not vision[cpt][0] <= pos <= vision[cpt][1]:
            self.command.append("Forward")
            cpt += 1
        direction = pos - ((vision[cpt][0] + vision[cpt][1]) // 2)
        if direction < 0:
            self.command.append("Left")
        elif direction > 0:
            self.command.append("Right")
        for i in range(abs(direction)):
            self.command.append("Forward")
        for i in range(len(self.look_content[cpt])):
            if item in self.look_content[cpt][i]:
                self.command.append("Take " + item)
        self.inventory_in_turn = True
        return

    def item_in_look(self, item: str) -> int:
        for i in range(len(self.look_content)):
            if item in self.look_content[i]:
                return i
        return None

    def take_item(self):
        nessesary_food = 0
        if self.id == 0:
            nessesary_food = 20
        else:
            nessesary_food = 40
        if self.inventory.get_ressource("food") < nessesary_food:
            if self.item_in_look("food") is not None:
                self.go_to(self.item_in_look("food"), "food")
                return
            else:
                if randint(0, 5) == 0:
                    self.command.append("Left")
                self.command.append("Forward")
                return
        elif self.need_to_elevation() is not None:
            if self.item_in_look(self.need_to_elevation()) is not None:
                self.go_to(
                    self.item_in_look(self.need_to_elevation()),
                    self.need_to_elevation(),
                )
                return
            else:
                if randint(0, 5) == 0:
                    self.command.append("Left")
                self.command.append("Forward")
                return
        else:
            self.command.append("Inventory")


    def update_inventory(self, result: list):
        value_return = 0
        for item in result:
            if self.inventory.get_ressource(item[0]) != int(item[1]):
                value_return = 1
            self.inventory.set_ressource(item[0], int(item[1]))
        self.share_inventory[self.id - 1].set_inventory(result)
        return value_return

    def update_share_inventory(self, result: list):
        for item in result:
            self.share_inventory[self.id - 1].set_ressource(item[0], item[1])
        self.update_total_inventory()

    def update_total_inventory(self):
        for j in items:
            self.share_inventory[6].set_ressource(j, 0)
        for i in range(6):
            for j in items:
                self.share_inventory[6].add_ressource(
                    j, self.share_inventory[i].get_ressource(j)
                )


    def end_turn_command(self):
        if self.inventory_in_turn == True:
            self.command.append("Inventory")
            self.inventory_in_turn = False
        if self.inventory_content != []:
            if self.update_inventory(self.inventory_content) == 1:
                self.broadcast_inventory()
        self.command.append("Look")

    def fork_player(self):
        if (
            self.inventory.get_ressource("food") < 20
            or self.enough_player == True
            or self.id != 0
        ):
            return
        if self.slot_open != 0:
            subprocess.Popen(
                [
                    "./zappy_ai",
                    "-p",
                    str(self.servhost),
                    "-n",
                    str(self.team_name),
                    "-h",
                    str(self.servport),
                ]
            )
            self.id = 1
        else:
            self.command.append("Fork")

    def message_interpreter(self, messages: list):
        for i in messages:
            direction = i.split(", ")[0]
            message = i.split(", ")[1].split("\n")[0]
            message = self.decode(message)
            if message == "error":
                continue
            id_player = int(message.split("|", 1)[0])
            message = message.split("|", 1)[1]
            if "ready" in message:
                self.enough_player = True
                if self.id == 0:
                    self.id = 1
            if "here" in message:
                self.id += 1
                if self.id == 6:
                    self.command.append("Broadcast " + self.encode("ready"))
            if "inventory" in message:
                self.share_inventory[id_player - 1].set_inventory(
                    utile.str_to_list(message.split("|")[1])
                )
                self.update_total_inventory()
            if "finished" in message:
                self.is_elevating = False
            if "elevation" in message and self.move_in_turn == False:
                self.command = []
                self.is_elevating = True
                direction = int(direction)
                if direction == 1:
                    self.command.append("Forward")
                elif direction == 5:
                    self.command.append("Left")
                    self.command.append("Left")
                    self.command.append("Forward")
                elif direction == 2 or direction == 3 or direction == 4:
                    self.command.append("Left")
                    self.command.append("Forward")
                elif direction == 6 or direction == 7 or direction == 8:
                    self.command.append("Right")
                    self.command.append("Forward")
                elif direction == 0:
                    self.drop_item_for_elevation()

    def drop_item_for_elevation(self):
        cpt = 1
        for i in items:
            if i == "food":
                continue
            if for_level[self.level - 1][cpt] != 0:
                for j in range(self.inventory.get_ressource(i)):
                    self.command.append("Set " + i)
            cpt += 1

    def command_interpreter(self, answer: list):
        for i in answer:
            command = i.split("|")
            if command[0] == "Connect_nbr":
                self.slot_open = int(command[1])
            elif command[0] == "Look":
                self.look_content = utile.str_to_list(command[1])
            elif command[0] == "Inventory":
                self.inventory_content = utile.str_to_list(command[1])
            elif command[0] == "Incantation":
                self.is_elevating = False
                if (len(command[1].split(":")) == 2):
                    self.level = int(command[1].split(":")[1])
                self.command.append("Broadcast " + self.encode("finished"))
            elif command[0] == "Forward":
                self.move_in_turn = True
            elif command[0] == "Right":
                self.move_in_turn = True
            elif command[0] == "Left":
                self.move_in_turn = True

    def wake_up(self):
        self.command.append("Broadcast " + self.encode("here"))
        self.command.append("Inventory")
        self.command.append("Look")
        self.command.append("Connect_nbr")

    def can_elevation(self) -> bool:
        nb_player = 0
        lim = 0
        der = 0
        sib = 0
        mend = 0
        phi = 0
        thys = 0
        if self.look_content == []:
            return False
        for i in self.look_content[0]:
            if "player" in i:
                nb_player += 1
            elif "linemate" in i:
                lim += 1
            elif "deraumere" in i:
                der += 1
            elif "sibur" in i:
                sib += 1
            elif "mendiane" in i:
                mend += 1
            elif "phiras" in i:
                phi += 1
            elif "thystame" in i:
                thys += 1
        if (
            nb_player >= 6
            and for_level[self.level - 1][1] <= lim
            and for_level[self.level - 1][2] <= der
            and for_level[self.level - 1][3] <= sib
            and for_level[self.level - 1][4] <= mend
            and for_level[self.level - 1][5] <= phi
            and for_level[self.level - 1][6] <= thys
        ):
            return True
        return False

    def elevation(self):
        if self.id != 6:
            return
        enought_food = True
        if self.is_elevating == False:
            for i in range(6):
                if self.share_inventory[i].get_ressource("food") < 40:
                    enought_food = False
        if self.need_to_elevation() is None and enought_food == True:
            self.is_elevating = True
        if self.is_elevating == True:
            self.command.append("Broadcast " + self.encode("elevation"))
            self.command.append("Look")
            self.drop_item_for_elevation()
            if self.can_elevation() == True:
                self.command.append("Incantation")

    def logic(self, answer: list, message: list) -> list:
        self.command = []
        if self.first_turn == True:
            self.wake_up()
            self.first_turn = False
            return self.command
        self.command_interpreter(answer)
        self.message_interpreter(message)
        self.elevation()
        if self.is_elevating == False:
            self.fork_player()
            self.take_item()
            self.end_turn_command()
        self.inventory_content = []
        self.move_in_turn = False
        self.command.append("Connect_nbr")
        return self.command
