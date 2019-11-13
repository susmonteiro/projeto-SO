#!/bin/bash

INPUTDIR=$1
OUTPUTDIR=$2
MAXTHREADS=$3
NUMBUCKETS=$4

mkdir -p $OUTPUTDIR #cria a diretoria de OUTPUT se esta ainda nao existir

for input in ${INPUTDIR}/*.txt 
do
    fileName_withExt=${input#${INPUTDIR}/}
    fileName_withoutExt=${fileName_withExt%.*}

    nThreads_nosync=1
    nBuckets_nosync=1
    outputFile_nosync=${OUTPUTDIR}/${fileName_withoutExt}-${nThreads_nosync}.txt
    echo InputFile=$fileName_withExt NumThreads=$nThreads_nosync

    ./tecnicofs-nosync $input $outputFile_nosync $nThreads_nosync $nBuckets_nosync | tail -n 1

    for nThreads in $(seq 2 $MAXTHREADS)
    do
        outputFile=${OUTPUTDIR}/${fileName_withoutExt}-${nThreads}.txt
        echo InputFile=$fileName_withExt NumThreads=$nThreads
        ./tecnicofs-mutex $input $outputFile $nThreads $NUMBUCKETS | tail -n 1
    done
done