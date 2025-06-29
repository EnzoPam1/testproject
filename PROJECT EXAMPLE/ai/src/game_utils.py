##
## EPITECH PROJECT, 2025
## Zappy
## File description:
## game_utils
##

import zlib
import base64

class GameUtils:
    @staticmethod
    def xor_cipher(message: str, key: str) -> str:
        """Apply XOR cipher with given key."""
        encrypted_message = ""
        key_length = len(key)
        
        for i, char in enumerate(message):
            key_char = key[i % key_length]
            encrypted_char = chr(ord(char) ^ ord(key_char))
            encrypted_message += encrypted_char
            
        return encrypted_message

    @staticmethod
    def encrypt_and_compress(message: str, key: str) -> str:
        """Encrypt and compress message."""
        # Compress the message
        compressed_message = zlib.compress(message.encode())
        
        # Encode to base64
        compressed_message_b64 = base64.b64encode(compressed_message).decode()
        
        # Apply XOR cipher
        encrypted_message = GameUtils.xor_cipher(compressed_message_b64, key)
        
        # Convert to hex
        return bytes(encrypted_message, "utf-8").hex()

    @staticmethod
    def decrypt_and_decompress(encrypted_message: str, key: str) -> str:
        """Decrypt and decompress message."""
        try:
            # Convert from hex
            decrypted_message = GameUtils.xor_cipher(
                bytes.fromhex(encrypted_message).decode("utf-8"), key
            )
            
            # Decode from base64
            compressed_message = base64.b64decode(decrypted_message)
            
            # Decompress
            decompressed_message = zlib.decompress(compressed_message).decode()
            
            return decompressed_message
            
        except Exception:
            return "error"

    @staticmethod
    def parse_string_to_list(content):
        """Parse string content into structured list."""
        if isinstance(content, str):
            content = content.split(",")
        
        result = []
        
        for item in content:
            # Clean up the item string
            item = item.replace("[", "").replace("]", "")
            
            # Remove leading/trailing spaces
            if item and item[0] == " ":
                item = item[1:]
            if item and item[-1] == " ":
                item = item[:-1]
            
            # Split into parts and create list
            if item:
                parts = item.split(" ")
                result.append(parts)
            else:
                result.append([])
        
        return result
