thrift_dir="`dirname "$0"`"
thrift_dir="`cd "$thrift_dir"; pwd`"

gen=$thrift_dir/thrift-gen.files
rm -rf $gen
mkdir -p $gen/
thrift -strict -verbose -gen cpp:templates,pure_enums,moveable_types -out $gen/ $thrift_dir/dialog.thrift

rm -rf $gen/*skeleton*

cd $gen/
FILES=./*.cpp
for f in $FILES
do
    filename=$(basename "$f")
    filename="${filename%.*}"
    mv $f "${filename}.cc"
done

cd $thrift_dir
mv $gen/*.cc $thrift_dir/../src/
mv $gen/*.h $gen/*.tcc $thrift_dir/../rpc/
rm -rf $gen
