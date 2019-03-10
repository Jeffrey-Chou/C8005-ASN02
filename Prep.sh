FOLDER_NAME=STRESS

make client
for i in 1 2 3 4 5
do
    FOLDER=${FOLDER_NAME}$i
    mkdir $FOLDER
    cp client $FOLDER
done

