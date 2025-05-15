# 对比判断加密结果是否正确

def clean_hex_string(s):
    return s.replace(" ", "").replace("\n", "").lower()

def compare_hex_strings(hex1, hex2):
    hex1_clean = clean_hex_string(hex1)
    hex2_clean = clean_hex_string(hex2)

    print("去空格后的 hex1：")
    print(hex1_clean)
    print("\n去空格后的 hex2：")
    print(hex2_clean)

    len1 = len(hex1_clean)
    len2 = len(hex2_clean)

    min_len = min(len1, len2)
    diff_index = None

    # 找出第一个不一样的位置
    for i in range(min_len):
        if hex1_clean[i] != hex2_clean[i]:
            diff_index = i
            break

    print("\n清理后的长度：")
    print(f"hex1: {len1} 个字符（{len1 // 2} 字节）")
    print(f"hex2: {len2} 个字符（{len2 // 2} 字节）")

    if diff_index is not None:
        print(f"\n从位置 {diff_index} 开始不同：")
        print(f"hex1[{diff_index}] = {hex1_clean[diff_index]}")
        print(f"hex2[{diff_index}] = {hex2_clean[diff_index]}")
    else:
        if len1 == len2:
            print("\n两个十六进制字符串完全一致。")
        else:
            print("\n字符串内容前面一致，但长度不同。")

# -*- coding: utf-8 -*-

from zhipuai import ZhipuAI

def zhenghe(message):
    client = ZhipuAI(api_key="b01fe371e52e41c48ad18f014632fd13.3YM1jQKGfE3oGjP8") # 填写您自己的APIKey
    response = client.chat.completions.create(
        model="glm-4-0520",  #填写需要调用的模型编码
        messages=[
            {"role":"user","content":message}
        ],
    )
    return response.choices[0].message.content

# 获取返回的加密结果字符串
string_test = zhenghe("风格 对应的值是？不存在或输入为空时回复 null,只回复值，不要回复其他内容")

# 这里应该调用对比函数，hex_str1 和 hex_str2 需要是你想对比的两个十六进制字符串
hex_str1 = string_test  # 假设 zhenghe 返回的是十六进制字符串
hex_str2 = string_test  # 假设你要和自己比较（或者其他数据）

compare_hex_strings(hex_str1, hex_str2)
