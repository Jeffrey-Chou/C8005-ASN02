SERVER="192.168.0.4"
LENGTH=16
ITERATION=500
THREADS=200

make client
ulimit -n 8196
FOLDER=$(date | awk ' { print $4} ')
mkdir $FOLDER
RET=0
while [ $RET -eq 0 ]
do
    echo "Testing with $THREADS clients, $LENGTH byte messages, $ITERATION times"
    DATAFOLDER=THREADS-${THREADS}-ITERATION-${ITERATION}-LENGTH-${LENGTH}
    mkdir $DATAFOLDER
    ./client -s $SERVER -t $THREADS -i $ITERATION -l $LENGTH
    RET=$?
    echo $RET
    mv *.txt $DATAFOLDER
    mv $DATAFOLDER $FOLDER
    THREADS=$((THREADS+50))
    echo "Testcase Done"
    sleep 5
    read -p "Press enter to continue"
done
echo "EXITING TEST"