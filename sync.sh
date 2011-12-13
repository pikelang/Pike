#!/bin/bash
#
# setup:
#   git init
#   git remote add --mirror -f origin git://pike-git.lysator.liu.se/pike
#   git remote add -f github git@github.com:pikelang/Pike.git

DIR="$( cd -P "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR

if [ ! -e ".git" ]; then
  git init
fi

if [ "x1" == "x`git remote -v  | egrep -c '^origin\s+git://pike-git.lysator.liu.se/pike\s+\(push\)$'`" ]; then
  git remote add --mirror -f origin git://pike-git.lysator.liu.se/pike
fi

if [ "x1" == "x`git remote -v  | egrep -c '^github\s+git@github.com:pikelang/Pike.git\s+\(push\)$'`" ]; then
  git remote add -f github git@github.com:pikelang/Pike.git
fi

git fetch -q origin
git fetch -q github
git checkout -q github_sync
git push -q --mirror github

