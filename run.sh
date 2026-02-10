#!/bin/bash

export DB_URL=postgresql://postgres:4317321@localhost:30432/central

cd backend/build && ./run_server . &
SERVER_PID=$!

sleep 5

cd frontend
npm install
npm run dev

kill $SERVER_PID 2>/dev/null
