#!/bin/bash

version=1.73.0
echo "Building boost $version..."

set -eu

toolchain=$PWD/toolchain-64
if [ ! -d "$toolchain" ]; then
  echo "Building toolchain..."
  ~/Android/NDK/build/tools/make-standalone-toolchain.sh \
      --arch=arm64 --platform=android-21 \
      --install-dir="$toolchain" \
      --toolchain=arm-linux-androideabi-clang \
      --use-llvm --stl=libc++
else
  echo "Toolchain already built"
fi

dir_name=boost_64_$(sed 's#\.#_#g' <<< $version)
arch_name=boost_$(sed 's#\.#_#g' <<< $version)
archive=${arch_name}.tar.bz2
if [ ! -f "$archive" ]; then
  # wget -O $archive "https://dl.bintray.com/boostorg/release/$version/source/$archive"
  wget -O $archive "https://boostorg.jfrog.io/artifactory/main/release/$version/source/$archive"
else
  echo "Archive $archive already downloaded"
fi

echo "Extracting..."
if [ ! -d "$dir_name" ]; then
  # rm -rf $dir_name
  tar xf $archive
  mv $arch_name $dir_name
else
  echo "Archive $archive already unpacked into $dir_name"
fi
cd $dir_name

echo "Generating config..."
user_config=tools/build/src/user-config.jam
rm -f $user_config
cat <<EOF > $user_config
import os ;
using clang : android
:
"$toolchain/bin/clang++"
:
<archiver>$toolchain/bin/aarch64-linux-android-ar
<ranlib>$toolchain/bin/aarch64-linux-android-ranlib
;
EOF

echo "Bootstrapping..."
./bootstrap.sh #--with-toolset=clang

echo "Building..."
./b2 -j4 \
    --with-atomic \
    --with-chrono \
    --with-container \
    --with-date_time \
    --with-exception \
    --with-fiber \
    --with-filesystem \
    --with-graph \
    --with-graph_parallel \
    --with-iostreams \
    --with-locale \
    --with-log \
    --with-math \
    --with-mpi \
    --with-program_options \
    --with-random \
    --with-regex \
    --with-serialization \
    --with-system \
    --with-thread \
    --with-timer \
    toolset=clang-android \
    architecture=arm \
    variant=release \
    --layout=versioned \
    target-os=android \
    threading=multi \
    threadapi=pthread \
    link=static \
    runtime-link=static \

echo "Running ranlib on libraries..."
libs=$(find "bin.v2/libs/" -name '*.a')
for lib in $libs; do
  "$toolchain/bin/aarch64-linux-android-ranlib" "$lib"
done

echo "Done!"
