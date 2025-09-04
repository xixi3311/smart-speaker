#!/bin/bash
set -euo pipefail

# 获取所有匹配的共享内存段 ID
shm_ids=$(ipcs -m | grep '0x004d2' | awk '{print $2}')

# 遍历所有匹配的段 ID 并删除
for shm_id in $shm_ids; do
    if [[ -n "$shm_id" ]]; then
        ipcrm -m "$shm_id"
    fi
done

file="cmd_fifo"

# 确保文件不存在
rm -rf "$file"
# 创建命名管道
mkfifo "$file"