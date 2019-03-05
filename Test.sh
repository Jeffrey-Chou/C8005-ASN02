SERVER="192.168.43.13"
LENGTH=16
ITERATION=100
THREADS=1

make client
FOLDER=$(date | awk ' { print $5} ')
mkdir $FOLDER
RET=0
while [ $RET -eq 0 ]
do
    DATAFOLDER=THREADS-${THREADS}-ITERATION-${ITERATION}-LENGTH-${LENGTH}
    mkdir $DATAFOLDER
    ./client -s $SERVER -t $THREADS -i $ITERATION -l $LENGTH
    RET=$?
    echo $RET
    mv *.txt $DATAFOLDER
    mv $DATAFOLDER $FOLDER
    THREADS=$((THREADS+1))
done
echo "TEST COMPLETE"