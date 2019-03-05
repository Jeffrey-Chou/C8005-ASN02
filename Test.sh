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
    echo "Testing with $THREADS clients, $LENGTH byte messages, $ITERATION times"
    exit 0
    DATAFOLDER=THREADS-${THREADS}-ITERATION-${ITERATION}-LENGTH-${LENGTH}
    mkdir $DATAFOLDER
    ./client -s $SERVER -t $THREADS -i $ITERATION -l $LENGTH
    RET=$?
    echo $RET
    mv *.txt $DATAFOLDER
    mv $DATAFOLDER $FOLDER
    THREADS=$((THREADS+50))
    echo "Testcase Done"
    sleep 2
done
echo "EXITING TEST"