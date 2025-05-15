import subprocess
import time

# 所有测试项名称
tests = [
    "latency_1024_10", "latency_1024_100", "latency_1024_1000", "latency_1024_10000", "latency_1024_100000", "latency_1024_50",
    "latency_128_10", "latency_128_100", "latency_128_1000", "latency_128_10000", "latency_128_100000", "latency_128_50",
    "latency_1472_10", "latency_1472_100", "latency_1472_1000", "latency_1472_10000", "latency_1472_100000", "latency_1472_50",
    "latency_16_10", "latency_16_100", "latency_16_1000", "latency_16_10000", "latency_16_100000", "latency_16_50",
    "latency_256_10", "latency_256_100", "latency_256_1000", "latency_256_10000", "latency_256_100000", "latency_256_50",
    "latency_32_10", "latency_32_100", "latency_32_1000", "latency_32_10000", "latency_32_100000", "latency_32_50",
    "latency_512_10", "latency_512_100", "latency_512_1000", "latency_512_10000", "latency_512_100000", "latency_512_50",
    "latency_64_10", "latency_64_100", "latency_64_1000", "latency_64_10000", "latency_64_100000", "latency_64_50"
]

for test in tests:
    print(f"\n🚀 正在执行测试: {test}")
    try:
        # 假设每个测试是一个可执行文件或脚本
        # 如果是 Python 脚本用 ["python3", f"{test}.py"]
        subprocess.run([f"./{test}"], check=True)
    except Exception as e:
        print(f"❌ 执行失败: {test}，错误: {e}")
    
    print("⏳ 等待 10 秒继续下一个测试...\n")
    time.sleep(10)

print("✅ 所有测试执行完毕。")
