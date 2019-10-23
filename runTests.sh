#!/bin/bash

INPUTDIR=$1
OUTPUTDIR=$2
MAXTHREADS=$3
NUMBUCKETS=$4


for input in ${INPUTDIR}/*.txt 
do
    echo ==== ${input} ====
    echo ${OUTPUTDIR}/${input::${#INPUTDIR}/.txt/.out} 
    #./tecnicofs-nosync $input ${OUTPUTDIR}/${input}-sync.out 1 1
done