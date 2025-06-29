import zlib
import base64
import json


"""
XOR cipher algorithm
Permet de chiffrer un message avec une clé
"""


def xor_cipher(message, key):
    encrypted_message = ""
    key_length = len(key)
    for i, char in enumerate(message):
        key_char = key[i % key_length]
        encrypted_char = chr(ord(char) ^ ord(key_char))
        encrypted_message += encrypted_char
    return encrypted_message

def xor_compressed_cipher(message, key):
    compressed_message = zlib.compress(message.encode())
    compressed_message_b64 = base64.b64encode(compressed_message).decode()
    encrypted_message = xor_cipher(compressed_message_b64, key)
    return bytes(encrypted_message, "utf-8").hex()


def xor_compressed_decipher(encrypted_message, key):
    try :
        decrypted_message = xor_cipher(bytes.fromhex(encrypted_message).decode("utf-8"), key)
        compressed_message = base64.b64decode(decrypted_message)
        zlib.decompress(compressed_message)
        decompressed_message = zlib.decompress(compressed_message).decode()
        return decompressed_message
    except:
        return "error"

def str_to_list(content: str):
    content = content.split(",")
    result = []
    for item in content:
        item = item.replace("[", "")
        item = item.replace("]", "")
        if item != "":
            if item[0] == " ":
                item = item[1:]
        if item != "":
            if item[-1] == " ":
                item = item[:-1]
        parts = item.split(" ")
        tmp = []
        for i in range(len(parts)):
            tmp.append(parts[i])
        result.append(tmp)
    return result



