#!/bin/bash

set -e

echo "Fetching latest branches..."
git fetch --all --prune

echo

for branch in $(git branch -r | grep '^  origin/' | grep -v 'HEAD' | sed 's|origin/||'); do
    if git show-ref --verify --quiet "refs/heads/$branch"; then
        echo "✓ $branch already exists locally"
    else
        echo "Creating local branch: $branch"
        git branch --track "$branch" "origin/$branch"
    fi
done

echo
echo "Done!"
echo
git branch