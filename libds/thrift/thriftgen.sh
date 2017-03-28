gen=thrift-gen.files
rm -rf $gen
mkdir -p $gen/
thrift -gen cpp -out $gen/ log_store.thrift
thrift -gen java -out $gen/ log_store.thrift
thrift -gen py -out $gen/ log_store.thrift

thrift -gen cpp -out $gen/ coordinator.thrift
thrift -gen java -out $gen/ coordinator.thrift
thrift -gen py -out $gen/ coordinator.thrift
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
