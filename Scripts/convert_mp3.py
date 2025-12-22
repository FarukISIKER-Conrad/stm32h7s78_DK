# convert_mp3.py
import os

input_file = "guitar.mp3"
output_file = "guitar_mp3_data.c"

with open(input_file, "rb") as f:
    data = f.read()

with open(output_file, "w") as f:
    f.write("// Auto-generated from guitar.mp3\n")
    f.write("#include <stdint.h>\n\n")
    f.write("__attribute__((section(\".rodata\")))\n")
    f.write("__attribute__((aligned(4)))\n")
    f.write("const uint8_t mp3_file_data[] = {\n    ")
    
    for i, byte in enumerate(data):
        f.write(f"0x{byte:02X}")
        if i < len(data) - 1:
            f.write(", ")
        if (i + 1) % 12 == 0:
            f.write("\n    ")
    
    f.write("\n};\n\n")
    f.write(f"const size_t mp3_file_size = {len(data)};\n")

print(f"Dosya oluşturuldu: {output_file} ({len(data)} bytes)")