#!/bin/bash

set -e

echo "========================================"
echo "      Git Branch Cleanup Utility"
echo "========================================"
echo

git fetch --prune

CURRENT=$(git branch --show-current)

echo "Current branch: $CURRENT"
echo

branches=()

echo "Available branches:"
echo

i=1
while IFS= read -r branch; do
    branch=$(echo "$branch" | sed 's/^[* ]*//')

    if [ "$branch" != "$CURRENT" ]; then
        branches+=("$branch")
        printf "  %2d) %s\n" "$i" "$branch"
        ((i++))
    fi
done < <(git branch)

echo

if [ ${#branches[@]} -eq 0 ]; then
    echo "No branches available to delete."
    exit 0
fi

echo "Enter branch numbers to delete."
echo "Examples:"
echo "  1"
echo "  1 3 5"
echo "  all"
echo

read -rp "> " selection

declare -a delete

if [[ "$selection" == "all" ]]; then
    delete=("${branches[@]}")
else
    for num in $selection; do
        if [[ "$num" =~ ^[0-9]+$ ]] &&
           (( num >= 1 && num <= ${#branches[@]} )); then
            delete+=("${branches[$((num-1))]}")
        fi
    done
fi

if [ ${#delete[@]} -eq 0 ]; then
    echo
    echo "Nothing selected."
    exit 0
fi

echo
echo "The following branches will be deleted:"
echo

for b in "${delete[@]}"; do
    echo "  • $b"
done

echo
read -rp "Continue? (y/N): " confirm

if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
    echo "Cancelled."
    exit 0
fi

echo

for b in "${delete[@]}"; do
    echo "Deleting local branch: $b"
    git branch -D "$b"

    if git ls-remote --heads origin "$b" | grep -q "$b"; then
        echo "Deleting remote branch: $b"
        git push origin --delete "$b"
    else
        echo "No remote branch named '$b'."
    fi

    echo
done

git fetch --prune

echo "========================================"
echo "Done!"
echo "Remaining branches:"
git branch
echo "========================================"