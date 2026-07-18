#!/bin/bash
# setlocal EnableDelayedExpansion

LOW=1
HIGH=1

echo "Finding upper limit..."

increase() {
    echo ""
    echo "Testing $HIGH"

    python3 scripts/generate_benchmark.py $HIGH
    if [ $? -ne 0 ]; then
        echo "Generator failed"
        echo ""
        echo "=========================="
        echo "Maximum working COUNT: $LOW"
        echo "=========================="
        exit
    fi

    echo "Building..."

    ./bin/leyo build tests/benchmark.leyo -s
    RESULT=$?

    echo "Result: $RESULT"

    if [ $RESULT -eq 0 ]; then
        LOW=$HIGH
        HIGH=$((HIGH * 2))
        increase
    fi

    echo ""
    echo "Failed at $HIGH"
    echo "Searching between $LOW and $HIGH..."

    binary
}

binary() {
    if [ $LOW -ge $HIGH ]; then
        echo ""
        echo "=========================="
        echo "Maximum working COUNT: $LOW"
        echo "=========================="
        exit
    fi

    MID=$(( (LOW + HIGH) / 2 ))

    echo ""
    echo "Testing $MID"

    python3 scripts/generate_benchmark.py $MID
    if [ $? -ne 0 ]; then
        echo "Generator failed"
        echo ""
        echo "=========================="
        echo "Maximum working COUNT: $LOW"
        echo "=========================="
        exit
    fi

    ./bin/leyo build tests/benchmark.leyo -s
    RESULT=$?

    echo "Result: $RESULT"

    if [ $RESULT -eq 0 ]; then
        LOW=$MID
    else
        HIGH=$((MID - 1))
    fi

    binary
}

#done() {
#    echo ""
#    echo "=========================="
#    echo "Maximum working COUNT: $LOW"
#    echo "=========================="
#ß}

increase
