#!/bin/bash

export DB_URL=postgresql://USER:PASSWORD@HOST:PORT/DB

cd backend/build && ./run_server ~/Diploma/frontend/dist