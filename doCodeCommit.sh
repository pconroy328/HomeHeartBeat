#!/bin/bash

echo "Calling git init"
git init

echo "Initializing Git to use AWS Code Commit"
git config --global user.name "Patrick Conroy"
git config --global user.email "patrick@conroy-family.net"

echo "Adding C source files and Headers"
git add *.c
git add *.h

echo "Adding Makefile"
git add Makefile

echo "Do a commit"
git commit

echo "After all files have been added - configure the remote repository"
echo "You'll need to correct AWS URL to the repository."
git remote add origin https://git-codecommit.us-east-1.amazonaws.com/v1/repos/HomeHeartBeat 

echo "Now issue 'git remote -v' to make sure remote is connected"
