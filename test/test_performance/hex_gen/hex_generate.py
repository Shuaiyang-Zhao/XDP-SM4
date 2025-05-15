# 随机自定义长度的16进制数

import random

def generate_hex_bytes(count):
    hex_bytes = []
    for _ in range(count):
        # 生成一个 0 到 255 之间的随机整数
        byte = random.randint(0, 255)
        # 将整数转为十六进制并格式化为 '0x' 开头
        hex_bytes.append(f"0x{byte:02x}")
    return hex_bytes

def save_to_file(hex_values, filename):
    with open(filename, 'w') as file:
        file.write(", ".join(hex_values))

# 示例：生成 8 个自定义字节十六进制数
count = 32
# count = 64
# count = 128
# count = 256
# count = 512
# count = 1024
# count = 1472

hex_values = generate_hex_bytes(count)

# 输出结果到 txt 文件
save_to_file(hex_values, "hex_bytes.txt")

print("生成的十六进制字节已保存到 'hex_bytes.txt' 文件中。")
