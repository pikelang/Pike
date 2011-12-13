#!/bin/bash
#
# This script will automatically set up the current directory
# to mirror liu.se's pike repo to github.
# The first time it's run it will set up all the git remotes
# and do the first sync.
# If the version of the sync script updates then it will 
# use the newest one from github. Ace.
#
# When running for the first time, change into an empty directory
# and then call:
#   lynx -source https://github.com/pikelang/Pike/raw/github_sync/sync.sh | bash

DIR="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

if [ ! -e ".git" ]; then
  git init
fi

if [ "x1" != "x`git remote -v  | egrep -c '^origin\s+git://pike-git.lysator.liu.se/pike\s+\(fetch\)$'`" ]; then
  git remote add --mirror=fetch -f origin git://pike-git.lysator.liu.se/pike
fi

if [ "x1" != "x`git remote -v  | egrep -c '^github\s+git@github.com:pikelang/Pike.git\s+\(push\)$'`" ]; then
  git remote add --mirror=push -f github git@github.com:pikelang/Pike.git
fi

git fetch -q origin
git fetch -q github

if [ "x1" != "x`git branch | grep -c github_sync`" ]; then
  rm -f sync.sh
  git fetch github github_sync:github_sync
  git checkout -q github_sync
else 
  git checkout -q github_sync
fi

git push -q --mirror github

