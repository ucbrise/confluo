gen=thrift-gen.files
rm -rf $gen
mkdir -p $gen/
thrift -gen cpp -out $gen/ timeseries_db.thrift
thrift -gen java -out $gen/ timeseries_db.thrift
thrift -gen py -out $gen/ timeseries_db.thrift

#rm -rf $gen/*skeleton*

cd $gen/
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
cd ../
