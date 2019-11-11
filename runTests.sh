#!/bin/bash

INPUTDIR=$1
OUTPUTDIR=$2
MAXTHREADS=$3
NUMBUCKETS=$4

<<<<<<< HEAD
if [ "$#" -ne 4 ]; then
    echo "Invalid number of arguments"
    exit 1
elif [ ! -d "${INPUTDIR}" ]; then
    echo "$1 is not a directory"
    exit 1
elif [[ ! "$MAXTHREADS" =~ ^[0-9]+$ ]] || [ "$MAXTHREADS" -lt 1 ]; then
    #se o numero de threads inserido for 1, corre apenas o nosync
    echo "Invalid number of threads"
    exit 1
elif [[ ! "$NUMBUCKETS" =~ ^[0-9]+$ ]] || [ "$NUMBUCKETS" -lt 1 ]; then
    echo "Invalid number of buckets"
    exit 1
elif [ ! -d "${OUTPUTDIR}" ]; then
    mkdir -p $OUTPUTDIR #cria a diretoria de OUTPUT se esta ainda nao existir
fi

=======
mkdir -p $OUTPUTDIR #cria a diretoria de OUTPUT se esta ainda nao existir
>>>>>>> e1e57424caf24e4b02fe8f4f925864ceb7d35831

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