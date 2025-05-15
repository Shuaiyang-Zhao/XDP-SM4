from flask import Flask, render_template, request, jsonify
from udp_cs import encrypt, decrypt, get_performance_stats, performance_lock, performance_data
import time

app = Flask(__name__, template_folder='templates')

# 缓存性能数据与上次更新时间
last_performance_update = 0
cached_performance_data = None

@app.route('/')
def index():
    # 刷新页面时清空所有历史数据
    with performance_lock:
        for k in [
            'latency_history',
            'encrypt_time_history',
            'decrypt_time_history',
            'throughput_history',
            'packet_sizes',
            # 更新为新的键名
            'encrypt_cpu_usage',
            'encrypt_memory_usage',
            'decrypt_cpu_usage',
            'decrypt_memory_usage'
        ]:
            if k in performance_data:
                performance_data[k].clear()
        performance_data['total_packets'] = 0
        performance_data['successful_packets'] = 0

    return render_template('index4.html')

@app.route('/encrypt', methods=['POST'])
def encrypt_route():
    global cached_performance_data

    data = request.get_json() or {}
    plaintext = data.get('plaintext', '').strip()
    if not plaintext:
        return jsonify(error="明文不能为空"), 400

    try:
        ciphertext, enc_time, payload_size = encrypt(plaintext)
    except Exception as e:
        return jsonify(error=f"加密失败：{e}"), 500

    try:
        decrypted, total_time, throughput = decrypt(ciphertext)
    except Exception as e:
        return jsonify(error=f"解密失败：{e}"), 500

    # 清掉缓存，确保下次 /performance 能拿到最新数据
    cached_performance_data = None

    return jsonify(
        ciphertext=ciphertext,
        decrypted=decrypted,
        metrics={
            "encrypt_time": enc_time * 1000,
            "total_time": total_time * 1000,
            "throughput": throughput,
            "payload_size": payload_size
        }
    )

@app.route('/performance', methods=['GET'])
def get_performance():
    global last_performance_update, cached_performance_data

    now = time.time()
    if cached_performance_data is None or now - last_performance_update > 0.5:
        try:
            cached_performance_data = get_performance_stats()
            last_performance_update = now
        except Exception:
            return jsonify(status="waiting_for_data")

    return jsonify(cached_performance_data)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5050, debug=True)
