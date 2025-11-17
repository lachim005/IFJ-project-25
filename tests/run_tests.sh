#!/usr/bin/env bash

CORRECT_DIR=correct
COMP_DIR=comp-

RED='\033[0;31m'
YELLOW='\033[0;33m'
GREEN='\033[0;32m'
GRAY='\033[0;90m'
RESET='\033[0m'

EXE=../../main
INT=../../ic25int-linux-x86_64

run_correct() {
    echo --------------------------------------
    echo "      RUNNING CORRECT TESTS"
    echo --------------------------------------
    cd $CORRECT_DIR
    for i in *.test; do
        ((sum++))
        # Compile
        $EXE < $i 2> /dev/null 1> $i.ic25
        code=$?
        if [ $code -ne 0 ]; then
            echo -e "${RED}[ COMPILATION ERROR ]" $RESET $i "${GRAY}returned $code${RESET}"
            ((comp++))
            ((err++))
            continue
        fi

        if [ $CHECK_MEM -eq 1 ]; then
            # Check memory leaks
            valgrind --leak-check=full --errors-for-leak-kinds=all --error-exitcode=42 $EXE < $i 1>/dev/null 2>/dev/null
            if [ $? -eq 42 ]; then
                echo -e "${YELLOW}[ VALGRIND ERROR ]   " $RESET $i
                ((mem++))
            fi
        fi

        # Interpret
        timeout 2 $INT $i.ic25 2>/dev/null > $i.output
        code=$?
        if [ $code -ne 0 ]; then
            echo -e "${RED}[ INTERPRETER ERROR ]" $RESET $i "${GRAY}returned $code${RESET}"
            ((int++))
            ((err++))
            continue
        fi

        # Compare output
        diff $i.output $i.refoutput -s 2>/dev/null 1>/dev/null
        if [ $? -ne 0 ]; then
            echo -e "${RED}[ OUTPUT ERROR ]     " $RESET $i
            ((output_err++))
            ((err++))
            continue
        fi

        echo -e "${GREEN}[ CORRECT ]          " $RESET $i
        ((correct++))
    done
    cd $OLDPWD
}

run_with_comp_error() {
    echo --------------------------------------
    echo "  RUNNING TESTS WITH COMPILE ERROR $1"
    echo --------------------------------------
    cd $COMP_DIR$1
    for i in *.test; do
        ((sum++))
        # Compile
        $EXE < $i 2> /dev/null 1> /dev/null
        code=$?
        if [ $code -ne $1 ]; then
            echo -e "${RED}[ COMPILATION ERROR ]" $RESET $i "${GRAY}returned $code${RESET}"
            ((comp++))
            ((err++))
            continue
        fi

        if [ $CHECK_MEM -eq 1 ]; then
            # Check memory leaks
            valgrind --leak-check=full --errors-for-leak-kinds=all --error-exitcode=42 $EXE < $i 1>/dev/null 2>/dev/null
            if [ $? -eq 42 ]; then
                echo -e "${YELLOW}[ VALGRIND ERROR ]   " $RESET $i
                ((mem++))
            fi
        fi

        echo -e "${GREEN}[ CORRECT ]          " $RESET $i
        ((correct++))
    done


    cd $OLDPWD
}

if [[ "$1" == "-h" ]]; then
    echo USAGE:
    echo Just run it
    echo Include -m to run the compiler with valgrind
    exit 0
fi

CHECK_MEM=0
if [[ "$1" == "-m" ]]; then
    CHECK_MEM=1
fi

# Compile program
cd ..
make
if [ $? -ne 0 ]; then
    echo $RED Compilation error $RESET
    exit 1
fi
cd $OLDPWD

err=0
comp=0
mem=0
int=0
output_err=0
correct=0
sum=0

# Run correct tests
run_correct
run_with_comp_error 2
run_with_comp_error 3
run_with_comp_error 4
run_with_comp_error 5

# Print report
echo ======================================
if [ $correct -ne 0 ]; then
    echo -e "           Correct: ${GREEN}${correct}${RESET}/${sum}"
fi
if [ $mem -ne 0 ]; then
    echo -e "      Memory error: ${YELLOW}${mem}${RESET}/${sum}"
fi
if [ $comp -ne 0 ]; then
    echo -e " Compilation error: ${RED}${comp}${RESET}/${sum}"
fi
if [ $int -ne 0 ]; then
    echo -e " Interpreter error: ${RED}${int}${RESET}/${sum}"
fi
if [ $output_err -ne 0 ]; then
    echo -e "      Output error: ${RED}${output_err}${RESET}/${sum}"
fi
if [ $err -ne 0 ]; then
    echo - - - - - - - - - - - - - - - - - - -
    echo -e "      Total errors: ${RED}${err}${RESET}/${sum}"
fi
echo ======================================

if [ $err -ne 0 ]; then
    exit 1
fi

