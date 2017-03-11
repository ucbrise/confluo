# Must run scripts logged in as root
yum -y update

yum -y groupinstall "Development Tools"

# Install pre-requisites via yum
yum -y install libevent libevent-devel python-devel zlib-devel openssl-devel\
  mysql-devel ruby-devel rubygems java-devel cmake wget gcc-c++

# Create temporary directory for install files
rm -rf tmpdir
mkdir tmpdir
cd tmpdir

# Manually install autoconf
wget http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
tar xvf autoconf-2.69.tar.gz
cd autoconf-2.69
./configure --prefix=/usr
make -j
make install
cd ..

# Manually install automake
wget http://ftp.gnu.org/gnu/automake/automake-1.14.tar.gz
tar xvf automake-1.14.tar.gz
cd automake-1.14
./configure --prefix=/usr
make -j
make install
cd ..

# Manually install bison
wget http://ftp.gnu.org/gnu/bison/bison-2.5.1.tar.gz
tar xvf bison-2.5.1.tar.gz
cd bison-2.5.1
./configure --prefix=/usr
make -j
make install
cd ..

# Manually install boost
wget http://sourceforge.net/projects/boost/files/boost/1.55.0/boost_1_55_0.tar.gz
tar xvf boost_1_55_0.tar.gz
cd boost_1_55_0
./bootstrap.sh
./b2 threading=multi install
cd ..

# Manually install latest thrift
git clone https://git-wip-us.apache.org/repos/asf/thrift.git -b 0.10.0
cd thrift
./bootstrap.sh
./configure --with-lua=no
make -j
make install
cd ..

cd ..
rm -rf tmpdir
