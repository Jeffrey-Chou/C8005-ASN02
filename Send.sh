FOLDER=STRESS

for i in 1 2 3 4 5
do
    ./${STRESS}$i/client -s 192.168.0.4 -t 300 -i 500 &
done
