import subprocess
import time

# 定义程序文件名数组
programs = [
    "throught_16",
    "throught_32",
    "throught_64",
    "throught_128",
    "throught_256",
    "throught_512",
    "throught_1024",
    "throught_1472"
]

# 逐个执行程序
for prog in programs:
    print(f"正在执行 {prog} ...")
    
    # 执行程序
    result = subprocess.run([f"./{prog}"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
    # 打印程序输出
    print(result.stdout.decode())
    if result.stderr:
        print(result.stderr.decode(), file=sys.stderr)
    
    # 等待 10 秒
    print("等待 10 秒...")
    time.sleep(10)

print("所有测试完成。")
