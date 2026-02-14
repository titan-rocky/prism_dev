#!/bin/bash

git add *

git commit -m "$(echo $1)"

eval "$(ssh-agent -s)"

ssh-add ~/.ssh/github_ssh

git push -u origin master

