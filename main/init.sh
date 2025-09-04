#!/bin/sh

# 使用正确的命令替换语法和变量赋值
shm_id=$(ipcs -m | grep '0x000004d2' | awk '{print $2}')

# 检查变量是否为空
if [ ! -z "$shm_id" ]; then
    ipcrm -m "$shm_id"
fi

file="cmd_fifo"

rm -rf $file
mkfifo $file
