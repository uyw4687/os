./build-rpi3-arm64.sh
sudo ./scripts/mkbootimg_rpi3.sh
rm -rf ../tizen-image/
mkdir ../tizen-image/
cp boot.img ../tizen-image/
cp modules.img ../tizen-image/
cp tizen-unified_20181024.1_iot-headless-2parts-armv7l-rpi3.tar.gz ../tizen-image/tizen.tar.gz
cp ./copy.sh ../tizen-image/
cd ../tizen-image/
tar -xvzf tizen.tar.gz
rm tizen.tar.gz
mkdir mntdir
sudo mount rootfs.img ./mntdir/
sudo -s
sudo umount ~/tizen-image/mntdir
./qemu.sh