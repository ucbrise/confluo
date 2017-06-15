gen=thrift-gen.files
rm -rf $gen
mkdir -p $gen/
thrift -gen cpp -out $gen/ log_store.thrift

rm -rf $gen/*skeleton*

cd $gen/
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
cd ../
