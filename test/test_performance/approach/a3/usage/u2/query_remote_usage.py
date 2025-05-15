#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
query_remote_usage.py <IP> [<USERNAME> <PASSWORD>]

通过 SSH 查询给定 IP 的 CPU 和内存使用率，可选指定用户名和密码。
如果不提供用户名/密码，会根据内置映射选择：
 - 192.168.58.129 -> user=test-machine, password=020302
 - 192.168.58.128 -> user=zsy,          password=020302
"""
import paramiko
import sys

def query_usage(ip, username, password):
    command = r'''
CPU_LINE=$(top -b -n1 | grep 'Cpu(s)')
CPU_IDLE=$(echo "$CPU_LINE" | awk '{print $8}')
CPU_USAGE=$(echo "scale=2; 100 - $CPU_IDLE" | bc)
MEM_LINE=$(free -m | grep Mem)
TOTAL_MEM=$(echo $MEM_LINE | awk '{print $2}')
USED_MEM=$(echo $MEM_LINE | awk '{print $3}')
MEM_USAGE=$(echo "scale=2; $USED_MEM/$TOTAL_MEM*100" | bc)
echo "$CPU_USAGE $MEM_USAGE"
'''
    try:
        ssh = paramiko.SSHClient()
        ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh.connect(ip, username=username, password=password, timeout=5)
        stdin, stdout, stderr = ssh.exec_command(command)
        out = stdout.read().decode().strip()
        ssh.close()
        # 输出格式："cpu mem"
        print(out)
    except Exception:
        # 出错时输出两个 -1
        print("-1 -1")

if __name__ == "__main__":
    if len(sys.argv) not in (2,4):
        print("Usage: python3 query_remote_usage.py <IP> [<USERNAME> <PASSWORD>]")
        sys.exit(1)
    ip = sys.argv[1]
    if len(sys.argv) == 4:
        user, pwd = sys.argv[2], sys.argv[3]
    else:
        # 内置默认映射
        if ip == "192.168.58.129":
            user, pwd = "test-machine", "020302"
        elif ip == "192.168.58.128":
            user, pwd = "zsy", "020302"
        else:
            # 回退
            user, pwd = "root", "root"
    query_usage(ip, user, pwd)

