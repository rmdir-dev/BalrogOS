#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Usage: $0 <username> <password>"
  exit 1
fi

USERNAME=$1
PASSWORD=$2

# open fs/etc/passwd and search for the highest user id
MAX_ID=$(cut -d: -f3 fs/etc/passwd | sort -n | tail -1)

echo "Creating user $USERNAME with id $((MAX_ID + 1))"
echo -ne "$USERNAME:x:$((MAX_ID + 1)):$((MAX_ID + 1)):/home/$USERNAME:/bin/besh\n" >> fs/etc/passwd
echo -ne "$USERNAME:$PASSWORD:$((MAX_ID + 1)):$((MAX_ID + 1)):/home/$USERNAME:/bin/besh\n" >> fs/etc/shadow

echo "Creating password for user $USERNAME"
mkdir fs/home/$USERNAME

echo "create default besh config file"
cat << EOF > fs/home/$USERNAME/.spade
# Besh config file
HISTORIC_LEN=10
CURSOR_COLOR=97
# CURSOR=_

EOF
