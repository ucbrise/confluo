rm -rf gen
mkdir -p gen/server
thrift -gen cpp -out gen/server server.thrift
rm -rf gen/*/*skeleton*

cd gen/server
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done
cd ../..
