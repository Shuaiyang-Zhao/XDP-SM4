import random
import string

def generate_hex_string(length):
    # 生成一个包含16个字符（0-9, a-f）的字符集
    hex_chars = string.hexdigits.lower()[:16]
    
    # 随机选择并生成指定长度的16进制字符串
    hex_string = ''.join(random.choice(hex_chars) for _ in range(length * 2))  # 每字节2个字符
    return hex_string

# 生成1450字节的16进制字符串
byte_length = 1450
hex_string = generate_hex_string(byte_length)
print(f"生成的16进制字符串: {hex_string}")
