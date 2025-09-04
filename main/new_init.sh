#!/bin/bash
set -euo pipefail

# ��ȡ����ƥ��Ĺ����ڴ�� ID
shm_ids=$(ipcs -m | grep '0x004d2' | awk '{print $2}')

# ��������ƥ��Ķ� ID ��ɾ��
for shm_id in $shm_ids; do
    if [[ -n "$shm_id" ]]; then
        ipcrm -m "$shm_id"
    fi
done

file="cmd_fifo"

# ȷ���ļ�������
rm -rf "$file"
# ���������ܵ�
mkfifo "$file"