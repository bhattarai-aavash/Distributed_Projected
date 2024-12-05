#!/bin/bash

# SSH into the remote server, start the Redis server in the background, and return to the original server
ssh -t abhattar@lhotse3.uwyo.edu << 'EOF'
  cd ~/redis/redis
  nohup ./src/redis-server redis.conf > redis.log 2>&1 &
  exit  # Exit the SSH session after starting Redis
EOF


echo "Redis server started on remote node. Now running your local script."

