#!/bin/sh

# ʹ����ȷ�������滻�﷨�ͱ�����ֵ
shm_id=$(ipcs -m | grep '0x000004d2' | awk '{print $2}')

# �������Ƿ�Ϊ��
if [ ! -z "$shm_id" ]; then
    ipcrm -m "$shm_id"
fi

file="cmd_fifo"

rm -rf $file
mkfifo $file
