import sys
import os

def build_sys(input_file, output_file):
    strings = {}
    max_id = -1
    
    with open(input_file, 'r', encoding='utf-8') as f:
        for line in f:
            if '=' not in line:
                continue
            parts = line.split('=', 1)
            msg_id = int(parts[0].strip())
            text = parts[1].rstrip('\n').replace('\\n', '\n')
            strings[msg_id] = text.encode('utf-8')
            if msg_id > max_id:
                max_id = msg_id
                
    num_strings = max_id + 1
    
    # Header: 1 byte num_strings
    # Table: 4 bytes per ID (2 for offset, 2 for length)
    
    table_size = num_strings * 4
    current_offset = 1 + table_size
    
    table_data = bytearray()
    string_data = bytearray()
    
    for i in range(num_strings):
        if i in strings:
            s = strings[i]
            length = len(s)
            if length > 511:
                length = 511
                s = s[:511]
            table_data.append(current_offset & 0xFF)
            table_data.append((current_offset >> 8) & 0xFF)
            table_data.append(length & 0xFF)
            table_data.append((length >> 8) & 0xFF)
            string_data.extend(s)
            current_offset += length
        else:
            table_data.extend([0, 0, 0, 0])
            
    with open(output_file, 'wb') as f:
        f.write(bytes([num_strings]))
        f.write(table_data)
        f.write(string_data)
        
if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: build_udos_sys.py <input.txt> <output.sys>")
        sys.exit(1)
    build_sys(sys.argv[1], sys.argv[2])
