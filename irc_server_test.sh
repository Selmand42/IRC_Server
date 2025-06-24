#!/bin/bash

PORT=6667
PASS="testpass"
SERVER_EXEC=./ircserv
SERVER_LOG=server_test.log

# Kill any previous server on the port
lsof -i :$PORT | grep ircserv | awk '{print $2}' | xargs kill -9 2>/dev/null || true

# Start the server in the background
$SERVER_EXEC $PORT $PASS > $SERVER_LOG 2>&1 &
SERVER_PID=$!
sleep 1

echo "Server started with PID $SERVER_PID"

# Test with first client (user1)
{
  echo "PASS $PASS"
  echo "NICK user1"
  echo "USER user1 0 * :User One"
  echo "JOIN #test"
  echo "PRIVMSG #test :Hello from user1"
  sleep 1
  # Test partial command (split PRIVMSG)
  echo -n "PRIVMSG #test :This is a partial"
  sleep 1
  echo " message from user1"
  sleep 1
  # Set topic
  echo "TOPIC #test :Welcome to the test channel!"
  sleep 1
  # Quit and rejoin
  echo "QUIT :leaving"
  sleep 1
  echo "NICK user1"
  echo "USER user1 0 * :User One"
  echo "PASS $PASS"
  echo "JOIN #test"
  sleep 1
  # Test joining invite-only channel (should fail)
  echo "JOIN #inviteonly"
  sleep 1
  # Wait for invite from user2
  sleep 2
  echo "JOIN #inviteonly"
  sleep 1
  # Test joining channel with key (should fail)
  echo "JOIN #keychan wrongkey"
  sleep 1
  # Join with correct key
  echo "JOIN #keychan secretkey"
  sleep 1
  # Test joining channel with user limit (should succeed for user1)
  echo "JOIN #limitchan"
  sleep 1
  # Test restricted mode
  echo "MODE user1 +r"
  sleep 1
} | nc 127.0.0.1 $PORT > user1.out &
USER1_PID=$!
sleep 1

# Test with second client (user2)
{
  echo "PASS $PASS"
  echo "NICK user2"
  echo "USER user2 0 * :User Two"
  echo "JOIN #test"
  sleep 1
  # Try to kick user1 before being op (should fail)
  echo "KICK #test user1 :should fail, not op"
  sleep 1
  # Become op by being first in channel after user1 quit
  echo "MODE #test +o user2"
  sleep 1
  # Now kick user1 (should succeed)
  echo "KICK #test user1 :now op, should succeed"
  sleep 1
  # Private message to user1
  echo "PRIVMSG user1 :Hello user1, this is a private message."
  sleep 1
  # Get topic
  echo "TOPIC #test"
  sleep 1
  # Test INVITE command
  echo "MODE #inviteonly +i"
  sleep 1
  echo "INVITE user1 #inviteonly"
  sleep 1
  # Test channel key (password)
  echo "MODE #keychan +k secretkey"
  sleep 1
  # Test user limit
  echo "MODE #limitchan +l 1"
  sleep 1
  # Test topic restriction
  echo "MODE #test +t"
  sleep 1
  # Test de-op
  echo "MODE #test -o user2"
  sleep 1
} | nc 127.0.0.1 $PORT > user2.out &
USER2_PID=$!
sleep 5

# Print outputs
echo "\n--- User1 Output ---"
cat user1.out

echo "\n--- User2 Output ---"
cat user2.out

# Clean up
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f user1.out user2.out $SERVER_LOG

echo "Test complete."
