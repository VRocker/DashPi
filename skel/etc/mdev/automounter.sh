#!/bin/sh

destdir=/recordings

my_umount()
{
	# Stop the recording process when unmounting
	killall recorder.bin

        umount -f "${destdir}"
}

my_mount()
{
        if ! mount -t auto -o noexec,rw,async "/dev/$1" "${destdir}"; then
                # failed to mount
                exit 1
        fi

	# Fire up the recorder when a USB stick is inserted
	/dashpi/bin/recorder.bin &
}

case "${ACTION}" in
add)
        my_umount ${MDEV}
        my_mount ${MDEV}
        ;;
remove)
        my_umount ${MDEV}
        ;;
*)
	;;
esac
