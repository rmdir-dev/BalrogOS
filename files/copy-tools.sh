sudo mount -o loop ./filesys.dd root/
sudo cp -R ../build/bin/* root/bin/
sudo umount filesys.dd