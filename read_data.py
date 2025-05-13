import struct

def decode_binary_file(file_path):
    # Open the binary file in read-binary mode
    with open(file_path, "rb") as f:
        # Read the entire file content (or specify chunks)
        data = f.read()
        
        # Loop through the file in chunks of 24 bytes (4 bytes for message ID, 8 bytes for timestamp, 8 bytes for payload)
        index = 0
        while index + 24 <= len(data):
            chunk = data[index:index+24]

            # Extract the Message ID (4 bytes)
            message_id = struct.unpack('<I', chunk[:4])[0]  # Little-endian 4-byte integer
            
            # Extract the Timestamp (8 bytes)
            timestamp = struct.unpack('<Q', chunk[4:12])[0]  # Little-endian 8-byte unsigned long
            
            # Extract the Payload (8 bytes)
            payload = chunk[12:20]  # Raw bytes for the payload
            
            # Print the decoded values
            print(f"Message ID (hex): {message_id:08x}")  # Format as 8-digit hexadecimal
            print(f"Timestamp: {timestamp}")
            print(f"Payload (hex): {' '.join(f'{byte:02x}' for byte in payload)}")
            print("="*40)
            
            # Move to the next chunk of 24 bytes
            index += 24


# Example usage with the binary file created from the ESP32
file_path = "/media/udmt/canlog/log.bin"  # Replace with the actual file path

# Call the decode function
decode_binary_file(file_path)
